/*
    LtusMain.cpp

    (c) Leltek 2018

    Implementation
    Cross-platform Core Module of Leltek UltraSound APP
     (Win32, Android, iOS)

    For how to use this module, see Leltek UltraSound-Bridging-Header.h
    For implementing customizing functions, go to the last section of this file



    Created by wiza on 2017/9/21.
    Copyright (c) 2017 Leltek. All rights reserved.



 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <time.h>

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(__BORLANDC__)
  #include <windows.h>
  #include <direct.h> //for chdir ()
#else
  #include <unistd.h> //for chdir ()
#endif

//header of APP core
#include "Leltek UltraSound-Bridging-Header.h"

//  Data Type

#ifndef byte
#define byte unsigned char
#endif
#ifndef UINT
#define UINT unsigned int
#endif
#ifndef BYTE
#define BYTE unsigned char
#endif

//header of device driver
#include "lelapi.h"


//--------------------------------------------------------------------------------
//  Essential Data
//

       #define HISTORY_SIZE 100   //# of frames in history

       //size of ultrasound data buffer
       #define BufU_X  128
       #define BufU_Y  1024
       #define BufB_X  128
       #define BufB_Y  1024
       #define BufB_Y_DISPLAY  512 //only display 512 lines of 1024
       #define BufC_X  128
       #define BufC_Y  256
       #define BufP_X  8
       #define BufP_Y  128
       #define BufM_X HISTORY_SIZE
       #define BufM_Y 1024

       #define DataBufferWidth BufB_X
       #define DataBufferHeight BufB_Y

       #define BufFilter_X 160
       #define BufFilter_Y 1200
               //must be larger than any buffer + 2 more columns + 2 more rows

       static  UINT CntU = 0;
       static  UINT CntB = 0;
       static  UINT CntC = 0;
       static  UINT CntP = 0;

       // buffer of result ultrasound image (large-precision version)
       // All following buffers are in row-major, that is,
       // data  of pixel (x,y) is at BufU [y * BufU_X + x]
       //
       static  UINT  BufU [BufU_X*BufU_Y];  //(size: 512K)

       //buffer for receiving from device
       static  byte  BufB_Recv [BufB_X*BufB_Y];  //(size: 128K)
       static  byte  BufC_Recv  [BufC_X*BufC_Y];  //(size: 32K)
       static  byte  BufP_Recv [BufP_X*BufP_Y];  //(size: 1K)

       //buffer for post-processing (may be in another thread)
       static  byte  BufB_Proc [BufB_X*BufB_Y];  //(size: 128K)
       static  byte  BufC_Proc  [BufC_X*BufC_Y];  //(size: 32K)
       static  byte  BufP_Proc [BufP_X*BufP_Y];  //(size: 1K)
       static UINT CntC_Proc = 0;
       static UINT CntP_Proc = 0;

       static  volatile int WaitingPostProcessor = 0;
       static  volatile int PostProcessing = 0;  //0-none, 1-processing, 2-done

       //buffer of result ultrasound image
       static  byte  BufB [BufB_X*BufB_Y];  //(size: 128K)
       static  byte  BufC [BufC_X*BufC_Y];  //(size: 32K)
       static  byte  BufP [BufP_X*BufP_Y];  //(size: 1K)

       //buffer of MMode
       static  byte  BufM [BufM_X*BufM_Y];  //(size: 100K)

       //buffer for filtering
       static  byte  BufFilter [BufFilter_X*BufFilter_Y];

       static  byte  BufFlag = 0;  //what is in current buffer
                         #define BufFlag_Color 1

       static  byte colorMapR [256], colorMapG [256], colorMapB [256];

       //Ring buffer for ultrasound history
       static byte HistoryB [HISTORY_SIZE][BufB_X*BufB_Y];
       static byte HistoryC [HISTORY_SIZE][BufC_X*BufC_Y];
       static byte HistoryP [HISTORY_SIZE][BufP_X*BufP_Y];
       static byte HistoryFlag [HISTORY_SIZE];
       static int HistoryNext = 0;  //next frame to put history


       //static  int TotalCntU;
       //static  int TotalCntB;
       static  bool stopClick = false;

       //FPS/data frequency statistic
       static unsigned long FPS_LastTime = 0;
       static int FPS_LastDataCount = 0, FPS_LastAccessCount = 0;
       static int FPS_CurrDataCount = 0, FPS_CurrAccessCount = 0;

       //status of device
       static int Lel_UltraSound_Initialized = 0;  //whether device is connected
       static int LelStarted = 0;                  //whether device is started or stopped
       static int FreezeButtonPressed= 0;   //1-pressed, 0-released or device not available

       static int ColorMode = 0;
       static int MMode = 0;
       static int MModeScanPos = BufB_X/2;   //The column being scanned in MMode



//--------------------------------------------------------------------------------
//  UI

//
//  RGB Display buffer, including ultrasound image, MMode image (if any)
//  and rulers. Entire buffer or part of it (if zooming in) will be displayed
//  on Mobile phone screen.
//
static unsigned char *DisplayBuffer = NULL;
static int DisplayBufferWidth = 0, DisplayBufferHeight = 0;
static int DisplayBufferShortEdge = 0;  //smaller one of width & height


//
//   (X, Y) Mapping from DisplayBuffer [] to data (Ultrasound/MMode)
//   Value DisplayMappingX [] is a combination of target x coordiate in ultrasound data
//   and which data area is to be mapped (ultrasound data, MMode ... etc)
//
//    Example: for a pixel (x,y) in DisplayBuffer,
//    Locate_Data_Type(x,y)  return 0 if this pixel is in ultrasound data display area;
//       1 if it is in MMode data display area;
//       if 0 is returned, this pixel is mapped to ultrasound data
//            at position (x', y') =  ( Locate_Data_X(x,y) ,  Locate_Data_Y(x,y))
//          or more precisely,  mapped to
//             BufB [ Locate_Data_Y(x,y) * BufB_X +  Locate_Data_X(x,y) ]
//       if 1 is returned, this pixel is mapped to MMode data
//            at position (x', y') =  ( Locate_Data_X(x,y) ,  Locate_Data_Y(x,y))
//          or more precisely,  mapped to
//             BufM [ Locate_Data_Y(x,y) * BufM_X +  Locate_Data_X(x,y) ]
//
//

#define Cut_Data_Div 10000
#define Locate_Data_Type(x, y) (DisplayMappingX [(y)*DisplayBufferWidth+(x)] / Cut_Data_Div)
   #define DataType_UltraSound 0
   #define DataType_MMode 1
#define Locate_Data_X(x, y) (DisplayMappingX [(y)*DisplayBufferWidth+(x)] % Cut_Data_Div)
#define Locate_Data_X_ALL(x, y) (DisplayMappingX [(y)*DisplayBufferWidth+(x)])
#define Locate_Data_Y(x, y) (DisplayMappingY [(y)*DisplayBufferWidth+(x)] % Cut_Data_Div)

//separate data type/pos from entries of DisplayMappingX []
#define Cut_Data_Type(x) ((x) / Cut_Data_Div)
#define Cut_Data_Pos(x) ((x) % Cut_Data_Div)

#define In_Data_UltraSound(x,y)  (DisplayMappingX [(y)*DisplayBufferWidth+(x)] > 0  &&  \
                                 DisplayMappingX [(y)*DisplayBufferWidth+(x)] / 10000 == 0 )
#define In_Data_Area(x,y)  (DisplayMappingX [(y)*DisplayBufferWidth+(x)] >= 0)
#define Out_Of_Data_Area(x,y)  (DisplayMappingX [(y)*DisplayBufferWidth+(x)] < 0)

static int *DisplayMappingX = NULL, *DisplayMappingY = NULL;
           /*
               index to ultrasound data
               Mapping from display buffer to ultrasound/mmode data
               NOTE: DisplayMappingY is a pointer to DisplayMappingX

               Use above marco such as Cut_Data_Type () or Locate_Data_Type () instead
            */

//
//pixel value change (for brightness/contrast modification)
// Display pixel value (0...255) =  ultrasound data value(0...255) * PixelValueScale  + PixelValueAdd
//
float PixelValueAdd  = 0.0f;
float PixelValueScale = 1.0f;
static unsigned char PixelValueMapping [256];


//Status Line of APP core
static char StatusLine [1024] = "(System Uninitialized)";

//
//   Projection Mode  (how ultra sound data are projected to display buffer)
//   FanDegree == 0 is used for linear head, the ultrasound data are projected to
//   a rectangle area in display buffer
//
//   FanDegree > 0 is used for convex head, the ultrasound data are projected to
//   a fan area in display buffer, where FanDegree is the angle of the fan, and
//    FanRadiusStart,  FanRadiusEnd are the radii of the fan.
//
static double FanDegree = 0, FanRadiusStart = 0,  FanRadiusEnd = 0;

       //
       // Detail information of current projection
       //
       static double LastFan_Degree = 0;
       static int LastFan_orgw = 0, LastFan_orgh = 0;
       static int LastFan_prjL = 0, LastFan_prjR = 0, LastFan_prjU = 0, LastFan_prjD = 0;
       static int LastFan_mmodedataw = 0, LastFan_mmodedatah = 0;
       static int LastFan_mmodeL = 0, LastFan_mmodeR = 0, LastFan_mmodeU = 0, LastFan_mmodeD = 0;
       static int  LastFan_centerx = 0;
       static double LastFan_currscale = 0, LastFan_ybias = 0, LastFan_yadd = 0;
       static double LastFan_fan2rad = 0, LastFan_FanRadiusStart = 0, LastFan_FanRadiusEnd = 0;


//
//  The Size of display view on your mobile phone screen
//
//  The relation between size of display buffer and display
//  view on screen is:
//
//    DisplayBufferWidth = ViewWidth /  ViewResolutionDiv
//    DisplayBufferHeight = ViewHeight /  ViewResolutionDiv
//
static int ViewWidth = 1, ViewHeight = 1;
static int ViewResolutionDiv = 1;


//
//  How APP core handles  user's touching screen
//
       #define TouchMode_None 0   //user is not touching screen yet
       #define TouchMode_Zoom 1   //zooming ultrasound image
       #define TouchMode_Mark 2    //add marks (rulers or annotations)
       #define TouchMode_MMode 3   //user is using MMode
static int CurrTouchMode = TouchMode_None;

static int TouchPhase= 0;  //0: none, 1-single touch ,  2-double touch
static int TouchX = 0, TouchY = 0, TouchX2 = 0, TouchY2 = 0;
static int TouchStartX = 0, TouchStartY = 0, TouchStartX2 = 0, TouchStartY2 = 0;
static float TouchStartScale = 1.0;
static int TouchStartLeft = 0, TouchStartUp = 0;

//
//   Zooming Status of ultrasound image
//
//   ZoomScale: how DisplayBuffer [] is scaled to screen
//     1.0 means normal size, 1.5 means 150%
//   (ZoomLeft, ZoomUp) means that
//     (x,y)=(ZoomLeft, ZoomUp) in DisplayBuffer []
//    should be mapped to upper-left corner on screen
//
static float  ZoomScale= 1.0;
static int ZoomLeft = 0, ZoomUp = 0;



//
//  Marks (rulers or annotations)
//
static int MarkMode = LelMarkMode_None;
//Cliff
static float FinalDis = 0.0;
//
//  Rulers for measuring distance (by line) or area (by ellipse)
//
//  Rulers are made of several "ruler points" (1...RulerPointMax),
//
//   From index RulerIndex_Line_Start to RulerIndex_Line_End,
//   each 2 points form a line to meause distance;
//   from index RulerIndex_Ellipse_Center to RulerIndex_Ellipse_Down.
//   5 points form an ellipse to measure area.
//
//   RulerPointX [0...RulerPointMax], RulerPointY [0...RulerPointMax]
//   stand for the coordinate of ruler points in ultrasound data, i.e. the
//   position in BufB []. If it is -1, this ruler point entry is empty
//
//   RulerDisplayX [0...RulerPointMax], RulerDisplayY [0...RulerPointMax]
//   stand for the coordinate of ruler points in display buffer, i.e. the
//   position in DisplayBuffer []. If it is -1, this ruler point entry is empty
//
//   NOTE: if DisplayBuffer [] being resized when mobile device is
//    rotated with screen size changed, or when window in screen of PC is
//    resized, RulerPointX/Y [] will kept and be RulerDisplayX/Y [] will be
//     re-calculated from RulerPointX/Y []
//
//

//variables used when user touching screen to draw a ruler (line or ellipse)
// holding means a ruler point is holding by user when user is touching screen
static int RulerPointHolding = 0;
static int RulerNewStartDataX = -1, RulerNewStartDataY = -1;
static int RulerNewStartDataType = -1;

static float RulerVolumeMultiplier = 0;

#define RulerPointMax 30   //must >  RulerLineMax * 2 + <other points>

  //display size of ruler points in DisplayBuffer []
  #define RulerPointDisplayRadius (DisplayBufferShortEdge/50)
  #define RulerPointTouchRadius (DisplayBufferShortEdge/25)

static int RulerPointX [RulerPointMax+1], RulerPointY [RulerPointMax+1];
                        // Control Points of Rulers (in coordinate of Ultrasound Data, BufB [])
                        //  X, Y >= 0 means point exists; -1 means point not exists

static int RulerDisplayX [RulerPointMax+1], RulerDisplayY [RulerPointMax+1];
                        // Control Points of Rulers (in coordinate of DisplayBuffer [])
                        //only valid after CalculateRulerDisplayPosition ()

                   //
                   //  Index of each ruler in above array  ([0] not used)
                   //  For example, ruler point of ellipise center is at
                   //   (RulerPointX [RulerIndex_Ellipse_Center],
                   //       RulerPointY [RulerIndex_Ellipse_Center])
                   //    in ultrasound data BufB []
                   //
                   #define RulerLineMax 5
                   #define RulerIndex_Line_Start 1   //(1,2) (3,4)...  stand for each lines
                   #define RulerIndex_Line_End 10

                   #define RulerIndex_Ellipse_Center 11
                   #define RulerIndex_Ellipse_Left 12
                   #define RulerIndex_Ellipse_Right 13
                   #define RulerIndex_Ellipse_Up 14
                   #define RulerIndex_Ellipse_Down 15


                   //
                   // R, G, B color value of each ruler point
                   // These colors are used to draw  rulers
                   //  in DisplayBuffer []
                   //
static int RulerColor [(RulerPointMax+1)*3] = { 0, 0, 0,
                                              //Lines
                                              0x00, 0xFF, 0xFF,   0x00, 0xFF, 0xFF,
                                              0x00, 0xFF, 0x00,   0x00, 0xFF, 0x00,
                                              0x00, 0x2F, 0xFF,   0x00, 0x2F, 0xFF,
                                              0xCF, 0x00, 0xFF,   0xCF, 0x00, 0xFF,
                                              0xCF, 0xFF, 0x00,   0xCF, 0xFF, 0x00,

                                              //Ellipse
                                              0x00, 0xCF, 0xFF,   0x00, 0xCF, 0xFF,   0x00, 0xCF, 0xFF,
                                              0x00, 0x6F, 0xFF,   0x00, 0x6F, 0xFF
                                           };



//
//  Annotations
//
//   Annotations are stored in a fixed array.
//   Anntations inputed by user are stored from index Annotation_StartIndexForUser;
//   array entries from 0 to Annotation_StartIndexForUser-1 are reserved for
//   APP core use. For example, APP core will use these entries to add an annotation
//   of result of measurement of ruler on screen.
//
#define Annotation_StartIndexForRuler 1
#define Annotation_StartIndexForRulerLine 1
#define Annotation_StartIndexForRulerEllipse  (1+RulerLineMax)
#define Annotation_EndIndexForRuler (1+RulerLineMax+2)

#define Annotation_StartIndexForUser 20


static char AnnotationText [LelAnnotation_Max+1] [LelAnnotation_MaxLen+8];
                           //annotation text in UTF-8 string

static int AnnotationDataX [LelAnnotation_Max+1];
static int AnnotationDataY [LelAnnotation_Max+1];
                           //position of annotation in ultrasound data (location in BufB [])

static int AnnotationL [LelAnnotation_Max+1];
static int AnnotationU [LelAnnotation_Max+1];
static int AnnotationW [LelAnnotation_Max+1];
static int AnnotationH [LelAnnotation_Max+1];
                            //Annotation display area (Left, Up, Width, Height)
                            //in display view on mobile phone screen
                            //must be updated by UI layer every cycle
                            //NOTE: above coordinate is in display view, not
                            // DisplayBuffer []; when ViewResolutionDiv == 1,
                            // both coordinates are equal to each other.

int AnnotationHolding = 0, LastAnnotationHolding = 0;
                            //annotation holding by user when user is touching screen

//--------------------------------------------------------------------------------

extern "C"
{

             static int Lel_UltraSound_SetFan (double ADegree, double ASkipRadius = 60, double ADisplayRadius = 180);
                            //
                            //  Set the project from ultra sound data to display buffer.
                            //  Will calculate the mapping from ultrasound data to display buffer, i.e. setting
                            //   DisplayMappingX/Y []
                            //  ADegree = 0 means rectangle projection; -1 means no change (reset for view change)
                            //   > 0 means the angle of fan projection
                            //   ASkipRadius and ADisplayRadius are in pixels. The fan will be displayed from radius
                            //    ASkipRadius, and display until (ASkipRadius + ADisplayRadius)
                            //   This function will try to keep aspect ratio and put the fan/rectangle in the middle of
                            //   DisplayBuffer []
                            //

             static int CalculateBufferPosition (int *x, int *y);
                            //Convert coordinate from ultrasound data BufB [] to DisplayBuffer []
                            //    (reverse operation of DisplayMappingX/Y [])
                            //must be used after Lel_UltraSound_SetFan ()
                            //

             static int CalculateDisplayPosition (int *x, int *y);
                            //Convert coordinate from ultrasound data BufB [] to coordinate on
                            //    view of screen of mobile phone
                            //  will call  CalculateBufferPosition () and convert to display position
                            //

             static float CalculateCurrentDisplayScale (void);

             static void CalculatePixelValueMapping (void);

             static int UltraSoundToDisplay (void);
                            // Re-Generate entire display image (RGB pixels) into DisplayBuffer []
                            //   from current ultrasound data (BufB [], BufC []... etc)
                            //  Rulers will be drawn together.
                            //

             static void ResetHistory (void);
                            //clear all history data

             static void HandleNewFrame (void);
             static void CheckNewFrame (void);
             static void AddNewFrame (void);
                            //sub functions for handling a new frame from device

             static int MedianFilter (byte *image, int width, int height);
                                      //apply filter to *image

             static float distLinear (int x0, int y0, int x1, int y1);
             static float distConvex (int x0, int y0, int x1, int y1);
                                       //calculate distance of two points in ultrasound data BufB []

             //static char* GetRulerInfo (char *result);

             static float GetRulerBufferLength (int idx1, int idx2);
                            //get the distance in pixels in DisplayBuffer []
                            // for ruler point [idx1] and [idx2]

             static float GetRulerRealLength (int idx1, int idx2);
                            //get the distance in mm in real world
                            // for ruler point [idx1] and [idx2]

             static float GetBufferRealLength (int x1, int y1, int x2, int y2);
                            //get length between (x1, y1) and (x2, y2)
                            // where x, y are coordinate in BufB []

             static void CalculateRulerDisplayPosition (int start = -1, int end = -1);
                      //calculate RulerDisplayX/Y [start...end], -1 means all ruler points

             static int ClearRulerWithSmallLength (void);
             static void UpdateEllipseRulerMark (void);
                                       //other operations for rulers

             static int PutAnnotationAtData (int index, int x, int y);
             static void ClearAnnotationInMMode (void);
                                       //operations for annotations

             static void LimitZoomingInRange (void);
                                       //range-checking after zooming modified

             //
             //  Customizing functions.
             //
             //   Rewrite these functions at the end of the module for customization.
             //
             static int InitializeCustomPostProcessor (void);  //return 1 on success, 0 if failure
             static void TerminateCustomPostProcessor (void);
             static void CustomPostProcessor_AfterDeviceConnected (int headtype);  //head type: 0x49-Linear  0x47-convex
             static void CustomPostProcessor_BeforeStartColorMode(void);
             static int StartCustomPostProcessor (void);  //return 0:none, 1:complete, -1:started but not completed
             static int CheckCustomPostProcessor (void);  //return 1 on complete, 0 still in processing


    int LelInitialize (const char *AWorkDir)
    {
      #if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(__BORLANDC__)
        _chdir (AWorkDir);
      #else
        chdir (AWorkDir);
      #endif

        // test if asset data accessible
        /*
        char fn [2048];
        sprintf (fn, "./cfg/47/bMode.txt");
        FILE *fp;
        if ( (fp = fopen (fn, "rb")) != NULL )
        {
            char line [200];
            fgets (line, 100, fp);
            strcat (line, "");
            fclose (fp);
        }
         */

        //Reset Lelapi data
        //----------------------------------------
        int i;

        stopClick = false;
        FPS_LastTime = 0;
        FPS_LastDataCount = 0;  FPS_LastAccessCount = 0;
        FPS_CurrDataCount = 0;  FPS_CurrAccessCount = 0;

        Lel_UltraSound_Initialized = 0;
        LelStarted = 0;

        ResetHistory ();

        LelResetZoom ();
        LelResetTouch ();
        LelResetRuler ();
        LelResetAnnotation ();

        CalculatePixelValueMapping ();

        memset(BufC, 128, sizeof(BufC));

        //palette for color ultrasound
        for(i=0;i<64;i++) {
             colorMapR[i] = 0;
             colorMapG[i] = (byte)(255-i*4);
             colorMapB[i] = (byte)255;
        }
        for(i=64;i<64*2;i++) {
             colorMapR[i] = 0;
             colorMapG[i] = 0;
             colorMapB[i] = (byte)(255-(i-64)*4);
        }
        for(i=64*2;i<64*3;i++) {
             colorMapR[i] = (byte)((i-64*2)*4);
             colorMapG[i] = 0;
             colorMapB[i] = 0;
        }
        for(i=64*3;i<64*4;i++) {
             colorMapR[i] = 255;
             colorMapG[i] = (byte)((i-64*3)*4);
             colorMapB[i] = 0;
        }


        //Initialize display buffer
        //----------------------------------------
        //Lel_UltraSound_SetFan (0);
        Lel_UltraSound_SetFan (60);

        //db
        //kiki
        int y, x;
        for ( y = 0; y < BufB_Y; y ++ )
          for ( x = 0; x < BufB_X; x ++ )
           {
             BufB [y * BufB_X + x] = BufB_Proc [y * BufB_X + x] = BufB_Recv [y * BufB_X + x] = (byte) ( x * 255 / BufB_X );
           }

        UltraSoundToDisplay ();


        if ( !InitializeCustomPostProcessor  ())
         {
            strcpy (StatusLine, "Cannot Initialize Custom Post Processor.");
            return 0;
         }

        strcpy (StatusLine, "Device Not Initialized Yet.");
        return 1;
    }

    void LelDestruct (void)
    {
        if ( LelStarted )
          LelStop ();

        TerminateCustomPostProcessor ();

        lelapi_exit();

        Lel_UltraSound_Initialized = 0;

        if ( DisplayBuffer != NULL )
         {
            delete DisplayBuffer;
            DisplayBuffer = NULL;
         }
        DisplayBufferWidth = DisplayBufferHeight = 0;
        DisplayBufferShortEdge = 0;
    }

     int  LelStart (void)
      {
        if ( !Lel_UltraSound_Initialized )
         {
             strcpy (StatusLine, "Connecting Device...");

              if ( lelapi_init ())
               {
                   strcpy (StatusLine, "LELAPI Initializing success");

                   int mode = lelapi_detect();

                   sprintf (StatusLine, "Detect Mode: 0x%X", mode);

                    if(mode==-1)
                       strcat (StatusLine, ": No Ultrasound device.");
                    else if(mode==0x49)
                       strcat (StatusLine, ": Linear");
                    else if(mode==0x47)
                       strcat (StatusLine, ": Convex");
                    else if(mode==0x1f)
                       strcat (StatusLine, ": No probe.");
                    else
                        strcat (StatusLine, ": Unknown");

                   if ( mode == 0x47)
                    {
                       Lel_UltraSound_SetFan(60);
                    }
                   else
                    {
                       Lel_UltraSound_SetFan(0);
                    }

                     CustomPostProcessor_AfterDeviceConnected (mode); //head type: 0x49-Linear  0x47-convex

                    if ( lelapi_set ())
                       Lel_UltraSound_Initialized = 1;
                   else
                       strcat (StatusLine, "Set Failure");
               }
              else
                 strcpy (StatusLine, "LELAPI Initializing FAIL");
         }
        if ( !Lel_UltraSound_Initialized )
           return 0;

          stopClick = false;
          if ( lelapi_start ())
           {
              strcpy (StatusLine, "Device Started Successfully.");

              lelapi_BModeSetting(false);
              //strcpy (StatusLine, "Device Compress Enabled");

               LelStarted = 1;

               WaitingPostProcessor = 0;

              return 1;
           }
          else
           {
              strcpy (StatusLine, "Device Start FAIL");
              return 0;
           }

          return 1;
      }


        static byte Uint2ByteLog(UINT i)
        {
            int o;

           //        UINT g_UpperLimitBufU = (uint)pow(10, (255 + 80) / 40);
           //        UINT g_LowerLimitBufU = (uint)pow(10, (0 + 80) / 40);
           //            if (i>= g_UpperLimitBufU) o = 255;
           //            else if (i<= g_LowerLimitBufU) o = 0;
           //            else o = (int)floor(40 * log10(i) - 80);
            o = (int)floor(40 * log10((double)(i)) - 48);
            if (o > 255) o = 255;
            else if (o < 0) o = 0;
            return (byte)o;
        }

                        //  Draw a line in DisplayBuffer [] from (x1, y1) to (x2, y2)
                        //   with color = (r, g, b)
             static void DisplayBuffer_Line (int  x1, int y1, int x2, int y2, int r, int g, int b, int drawperiod = 1)
              {
                 int temp;
                 int dx, dy;
                 int i;
                 int kcnt, kmod;
                 int px, py;
                 unsigned char cR = (unsigned char) r;
                 unsigned char cG = (unsigned char) g;
                 unsigned char cB = (unsigned char) b;

                 dx = x2 - x1;
                 dy = y2 - y1;
                 kcnt = 0;

                 if ( dx * dx > dy * dy )   //x side longer
                  {
                     if ( dx < 0 )
                      {
                         //exchange point
                         temp = x2, x2 = x1, x1 = temp;
                         temp = y2, y2 = y1, y1 = temp;
                         dx = -dx, dy = -dy;
                      }
                     if ( !dx )
                        kmod = 0;
                     else if ( (kmod = dy % dx) < 0 )
                        kmod = -kmod;

                     for ( px = x1, py = y1, i = 0; i <= dx; i ++, px ++ )
                      {
                          if ( px >= 0 && py >= 0 && px < DisplayBufferWidth && py < DisplayBufferHeight )
                           {
                               unsigned char *q = DisplayBuffer + (DisplayBufferWidth * py + px) * 4;

                                if ( (px - x1) % drawperiod == 0 )
                                 {
                                   #if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(__BORLANDC__)
                                       //Blue, Green, Red, None
                                       *(q++) = cB;
                                       *(q++) = cG;
                                       *(q++) = cR;
                                       *(q++) = 0xFF;
                                   #elif defined(__ANDROID__)
                                       //Red, Green, Blue, Alpha
                                       *(q++) = cR;
                                       *(q++) = cG;
                                       *(q++) = cB;
                                       *(q++) = 0xFF;
                                   #else //iOS
                                       //Alpha, Red, Green, Blue
                                       *(q++) = 0xFF;
                                       *(q++) = cR;
                                       *(q++) = cG;
                                       *(q++) = cB;
                                   #endif
                                 }
                           }

                          kcnt += kmod;
                          if ( kcnt >= dx )
                           {
                              kcnt-= dx;
                              if ( dy > 0 )
                                py ++;
                              else
                                py --;
                           }
                      }
                  }
                else  //y side longer or the same
                 {
                     if ( dy < 0 )
                      {
                         //exchange point
                         temp = x2, x2 = x1, x1 = temp;
                         temp = y2, y2 = y1, y1 = temp;
                         dx = -dx, dy = -dy;
                      }
                     if ( !dy )
                        kmod = 0;
                     else if((kmod = dx % dy) < 0)
                         kmod = -kmod;

                     //special case: dy is +/- dx
                     if (kmod ==0 && dx != 0 && dy != 0 )
                         kmod = dy;


                     for ( px = x1, py = y1, i = 0; i <= dy; i ++, py ++ )
                      {
                          if ( px >= 0 && py >= 0 && px < DisplayBufferWidth && py < DisplayBufferHeight )
                           {
                               unsigned char *q = DisplayBuffer + (DisplayBufferWidth * py + px) * 4;

                               if ( (py - y1) % drawperiod == 0 )
                                {
                                  #if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(__BORLANDC__)
                                       //Blue, Green, Red, None
                                       *(q++) = cB;
                                       *(q++) = cG;
                                       *(q++) = cR;
                                       *(q++) = 0xFF;
                                   #elif defined(__ANDROID__)
                                       //Red, Green, Blue, Alpha
                                       *(q++) = cR;
                                       *(q++) = cG;
                                       *(q++) = cB;
                                       *(q++) = 0xFF;
                                   #else //iOS
                                       //Alpha, Red, Green, Blue
                                       *(q++) = 0xFF;
                                       *(q++) = cR;
                                       *(q++) = cG;
                                       *(q++) = cB;
                                   #endif
                                }
                           }

                          kcnt += kmod;
                          if ( kcnt >= dy )
                           {
                              kcnt -= dy;
                              if ( dx > 0 )
                                px ++;
                              else
                                px --;
                           }
                      } //for
                  } //if y side longer or the same

              }

             static void DisplayBuffer_Rectangle (int  x1, int y1, int x2, int y2, int r, int g, int b)
              {
                  DisplayBuffer_Line (x1, y1, x2, y1, r, g, b);
                  DisplayBuffer_Line (x2, y1, x2, y2, r, g, b);
                  DisplayBuffer_Line (x2, y2, x1, y2, r, g, b);
                  DisplayBuffer_Line (x1, y2, x1, y1, r, g, b);
              }

                        //  Draw an ellipse in DisplayBuffer [] from (x1, y1) to (x2, y2)
                        //  with color = (r, g, b)
                        //  long axis: (x1, y1)-(x2-y2);  short axis: center to (x3, y3)
             static void DisplayBuffer_Ellipse (int  x1, int y1, int x2, int y2, int x3, int y3, int r, int g, int b, float DegreeStep = 1)
              {
                 int cx, cy;
                 int temp, dx, dy;
                 float dL;
                 float dcos, dsin;
                 float radiusL, radiusS;  //long/short radius
                 float deg;
                 double fx, fy, fx2, fy2;
                 int px, py;
                 unsigned char cR = (unsigned char) r;
                 unsigned char cG = (unsigned char) g;
                 unsigned char cB = (unsigned char) b;

                 if ((dx = x2 - x1) < 0 )
                  {
                     temp = x2, x2 = x1, x1 = temp;
                     temp = y2, y2 = y1, y1 = temp;
                     dx = -dx;
                  }
                 dy = y2 - y1;
                 dL = (float) sqrt ( (float) (dx * dx + dy * dy) );
                 if ( dL > 0 )
                   { dcos = dx / dL; dsin = dy / dL; }
                 else
                   dcos = 1, dsin = 0;

                 cx = (x1 + x2) / 2;  cy = (y1 +y2) / 2;
                 radiusL = (float) sqrt ( (float) ( (cx - x1) * (cx-x1) + (cy - y1) * (cy-y1)) ) ;
                 radiusS = (float) sqrt ( (float) ( (cx - x3) * (cx-x3) + (cy - y3) * (cy-y3)) ) ;

                 for ( deg = 0; deg < 360; deg += DegreeStep )
                  {
                      fx = radiusL * cos ( deg * 3.1416/180);
                      fy = radiusS * sin ( deg * 3.1416/180);

                      fx2 = fx * dcos - fy * dsin;
                      fy2 = fy * dcos + fx * dsin;

                      px = (int) (cx + fx2);
                      py = (int) (cy + fy2);

                           if ( px >= 0 && py >= 0 && px < DisplayBufferWidth && py < DisplayBufferHeight )
                            {
                                 unsigned char *q = DisplayBuffer + (DisplayBufferWidth * py + px) * 4;

                                #if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(__BORLANDC__)
                                    //Blue, Green, Red, None
                                    *(q++) = cB;
                                    *(q++) = cG;
                                    *(q++) = cR;
                                    *(q++) = 0xFF;
                                #elif defined(__ANDROID__)
                                    //Red, Green, Blue, Alpha
                                    *(q++) = cR;
                                    *(q++) = cG;
                                    *(q++) = cB;
                                    *(q++) = 0xFF;
                                #else //iOS
                                    //Alpha, Red, Green, Blue
                                    *(q++) = 0xFF;
                                    *(q++) = cR;
                                    *(q++) = cG;
                                    *(q++) = cB;
                                #endif
                            }
                  }

              }


                            // Re-Generate entire display image (RGB pixels) into DisplayBuffer []
                            //   from current ultrasound data (BufB [], BufC []... etc)
                            //   Rulers will be drawn together.
       static int UltraSoundToDisplay (void)
        {
           byte *p, *pcolor;
           unsigned char *q;
           int x, y, xmax, ymax;
           int *idx_x, *idx_y;
           int i;
           int datatype, datax, datay;
           unsigned char v;

           if ( DisplayBuffer == NULL || DisplayMappingX == NULL )
              return 0;

                xmax = DisplayBufferWidth;
                ymax = DisplayBufferHeight;

                for ( y = 0; y < ymax; y ++ )
                 {
                    if ( BufFlag & BufFlag_Color )  //color mode
                     {
                        for ( x = 0, q = DisplayBuffer + DisplayBufferWidth * 4 * y,
                                idx_x = DisplayMappingX + DisplayBufferWidth * y,
                                idx_y = DisplayMappingY + DisplayBufferWidth * y;
                               x < xmax; x ++, idx_x ++, idx_y ++ )
                            {
                                if ( (*idx_x) >= 0 )
                                 {
                                    datatype = Cut_Data_Type (*idx_x);
                                    datax = Cut_Data_Pos (*idx_x);
                                    datay = Cut_Data_Pos (*idx_y);

                                         pcolor = BufC + BufC_X * ((datay) / 2) + (datax);

                                         if (  ((*pcolor) > 128 + 2 ||  (*pcolor) < 128 - 2 ) &&
                                               datatype == DataType_UltraSound )
                                          {
                                            #if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(__BORLANDC__)
                                                //Blue, Green, Red, None
                                                *(q++) = (unsigned char) colorMapB [*pcolor];
                                                *(q++) = (unsigned char) colorMapG [*pcolor];
                                                *(q++) = (unsigned char) colorMapR [*pcolor];
                                                *(q++) = 0xFF;
                                            #elif defined(__ANDROID__)
                                                //Red, Green, Blue, Alpha
                                                *(q++) = (unsigned char) colorMapR [*pcolor];
                                                *(q++) = (unsigned char) colorMapG [*pcolor];
                                                *(q++) = (unsigned char) colorMapB [*pcolor];
                                                *(q++) = 0xFF;
                                            #else //iOS
                                                //Alpha, Red, Green, Blue
                                                *(q++) = 0xFF;
                                                *(q++) = (unsigned char) colorMapR [*pcolor];
                                                *(q++) = (unsigned char) colorMapG [*pcolor];
                                                *(q++) = (unsigned char) colorMapB [*pcolor];
                                            #endif
                                          }
                                         else
                                          {
                                             if (datatype == DataType_MMode )
                                                  p = BufM + BufM_X * (datay) + (datax);
                                             else
                                                  p = BufB + BufB_X * (datay) + (datax);

                                             v = PixelValueMapping [*p];

                                            #if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(__BORLANDC__)
                                                //Blue, Green, Red, None
                                                *(q++) = (unsigned char) v;
                                                *(q++) = (unsigned char) v;
                                                *(q++) = (unsigned char) v;
                                                *(q++) = 0xFF;
                                            #elif defined(__ANDROID__)
                                                //Red, Green, Blue, Alpha
                                                *(q++) = (unsigned char) v;
                                                *(q++) = (unsigned char) v;
                                                *(q++) = (unsigned char) v;
                                                *(q++) = 0xFF;
                                            #else //iOS
                                                //Alpha, Red, Green, Blue
                                                *(q++) = 0xFF;
                                                *(q++) = (unsigned char) v;
                                                *(q++) = (unsigned char) v;
                                                *(q++) = (unsigned char) v;
                                            #endif
                                          }

                                 }
                                else //Color mode: No-data-area
                                 {
                                #if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(__BORLANDC__)
                                    //Blue, Green, Red, None
                                    *(q++) = 0x1F;
                                    *(q++) = 0x1F;
                                    *(q++) = 0x1F;
                                    *(q++) = 0xFF;
                                #elif defined(__ANDROID__)
                                    //Red, Green, Blue, Alpha
                                    *(q++) = 0x1F;
                                    *(q++) = 0x1F;
                                    *(q++) = 0x1F;
                                    *(q++) = 0xFF;
                                #else //iOS
                                    *(q++) = 0xFF;
                                    *(q++) = 0x1F;
                                    *(q++) = 0x1F;
                                    *(q++) = 0x1F;
                                #endif
                                 }
                            }  //for x
                     }  //if color mode
                    else  //black/white mode
                     {
                        for ( x = 0, q = DisplayBuffer + DisplayBufferWidth * 4 * y,
                                idx_x = DisplayMappingX + DisplayBufferWidth * y,
                                idx_y = DisplayMappingY + DisplayBufferWidth * y;
                               x < xmax; x ++, idx_x ++, idx_y ++ )
                            {
                                if ( (*idx_x) >= 0 )
                                 {
                                    datatype = Cut_Data_Type (*idx_x);
                                    datax = Cut_Data_Pos (*idx_x);
                                    datay = Cut_Data_Pos (*idx_y);

                                    if (datatype == DataType_MMode )
                                         p = BufM + BufM_X * (datay) + (datax);
                                    else
                                         p = BufB + BufB_X * (datay) + (datax);

                                     v = PixelValueMapping [*p];

                                     #if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(__BORLANDC__)
                                         //Blue, Green, Red, None
                                         *(q++) = (unsigned char) v;
                                         *(q++) = (unsigned char) v;
                                         *(q++) = (unsigned char) v;
                                         *(q++) = 0xFF;
                                     #elif defined(__ANDROID__)
                                         //Red, Green, Blue, Alpha
                                         *(q++) = (unsigned char) v;
                                         *(q++) = (unsigned char) v;
                                         *(q++) = (unsigned char) v;
                                         *(q++) = 0xFF;
                                     #else //iOS
                                         //Alpha, Red, Green, Blue
                                         *(q++) = 0xFF;
                                         *(q++) = (unsigned char) v;
                                         *(q++) = (unsigned char) v;
                                         *(q++) = (unsigned char) v;
                                     #endif


                                 }
                                else //BW mode: No-data-area
                                 {
                                #if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(__BORLANDC__)
                                    //Blue, Green, Red, None
                                    *(q++) = 0x1F;
                                    *(q++) = 0x1F;
                                    *(q++) = 0x1F;
                                    *(q++) = 0xFF;
                                #elif defined(__ANDROID__)
                                    //Red, Green, Blue, Alpha
                                    *(q++) = 0x1F;
                                    *(q++) = 0x1F;
                                    *(q++) = 0x1F;
                                    *(q++) = 0xFF;
                                #else //iOS Alpha, Red, Green, Blue
                                    *(q++) = 0xFF;
                                    *(q++) = 0x1F;
                                    *(q++) = 0x1F;
                                    *(q++) = 0x1F;
                                #endif
                                 }
                            }  //for x
                     } //else black/white mode
                 } //for y

                /* Ruler
                   ------------------------ */

                          //Ruler on Screen Side
                          if (  DisplayBufferHeight > 0 &&  ZoomScale > 0 )
                           {
                              int  x, y, ystart, yend;
                              float reallen;
                              int c;
                              int mw, mh, mw_curr, xpad;

                              x = BufB_X /2 , ystart = 0;
                              CalculateBufferPosition (&x, &ystart);

                              x = BufB_X /2 , yend = BufB_Y_DISPLAY - 1;
                              CalculateBufferPosition (&x, &yend);

                              if ((reallen = GetBufferRealLength (BufB_X /2 , 0,  BufB_X /2 , BufB_Y_DISPLAY - 1)) > 0 )
                               {
                                   mw = (int) ( ((float)DisplayBufferWidth) / ZoomScale /  80);
                                   mh = (int) ( ((float)DisplayBufferHeight) / ZoomScale /  200);
                                   if (mh > 1 )
                                     mh = 1;

                                   xpad = 0;

                                   for ( c = 0; c <= (int) reallen; c +=10 )
                                    {
                                        y = ystart + (int)(c * (yend - ystart) / reallen);
                                       if ( y >= ZoomUp && y <= (int) (ZoomUp + DisplayBufferHeight / ZoomScale) )
                                        {
                                             if ( c % 100 == 0 )
                                                mw_curr = mw *4;
                                             else if ( c % 50== 0 )
                                                mw_curr = mw *2;
                                             else
                                                mw_curr = mw;
                                             DisplayBuffer_Rectangle (ZoomLeft + xpad, y, ZoomLeft + xpad + mw_curr, y + mh,
                                                                  0X80, 0xC0, 0xC0);
                                        }
                                    }
                               } //if reallen >0

                           } //Ruler on Screen if DisplayBufferHeight > 0

                          //Lines
                          for ( i = RulerIndex_Line_Start; i <= RulerIndex_Line_End; i += 2 )
                            if ( RulerDisplayX [i] >= 0 && RulerDisplayX [i+1] >= 0 )
                             {
                                DisplayBuffer_Line ( RulerDisplayX [i], RulerDisplayY [i],
                                                    RulerDisplayX [i+1], RulerDisplayY [i+1],
                                                    0, 0, 0,   1);
                                DisplayBuffer_Line ( RulerDisplayX [i], RulerDisplayY [i],
                                                    RulerDisplayX [i+1], RulerDisplayY [i+1],
                                                    RulerColor [i*3], RulerColor [i*3+1],RulerColor [i*3+2], 2);
                             }

                          //Point of Lines as cross
                          for ( i = RulerIndex_Line_Start; i <= RulerIndex_Line_End; i ++)
                            if (  RulerDisplayX [i] >= 0 && RulerDisplayY [i] >= 0 )
                             {
                                DisplayBuffer_Line
                                     ( RulerDisplayX [i] - RulerPointDisplayRadius*4/3,  RulerDisplayY [i],
                                       RulerDisplayX [i] + RulerPointDisplayRadius*4/3,  RulerDisplayY [i],
                                       0, 0, 0);
                                DisplayBuffer_Line
                                     ( RulerDisplayX [i] ,  RulerDisplayY [i] - RulerPointDisplayRadius*4/3,
                                       RulerDisplayX [i],  RulerDisplayY [i] + RulerPointDisplayRadius*4/3,
                                       0, 0, 0);

                                DisplayBuffer_Line
                                     ( RulerDisplayX [i] - RulerPointDisplayRadius*4/5,  RulerDisplayY [i],
                                       RulerDisplayX [i] + RulerPointDisplayRadius*4/5,  RulerDisplayY [i],
                                       RulerColor [i*3], RulerColor [i*3+1],RulerColor [i*3+2]);
                                DisplayBuffer_Line
                                     ( RulerDisplayX [i] ,  RulerDisplayY [i] - RulerPointDisplayRadius*4/5,
                                       RulerDisplayX [i],  RulerDisplayY [i] + RulerPointDisplayRadius*4/5,
                                       RulerColor [i*3], RulerColor [i*3+1],RulerColor [i*3+2]);
                             }


                         //Ellipse
                         if ( RulerDisplayX [RulerIndex_Ellipse_Center] >= 0 && RulerDisplayX [RulerIndex_Ellipse_Down] >= 0 )
                           {
                               DisplayBuffer_Ellipse ( RulerDisplayX [RulerIndex_Ellipse_Left], RulerDisplayY [RulerIndex_Ellipse_Left],
                                                        RulerDisplayX [RulerIndex_Ellipse_Right], RulerDisplayY [RulerIndex_Ellipse_Right],
                                                        RulerDisplayX [RulerIndex_Ellipse_Up], RulerDisplayY [RulerIndex_Ellipse_Up],
                                                        RulerColor [RulerIndex_Ellipse_Up*3], RulerColor [RulerIndex_Ellipse_Up*3+1],
                                                        RulerColor [RulerIndex_Ellipse_Up*3+2],
                                                        1.0f);

                               DisplayBuffer_Line ( RulerDisplayX [RulerIndex_Ellipse_Left], RulerDisplayY [RulerIndex_Ellipse_Left],
                                                        RulerDisplayX [RulerIndex_Ellipse_Right], RulerDisplayY [RulerIndex_Ellipse_Right],
                                                        RulerColor [RulerIndex_Ellipse_Left*3], RulerColor [RulerIndex_Ellipse_Left*3+1],
                                                        RulerColor [RulerIndex_Ellipse_Left*3+2], 2);

                               //rectangle
                               for ( i = RulerIndex_Ellipse_Center; i <= RulerIndex_Ellipse_Down; i ++)
                                 if (  RulerDisplayX [i] >= 0 && RulerDisplayY [i] >= 0 )
                                  {
                                     DisplayBuffer_Rectangle
                                          ( RulerDisplayX [i] - RulerPointDisplayRadius-1,  RulerDisplayY [i] - RulerPointDisplayRadius-1,
                                            RulerDisplayX [i] + RulerPointDisplayRadius+1,  RulerDisplayY [i] + RulerPointDisplayRadius+1,
                                            0, 0, 0);
                                     DisplayBuffer_Rectangle
                                          ( RulerDisplayX [i] - RulerPointDisplayRadius,  RulerDisplayY [i] - RulerPointDisplayRadius,
                                            RulerDisplayX [i] + RulerPointDisplayRadius,  RulerDisplayY [i] + RulerPointDisplayRadius,
                                            RulerColor [i*3], RulerColor [i*3+1],RulerColor [i*3+2]);
                                  }
                           }

                /* MMode
                   ------------------------ */
                      if ( MMode )
                       {
                          int x, y, x2, y2;
                          x = x2 = MModeScanPos;
                          y = 0,  y2 = BufB_Y_DISPLAY-1;
                          CalculateBufferPosition (&x, &y);
                          CalculateBufferPosition (&x2, &y2);
                          DisplayBuffer_Line (x, y, x2, y2, 0x30, 0xFF, 0xFF);
                       }


           return 1;
        }

          static void ResetHistory (void)
           {
              memset (HistoryB, 0, sizeof (HistoryB));
              memset (HistoryC, 0x80, sizeof (HistoryC));
              memset (HistoryP, 0, sizeof (HistoryP));
              memset (HistoryFlag, 0, sizeof (HistoryFlag));
              HistoryNext = 0;
           }

          static void AddDataToHistory (void)
           {
              memcpy (HistoryB [HistoryNext], BufB, sizeof (BufB));
              memcpy (HistoryC [HistoryNext], BufC, sizeof (BufC));
              memcpy (HistoryP [HistoryNext], BufP, sizeof (BufP));
              HistoryFlag [HistoryNext] = BufFlag;

              if ( ++HistoryNext >= HISTORY_SIZE )
                HistoryNext = 0;
           }

          static void ClearMModeData (void)
           {
               memset(BufM, 0, sizeof(BufM));
           }

          static void AddDataToMMode (void)
           {
              //shift image left by 1 pixel
              memmove (BufM, BufM + 1, sizeof (BufM) - 1);

              //add new data to right-most line
              int y;
              byte *p, *q;
              for ( y = 0, p = BufB + MModeScanPos, q = BufM + (BufM_X - 1);
                     y < BufM_Y; y ++, p += BufB_X, q += BufM_X )
                  *q = *p;
           }


     void LelCycle (void)
      {
          if ( !LelStarted)
           {
               /*
               if ( MarkMode != LelMarkMode_None )
                {
                   *StatusLine = 0;
                   GetRulerInfo (StatusLine);
                }
               */
              return;
           }

             /* Check processing frame
                ---------------------------------------- */
             CheckNewFrame ();


             /* Try To Receive New Frame From Device
                ------------------------------------------------ */
             if ( !WaitingPostProcessor )
              {
                  bool status;
                  UINT *pBufU = BufU;
                  byte* pBufB = BufB_Recv;
                  byte *pBufC = BufC_Recv;
                  byte *pBufP = BufP_Recv;

                 status = lelapi_ImgData(pBufU, CntU, pBufB, CntB, pBufC, CntC, pBufP, CntP);
                 if (status == false)
                 {
                    strcpy (StatusLine, "Get data error!");
                 }
                 else  //get image success
                  {
                             if (CntU > 0)
                             {
                                 int copysize;
                                 copysize = BufU_X*BufU_Y;
                                 if ( copysize > BufB_X*BufB_Y )
                                      copysize = BufB_X*BufB_Y;
                                 for (int j = 0; j < copysize; j++)
                                     *(pBufB + j) = Uint2ByteLog(*(pBufU + j));

                                 HandleNewFrame ();
                             }
                             else if (CntB > 0)
                                 HandleNewFrame ();


                  }    //get image success
              }  //if  not  WaitingPostProcessor


            //sprintf (StatusLine, "%d Hz (FPS %d)", FPS_LastDataCount, FPS_LastAccessCount);

            if (stopClick )
            {
                lelapi_stop();
                strcat (StatusLine, "Stop buffering image data.");
            }


      }

     void  LelStop (void)
      {
          if ( !LelStarted)
              return;

          stopClick = true;

          LelCycle ();
          LelStarted = 0;

          strcpy (StatusLine, "Device Stopped.");

      }

     int LelSetColorMode (int mode)
      {
          if ( !LelStarted)
             return 0;

         memset(BufC, 0x80, sizeof(BufC));
         memset(BufC_Recv, 0x80, sizeof(BufC_Recv));
         memset(BufC_Proc, 0x80, sizeof(BufC_Proc));

         if ( mode )
            CustomPostProcessor_BeforeStartColorMode ();

         int kret =  ( lelapi_CModeSetting ((mode != 0)? true: false ))? 1: 0;
         if ( kret )
           ColorMode = mode;

         return kret;
      }

     int LelGetColorMode (void)
      {  return ColorMode; }


     int LelSetMMode (int mode)
      {
          /*
          if ( !LelStarted)
             return 0;
           */
         ClearMModeData ();

         //db 20171215
         int x, y;
         for ( y = 0; y < BufM_Y; y ++ )
           for ( x = 0; x <BufM_X; x ++ )
              BufM [BufM_X * y + x ] = (unsigned char) ( x % 256);


         MMode = mode;

          LelResetZoom();
          LelResetTouch();

          if ( mode == 0 )
             ClearAnnotationInMMode ();

          Lel_UltraSound_SetFan (-1) ; //re-generate buffer

          CalculateRulerDisplayPosition (); //repose rulers

          UltraSoundToDisplay ();

         return 1;
      }

     int LelGetMMode (void)
      {  return MMode; }

     int LelSetPower (int mode) //1-30V, 0-20V
      {
          if ( !LelStarted)
             return 0;
          return  ( lelapi_Power ((mode != 0)? true: false ))? 1: 0;
      }

     int LelSetGain (int mode)  //1-24db, 0-18db
      {
          if ( !LelStarted)
             return 0;
          return  ( lelapi_LNA ((mode != 0)? true: false ))? 1: 0;
      }

     void LelSetResolutionDiv(int div)
     {
         ViewResolutionDiv = div;
     }

            static void LimitZoomingInRange (void)
             {
                         //keep zoom in boundary
                         int bufw = DisplayBufferWidth, bufh = DisplayBufferHeight;
                         if ( ZoomLeft + (int) (bufw / ZoomScale) > bufw )
                            ZoomLeft = bufw - (int) (bufw / ZoomScale);
                         if ( ZoomUp + (int) (bufh / ZoomScale) > bufh )
                            ZoomUp =  bufh  - (int) (bufh / ZoomScale);
                         if ( ZoomLeft < 0 )
                             ZoomLeft = 0;
                         if ( ZoomUp < 0 )
                             ZoomUp = 0;
             }

     void LelSetViewSize (int w, int h)
      {
          if ( w < 1 || h < 1 )
             return; //zero size may happen during window creation
          ViewWidth = w;  ViewHeight = h;

                  //try to keep zooming status
                  float orgzoom_scale = ZoomScale;
                  int orgzoom_cx = BufB_X/2, orgzoom_cy = BufB_Y_DISPLAY/2;
                  if ( DisplayBufferWidth > 0 && DisplayBufferHeight > 0 && ZoomScale > 0 )
                   {
                       int px, py;
                       px = ZoomLeft + (int) (DisplayBufferWidth / 2/ ZoomScale);
                       py = ZoomUp +  (int) (DisplayBufferHeight / 2/ ZoomScale);
                       if ( In_Data_UltraSound (px, py))
                        {
                          orgzoom_cx = Locate_Data_X (px,py);
                          orgzoom_cy = Locate_Data_Y (px,py);
                        }
                      else
                        orgzoom_scale = 1.0f;
                   }


          LelResetZoom();
          LelResetTouch();

          Lel_UltraSound_SetFan (-1) ; //re-generate buffer


                    //try to restore zooming
                    if ( orgzoom_scale > 1.0f )
                     {
                        int px, py;
                        px = orgzoom_cx, py = orgzoom_cy;
                        CalculateBufferPosition (&px, &py);
                        ZoomLeft = px - (int)(DisplayBufferWidth / 2 / orgzoom_scale);
                        ZoomUp =  py - (int)(DisplayBufferHeight / 2 / orgzoom_scale);
                        ZoomScale = orgzoom_scale;

                         LimitZoomingInRange ();

                     }

          CalculateRulerDisplayPosition (); //re-pose rulers

          UltraSoundToDisplay ();
      }

     int LelGetHistoryMax (void) { return HISTORY_SIZE - 1; }
     int LelReadFromHistory (int index)  //index: 0... LelGetHistoryMax ()
      {
         int idx = HistoryNext  - 1 -  ( (HISTORY_SIZE - 1) - index);
         if ( idx < 0 )
           idx += HISTORY_SIZE;

         if ( idx < 0 )
           idx = 0;
         else if ( idx >= HISTORY_SIZE )
           idx = HISTORY_SIZE - 1;

         memcpy (BufB, HistoryB [idx], sizeof (BufB));
         memcpy (BufC, HistoryC [idx], sizeof (BufC));
         memcpy (BufP, HistoryP [idx], sizeof (BufP));
         BufFlag = HistoryFlag [idx];

         UltraSoundToDisplay ();

         return 1;
      }


     static int Lel_UltraSound_SetFan (double ADegree, double ASkipRadius, double ADisplayRadius)
      {
          int picw, pich;

          if ( ADegree == -1 )
            ADegree = FanDegree;

          FanDegree = ADegree; FanRadiusStart = ASkipRadius; FanRadiusEnd = ASkipRadius + ADisplayRadius;
          if (  FanRadiusEnd < FanRadiusStart + 1 )
               FanRadiusEnd = FanRadiusStart + 1;

          //decide display buffer size
           picw = ViewWidth/ViewResolutionDiv;
           pich = ViewHeight/ViewResolutionDiv;

          //release old display buffer
          if ( DisplayBuffer != NULL )
             delete DisplayBuffer;
           DisplayBufferWidth = DisplayBufferHeight = 0;
           DisplayBufferShortEdge = 0;
           if (DisplayMappingX != NULL )
              delete DisplayMappingX;
            DisplayMappingX = DisplayMappingY = NULL;

           //create display buffer
           int ksize = picw * pich * 4;
           if ( (DisplayBuffer = new unsigned char [ksize]) != NULL  )
           {
               DisplayBufferWidth = picw;
               DisplayBufferHeight = pich;

               if ( picw < pich )
                 DisplayBufferShortEdge = picw;
               else
                 DisplayBufferShortEdge = pich;

               memset (DisplayBuffer, 0, ksize);
           }
          else
             return 0;

          //create mapping from display buffer to data
          ksize = picw * pich;
          if ( (DisplayMappingX = new int [ksize * 2]) != NULL )
           {
               DisplayMappingY = DisplayMappingX + ksize;

               //clear all mapping to (-1,-1) (no data)
               int *pclear;
               int iclear;
               for ( pclear = DisplayMappingX, iclear = ksize*2;  iclear > 0; iclear --, pclear ++ )
                 *pclear = -1;

               int orgw, orgh;   //size of original data
               int winL, winU, winW, winH; //display area
               int idx;
               int y, x;
               double px, py;
               int rx, ry;
               int prjL, prjU, prjR, prjD;
               int mmodedataw, mmodedatah;
               int mmodeL, mmodeU, mmodeR, mmodeD;
               int prjlen;
               double yxratio;
               int wingap = pich / 60;

               if ( MMode )
                {
                   //calculating mapping to MMode Data
                   mmodedataw = BufM_X, mmodedatah = BufB_Y_DISPLAY;   //size of original Ultrasound image

                   mmodeL = 0, mmodeR = picw;
                   mmodeU = pich/2 + wingap, mmodeD = pich;

                      for ( y = 0, idx = 0; y < pich; y ++ )
                        for ( x = 0; x < picw; x ++, idx ++ )
                         {
                            if (  mmodeL <= x && x < mmodeR && mmodeU <= y && y < mmodeD )
                             {
                                 rx = (int) ( mmodedataw * ( x - mmodeL) / (mmodeR - mmodeL));
                                 ry = (int) ( mmodedatah * ( y - mmodeU) / (mmodeD - mmodeU) );

                                 if ( rx < 0 )
                                    rx = 0;
                                 else if ( rx >= mmodedataw )
                                    rx = mmodedataw -1;
                                 if ( ry < 0 )
                                    ry = 0;
                                 else if ( ry >= mmodedatah )
                                    ry = mmodedatah - 1;

                                 DisplayMappingX [idx] = rx + (Cut_Data_Div * DataType_MMode);
                                 DisplayMappingY [idx] = ry;
                             }
                         }
                }  //if MMode
               else
                 mmodedataw = mmodedatah = mmodeL = mmodeU = mmodeR = mmodeD = 0;

               LastFan_mmodedataw = mmodedataw, LastFan_mmodedatah = mmodedatah;
               LastFan_mmodeL = mmodeL, LastFan_mmodeU = mmodeU;
               LastFan_mmodeR = mmodeR, LastFan_mmodeD = mmodeD;

               //calculating mapping to UltraSound Data
               //.....................................................................
               if ( !MMode )
                  winL = 0, winU = 0, winW = picw, winH = pich;
               else
                  winL = 0, winU = 0, winW = picw, winH = pich/2-wingap;

               orgw = BufB_X, orgh = BufB_Y_DISPLAY;   //size of original Ultrasound image

               if ( FanDegree <= 0 ) //rectangle: project rectangle onto display buffer
                {
                      yxratio =  (63.0 / (0.3 * 128)) / ( ((float) orgh) / ((float)orgw)) ;
                      //double kspace;

                      if (  ((double) winW) / winH >  ((double) orgw) / (orgh * yxratio) )
                       {
                          //screen width > image: keep space at left and right

                          prjlen = (int) (  winH * ((double) orgw) / (orgh * yxratio) );  //projected width
                          prjL = winL + (winW - prjlen)  / 2;
                          prjR = prjL + prjlen;
                          prjU = winU + 0;
                          prjD = prjU + winH;
                       }
                      else  //image width > screen: keep space at up and down
                       {
                          prjlen = (int)  ( winW * (orgh * yxratio) / ((double) orgw) );   //project height
                          prjU = winU + (winH - prjlen) / 2;
                          prjD = prjU + prjlen;
                          prjL = winL + 0;
                          prjR = prjL + winW;
                       }

                      for ( y = 0, idx = 0; y < pich; y ++ )
                        for ( x = 0; x < picw; x ++, idx ++ )
                         {
                            if (  prjL <= x && x < prjR && prjU <= y && y < prjD )
                             {
                                 rx = (int) ( orgw * ( x - prjL) / (prjR - prjL));
                                 ry = (int) ( orgh * ( y - prjU) / (prjD - prjU) );

                                 if ( rx < 0 )
                                    rx = 0;
                                 else if ( rx >= orgw )
                                    rx = orgw -1;
                                 if ( ry < 0 )
                                    ry = 0;
                                 else if ( ry >= orgh )
                                    ry = orgh - 1;

                                 DisplayMappingX [idx] = rx;
                                 DisplayMappingY [idx] = ry;
                             }
                         }

                      LastFan_prjL = prjL, LastFan_prjR = prjR, LastFan_prjU = prjU, LastFan_prjD = prjD;

                }
               else  //FanDegree > 0, convex
                {
                     double xscale, yscale, currscale;
                     double fan2rad;
                     //double xbias;
                     double ybias;
                     double prjw, prjh;

                     //NOTE: FanDegree may be zero for rectangle projection

                     fan2rad = (FanDegree/2)  * 3.1415926 / 180;

                     prjw = (2 * FanRadiusEnd * sin ( fan2rad) );
                     prjh = (FanRadiusEnd - FanRadiusStart * cos  (fan2rad));

                     if ( prjw < 10 )
                       prjw = 10;
                     if ( prjh < 10 )
                       prjh = 10;
                     if (fan2rad < 0.01 )
                       fan2rad = 0.01;

                     xscale = winW / prjw;
                     yscale = winH / prjh;
                     if ( xscale > yscale )
                      {
                        //xbias = winW * ((1 - yscale / xscale)/ 2) ;
                        ybias = winU + 0;
                        xscale = yscale;
                        currscale = yscale;
                      }
                     else
                      {
                        //xbias = 0;
                        ybias = winU + winH * ((1- xscale / yscale) / 2 );
                        yscale = xscale;
                        currscale = xscale;
                      }

                      int centerx = winL + winW / 2;
                      double yadd = FanRadiusStart * cos  (fan2rad);
                      double currrad, currdis;

                      for ( y = 0, idx = 0; y < pich; y ++ )
                        for ( x = 0; x < picw; x ++, idx ++ )
                         {
                            px = (x - centerx) / currscale;
                            py = (y - ybias) / currscale + yadd;

                            currrad = atan2 (py, px) - 3.1415926 / 2;
                            currdis = sqrt (px * px + py * py);

                            if ( -fan2rad <= currrad && currrad <= fan2rad &&
                                 FanRadiusStart <= currdis && currdis <= FanRadiusEnd )
                              {
                                 rx = (int) ( orgw * (fan2rad  - currrad) / (fan2rad * 2));
                                 ry = (int) ( orgh * (currdis - FanRadiusStart) / (FanRadiusEnd - FanRadiusStart) );
                                 if ( rx < 0 )
                                    rx = 0;
                                 else if ( rx >= orgw )
                                    rx = orgw -1;
                                 if ( ry < 0 )
                                    ry = 0;
                                 else if ( ry >= orgh )
                                    ry = orgh - 1;

                                 DisplayMappingX [idx] = rx;
                                 DisplayMappingY [idx] = ry;
                              }
                         }  //for x

                       LastFan_centerx = centerx;
                       LastFan_currscale = currscale, LastFan_ybias = ybias, LastFan_yadd = yadd;
                       LastFan_fan2rad = fan2rad;


                }  //else: FanDegree > 0

                LastFan_orgw = orgw, LastFan_orgh = orgh;
                LastFan_Degree = FanDegree;
                LastFan_FanRadiusStart = FanRadiusStart, LastFan_FanRadiusEnd = FanRadiusEnd;

           } //if DisplayMappingX [] allocated successfully
          else
            return 0;

          return 1;
      }

     static float CalculateCurrentDisplayScale (void)
      {
          if ( DisplayBufferWidth > 0 && DisplayBufferHeight > 0 )
            {
                float  displayscale = ((float)ViewWidth)  / DisplayBufferWidth;
                 if  ( displayscale > ((float)ViewHeight) / DisplayBufferHeight )
                    displayscale = ((float)ViewHeight) / DisplayBufferHeight;
                 if  ( displayscale < 0.001 )
                    displayscale = (float)0.001;
                 return displayscale;
            }
          else
             return 0.0001f;
      }

     static int CalculateBufferPosition (int *x, int *y)
      {
         /*
            reverse calculation of Lel_UltraSound_SetFan ()

            add 0.5 to avoid data loss accumulation
          */

         int kret = 0;
         int rx, ry;
         int datatype;
         datatype = Cut_Data_Type (*x);
         rx = Cut_Data_Pos(*x);
         ry = *y;

         if ( datatype == DataType_MMode )
          {
             if ( LastFan_mmodedataw <= 0 || LastFan_mmodedatah <= 0 )
                return 0;

             *x = (int) ( rx * (double)(LastFan_mmodeR - LastFan_mmodeL) / (double)LastFan_mmodedataw + LastFan_mmodeL + 0.5);
             *y = (int) ( ry * (double)(LastFan_mmodeD - LastFan_mmodeU) / (double)LastFan_mmodedatah + LastFan_mmodeU + 0.5 );

             return 1;
          } //if DataType_MMode


         if ( LastFan_orgw <= 0 || LastFan_orgh <= 0 )
            return 0;

         if ( LastFan_Degree == 0 )
          {
             *x = (int) ( rx * (double)(LastFan_prjR - LastFan_prjL) / (double)LastFan_orgw + LastFan_prjL + 0.5);
             *y = (int) ( ry * (double)(LastFan_prjD - LastFan_prjU) / (double)LastFan_orgh + LastFan_prjU + 0.5 );
             kret = 1;
          }
         else
          {
             double currrad, currdis, px, py;

             currrad = LastFan_fan2rad - rx * (LastFan_fan2rad * 2) / LastFan_orgw;
             currdis = ry * (LastFan_FanRadiusEnd - LastFan_FanRadiusStart) / LastFan_orgh + LastFan_FanRadiusStart;

             px = currdis * cos (currrad + 3.1415926 / 2);
             py = currdis * sin (currrad + 3.1415926 / 2);

             *x = (int) (px * LastFan_currscale + LastFan_centerx + 0.5);
             *y = (int) ((py - LastFan_yadd) * LastFan_currscale + LastFan_ybias + 0.5);
             kret = 1;
          }

          if ( kret )
           {
             if ( *x < 0 )
               *x = 0;
             if ( *y < 0 )
               *y = 0;
             if ( *x >= DisplayBufferWidth )
               *x = DisplayBufferWidth - 1;
             if ( *y > DisplayBufferHeight )
               *y = DisplayBufferHeight - 1;
           }

          return kret;
      }

    static int CalculateDisplayPosition (int *x, int *y)
     {
        if ( DisplayBufferWidth > 0 && DisplayBufferHeight > 0 &&
            CalculateBufferPosition (x, y))
        {
            float  displayscale = CalculateCurrentDisplayScale ();

            //apply zooming
            *x = (int) (( (*x) - ZoomLeft) * ZoomScale * displayscale);
            *y = (int) (((*y) - ZoomUp) * ZoomScale * displayscale);

            return 1;
        }
        else
         {
            *x = *y = 0;
            return 0;
         }
     }



    int LelGetDisplayBufferWidth (void) { return DisplayBufferWidth; }
    int LelGetDisplayBufferHeight (void) { return DisplayBufferHeight; }
    int LelGetDisplayBufferSize (void) { return DisplayBufferWidth * 4 * DisplayBufferHeight; }

    unsigned char *LelGetDisplayBuffer (void)
      {
           //FPS statistic
           FPS_CurrAccessCount++;
           unsigned long currtime = (unsigned long) time (NULL);
           if ( currtime != FPS_LastTime )
            {
               FPS_LastTime = currtime;
               FPS_LastDataCount = FPS_CurrDataCount;  FPS_CurrDataCount = 0;
               FPS_LastAccessCount = FPS_CurrAccessCount;  FPS_CurrAccessCount = 0;
            }

           return DisplayBuffer;
      }


    char *LelGetStatusLine (void)
    {
        return StatusLine;
    }


    int LelGetLastDataFPS (void) { return FPS_LastDataCount; }
    int LelGetLastAccessFPS (void) { return FPS_LastAccessCount; }


         static int  PointInDataRange (int x, int y)
          {
              if ( x < 0 || y < 0 ||  x >= DisplayBufferWidth || y >= DisplayBufferHeight )
                 return 0;
              if (  Out_Of_Data_Area (x, y))
                 return 0;
              return 1;
          }

         static int StartNewMark (void)
          {
              int datax, datay;
              datax = RulerNewStartDataX, datay = RulerNewStartDataY;

              if (datax < 0 || datay <0 )
                 return 0;

                             if ( MarkMode == LelMarkMode_Line && RulerNewStartDataType == DataType_UltraSound )
                              {
                                 //start a line
                                 int i;
                                 for (  i = RulerIndex_Line_Start; i < RulerIndex_Line_End - 2; i +=2 )
                                    if ( RulerPointX [i] < 0 &&  RulerPointY [i] < 0 &&
                                         RulerPointX [i+1] < 0 &&  RulerPointY [i+1] < 0 )
                                        break;

                                 RulerPointX [i] = datax;
                                 RulerPointY [i] = datay;
                                 RulerPointX [i+1] = datax;
                                 RulerPointY [i+1] = datay;
                                 CalculateRulerDisplayPosition (i, i +1);

                                 RulerPointHolding = i +1;
                              }
                             else if ( MarkMode == LelMarkMode_Ellipse  && RulerNewStartDataType == DataType_UltraSound )
                              {
                                 //start an ellipse
                                 int kd = RulerPointTouchRadius;
                                 int cx, cy;

                                 cx = datax;  cy = datay;
                                 if ( cx < kd )
                                   cx = kd;
                                 if ( cy < kd )
                                   cy = kd;
                                 if ( cx >= DataBufferWidth - kd )
                                    cx = DataBufferWidth - kd - 1;
                                 if ( cy >= DataBufferHeight - kd )
                                    cy = DataBufferHeight - kd - 1;

                                 RulerPointX [RulerIndex_Ellipse_Center] = cx;
                                 RulerPointY [RulerIndex_Ellipse_Center] = cy;

                                 RulerPointX [RulerIndex_Ellipse_Left] = cx - kd;
                                 RulerPointY [RulerIndex_Ellipse_Left] = cy;
                                 RulerPointX [RulerIndex_Ellipse_Right] = cx + kd;
                                 RulerPointY [RulerIndex_Ellipse_Right] = cy;
                                 RulerPointX [RulerIndex_Ellipse_Up] = cx;
                                 RulerPointY [RulerIndex_Ellipse_Up] = cy - kd;
                                 RulerPointX [RulerIndex_Ellipse_Down] = cx;
                                 RulerPointY [RulerIndex_Ellipse_Down] = cy + kd;
                                 CalculateRulerDisplayPosition (RulerIndex_Ellipse_Center, RulerIndex_Ellipse_Down);

                                 RulerPointHolding = RulerIndex_Ellipse_Right;
                              }

                             else if ( MarkMode == LelMarkMode_Annotate )
                               {
                                  //start an annotation
                                  int i;
                                  for ( i = Annotation_StartIndexForUser; i <= LelAnnotation_Max; i ++ )
                                    if ( AnnotationText [i][0] == 0 )
                                      {
                                          LelSetAnnotation (i, (char*)"***");
                                          AnnotationHolding = LastAnnotationHolding = i;
                                          break;
                                      }
                               }

              return 1;
          }

    int LelTouch (int act, int total, int x, int y, int x2, int y2)
     {
         /*
               CAUTION: x, y, x2, y2 may be out of display buffer range
          */

         if ( DisplayBufferWidth < 1 || DisplayBufferHeight < 1 ||
              ViewWidth < 1 || ViewHeight < 1 )
            return 0;

         int bufw = DisplayBufferWidth;
         int bufh = DisplayBufferHeight;
         float  displayscale = ((float)ViewWidth)  / bufw;
         int  csrx, csry;  //position of (x,y) in display buffer
         int  datatype, datax, datax_all, datay;  //position of (x,y) in data, -1 means not in data area

          if  ( displayscale > ((float)ViewHeight) / bufh )
             displayscale = ((float)ViewHeight) / bufh;
          if  ( displayscale < 0.001 )
             displayscale = (float)0.001;

          //WARNING: (csrx, csry) may not in display buffer range
          csrx = ZoomLeft + (int) (((float)x) / ZoomScale / displayscale);
          csry = ZoomUp + (int) (((float)y) / ZoomScale / displayscale);

          if ( csrx >= 0 && csry >= 0 && csrx < DisplayBufferWidth && csry < DisplayBufferHeight )
           {
              datatype = Locate_Data_Type (csrx, csry);
              datax_all = Locate_Data_X_ALL (csrx, csry);
              datax = Locate_Data_X (csrx, csry);
              datay = Locate_Data_Y (csrx, csry);
           }
          else
           {
             datax = datax_all = datay = -1;
             datatype = -1;
           }

         if ( act == LelTouch_Down ) //start touch
          {
              if ( total >= 1 )
               {
                  TouchX = TouchStartX = x;
                  TouchY = TouchStartY = y;
                  TouchPhase = 1;

                  TouchStartScale = ZoomScale; TouchStartLeft = ZoomLeft; TouchStartUp = ZoomUp;

                  RulerPointHolding = 0;
                  RulerNewStartDataX = -1, RulerNewStartDataY = -1;
                  RulerNewStartDataType = -1;
                  AnnotationHolding = 0;
                  LastAnnotationHolding = 0;

                  if ( MarkMode == LelMarkMode_None || total >= 2 )
                   {
                       CurrTouchMode = TouchMode_Zoom;

                       if ( total == 1 && MMode )
                        {
                           if ( datax >= 0 && datatype == DataType_UltraSound )
                             {
                                MModeScanPos = datax;
                                ClearMModeData ();
                                UltraSoundToDisplay();
                             }
                           CurrTouchMode = TouchMode_MMode;
                        }
                    }
                   else
                    {
                       CurrTouchMode = TouchMode_Mark;

                       //picking any points?
                       int i;
                       int minidx;
                       int mindisp, disp;
                       int maxdisp;

                       minidx = -1;  mindisp = 0;
                       maxdisp = RulerPointTouchRadius * RulerPointTouchRadius;

                       //holding ruler point?
                       for ( i = 0; i <= RulerPointMax; i ++ )
                          if ( RulerDisplayX [i] >= 0 && RulerDisplayY [i] >= 0 )
                           {
                               disp = (csrx - RulerDisplayX [i]) * (csrx - RulerDisplayX [i]) +
                                  (csry - RulerDisplayY [i]) * (csry - RulerDisplayY [i]);
                               if ( disp > maxdisp )
                                  continue;
                               if ( minidx == -1 || disp < mindisp )
                                  { minidx = i;  mindisp = disp; }
                           }

                       if ( minidx >= 0 )  //picking some ruler point
                           RulerPointHolding = minidx;
                       else //not holding ruler point
                        {

                           //holding annotation?
                            maxdisp = RulerPointTouchRadius;
                            for ( i = LelAnnotation_Max; i >= 1; i-- )
                              if ( AnnotationText [i][0] && AnnotationW [i] > 0 && AnnotationH [i] > 0 )
                                {
                                   int cx, cy, dx, dy;
                                   cx = AnnotationL [i] + AnnotationW [i] / 2;
                                   cy = AnnotationU [i] + AnnotationH [i] / 2;
                                   if ( (dx = x - cx) < 0 )
                                     dx = -dx;
                                   if ( (dy = y - cy) < 0 )
                                     dy = -dy;
                                   dx -= AnnotationW [i] / 2;
                                   if ( dx < 0 )
                                     dx = 0;
                                   dy -= AnnotationH [i] / 2;
                                   if ( dy < 0 )
                                     dy = 0;

                                   disp = dx + dy;
                                   if ( disp > maxdisp )
                                      continue;
                                   if ( minidx == -1 || disp < mindisp )
                                      { minidx = i;  mindisp = disp; }
                                }

                              if ( minidx >= 0 )  //picking some annotation
                               {
                                  AnnotationHolding = minidx;
                                  if ( minidx >= Annotation_StartIndexForUser )
                                      LastAnnotationHolding = minidx;
                               }

                              else  //not holding annotation or ruler point
                               {
                                    if ( datax >= 0 && datay >= 0 )
                                      {
                                          //no point is picked, start a new ruler later (if dragged sufficient distance)
                                          RulerNewStartDataX = datax, RulerNewStartDataY = datay;
                                          RulerNewStartDataType = datatype;
                                      }
                                     else  //out of ultra sound image range
                                      {
                                      }

                               }   //not holding annotation or ruler point

                        }  //not holding ruler point

                       UltraSoundToDisplay();

                    }  //if MarkMode
               }
              if ( total >= 2 )
               {
                   TouchX2 = TouchStartX2 = x2;
                   TouchY2 = TouchStartY2 = y2;
                   TouchPhase = 2;
               }
          }

         else if ( act == LelTouch_Move ) //touch move
          {

              if ( total >= 1 )
               {  TouchX =  x;  TouchY = y; }
              if ( total >= 2 )
               {  TouchX2 =  x2;  TouchY2 = y2; }

              if ( TouchPhase == 1 )  //pan
               {
                  int dx = TouchX - TouchStartX;
                  int dy = TouchY - TouchStartY;

                  if ( CurrTouchMode == TouchMode_Zoom )
                   {
                       ZoomLeft = TouchStartLeft - (int) (dx / (ZoomScale * displayscale));
                       ZoomUp = TouchStartUp - (int) (dy / (ZoomScale * displayscale));
                   }   //if TouchMode_Zoom
                  else if ( CurrTouchMode == TouchMode_Mark )
                   {
                       if (  PointInDataRange (csrx, csry) )
                        {
                            //Try To Start a New Ruler if dragged sufficient range, not starting double touch
                            if ( RulerPointHolding == 0 &&  AnnotationHolding == 0 &&
                                  RulerNewStartDataX >= 0 && RulerNewStartDataY >= 0 &&
                                  dx * dx + dy * dy > 10 * 10 )
                                 StartNewMark ();

                             if ( RulerPointHolding > 0 && RulerPointHolding <= RulerPointMax &&
                                  datatype == DataType_UltraSound )
                              {
                                  int i;
                                  int orgx, orgy;
                                  float orglen1 = -1, orglen2 = -1;
                                  int nx, ny;

                                  //Holding Line
                                  if ( RulerPointHolding >= RulerIndex_Line_Start &&
                                            RulerPointHolding <= RulerIndex_Line_End )
                                   {
                                       RulerPointX [RulerPointHolding] = datax;
                                       RulerPointY [RulerPointHolding] = datay;
                                       CalculateRulerDisplayPosition (RulerPointHolding, RulerPointHolding);

                                       //display length
                                       char str [200];
                                       int i;
                                       float dis;
                                       int aidx;
                                       i = RulerIndex_Line_Start +
                                               (RulerPointHolding - RulerIndex_Line_Start) / 2 *2;
                                       if ( (dis = GetRulerRealLength (i, i +1)) >= 0 )
                                        {
                                            char *vstr = (char*)"";

                                            if ( RulerVolumeMultiplier > 0 &&  i == RulerIndex_Line_Start )
                                              {
                                                 vstr = (char*) "[V]";
                                                 UpdateEllipseRulerMark ();
                                              }

                                            sprintf (str, "%s%.1fcm", vstr, dis/10);
                                            // Cliff
                                            FinalDis = dis/10;

                                            aidx = Annotation_StartIndexForRulerLine + (i - RulerIndex_Line_Start)/ 2;
                                            LelSetAnnotation (aidx, str);
                                            PutAnnotationAtData ( aidx,  (RulerPointX [i] + RulerPointX [i+1]) / 2,
                                                                 (RulerPointY [i] + RulerPointY [i+1]) / 2);
                                        }

                                   }

                                  //Holding Ellipse
                                  else if ( RulerPointHolding >= RulerIndex_Ellipse_Center &&
                                            RulerPointHolding <= RulerIndex_Ellipse_Down &&
                                             datatype == DataType_UltraSound )
                                   {
                                       if ( datax >= 0 && datay >= 0 )
                                        {
                                           //move RulerDisplayX/Y [] first, and then move RulerPointX/Y []

                                           orglen1 = GetRulerBufferLength (RulerIndex_Ellipse_Left, RulerIndex_Ellipse_Right);
                                           orglen2 = GetRulerBufferLength (RulerIndex_Ellipse_Up, RulerIndex_Ellipse_Down);

                                           orgx = RulerDisplayX [RulerPointHolding];
                                           orgy = RulerDisplayY [RulerPointHolding];
                                           dx = csrx - orgx;
                                           dy = csry - orgy;
                                           RulerDisplayX [RulerPointHolding] = csrx;
                                           RulerDisplayY [RulerPointHolding] = csry;

                                           //move other points if necessary
                                           if ( RulerPointHolding == RulerIndex_Ellipse_Center )
                                            {
                                               //move entire ellipse if possible
                                               for ( i = RulerIndex_Ellipse_Left; i <= RulerIndex_Ellipse_Down; i ++ )
                                                 {
                                                    nx = RulerDisplayX [i] + dx;
                                                    ny = RulerDisplayY [i] + dy;
                                                    if ( nx < 0 || ny < 0 || nx >= DisplayBufferWidth || ny >= DisplayBufferHeight ||
                                                         !In_Data_UltraSound (nx, ny) )
                                                        break;  //illegal move
                                                 }
                                               if ( i > RulerIndex_Ellipse_Down ) //all point ok
                                                 {
                                                     for ( i = RulerIndex_Ellipse_Left; i <= RulerIndex_Ellipse_Down; i ++ )
                                                       {
                                                          RulerDisplayX [i] += dx;
                                                          RulerDisplayY [i] += dy;
                                                       }
                                                 }
                                               else //restore move
                                                 {
                                                    RulerDisplayX [RulerPointHolding] -= dx;
                                                    RulerDisplayY [RulerPointHolding] -= dy;
                                                 }
                                            }
                                           else if ( RulerPointHolding == RulerIndex_Ellipse_Left ||
                                                     RulerPointHolding == RulerIndex_Ellipse_Right )
                                            {
                                                RulerDisplayX [RulerIndex_Ellipse_Center] =
                                                  (RulerDisplayX [RulerIndex_Ellipse_Left] + RulerDisplayX [RulerIndex_Ellipse_Right]) / 2;
                                                RulerDisplayY [RulerIndex_Ellipse_Center] =
                                                  (RulerDisplayY [RulerIndex_Ellipse_Left] + RulerDisplayY [RulerIndex_Ellipse_Right]) / 2;

                                                   int mx = RulerDisplayX [RulerIndex_Ellipse_Right] - RulerDisplayX [RulerIndex_Ellipse_Left];
                                                   int my = RulerDisplayY [RulerIndex_Ellipse_Right] - RulerDisplayY [RulerIndex_Ellipse_Left];

                                                   if ( orglen1 > 0 && orglen2 >=  0 )
                                                    {
                                                       if ( orglen2 < RulerPointTouchRadius  && orglen1 > RulerPointTouchRadius )
                                                          orglen2 = (float) RulerPointTouchRadius;

                                                       //add 0.5 to avoid data loss accumulation
                                                       mx = (int) (mx * (orglen2 / orglen1 / 2) + 0.5);
                                                       my = (int) (my * (orglen2 / orglen1 / 2) + 0.5);
                                                    }

                                                   RulerDisplayX [RulerIndex_Ellipse_Up] = RulerDisplayX [RulerIndex_Ellipse_Center] + my;
                                                   RulerDisplayY [RulerIndex_Ellipse_Up] = RulerDisplayY [RulerIndex_Ellipse_Center] - mx;
                                                   RulerDisplayX [RulerIndex_Ellipse_Down] = RulerDisplayX [RulerIndex_Ellipse_Center] - my;
                                                   RulerDisplayY [RulerIndex_Ellipse_Down] = RulerDisplayY [RulerIndex_Ellipse_Center] + mx;


                                            }
                                           else if ( RulerPointHolding == RulerIndex_Ellipse_Up || RulerPointHolding == RulerIndex_Ellipse_Down )
                                            {
                                               RulerDisplayX [RulerPointHolding] = csrx;
                                               RulerDisplayY [RulerPointHolding] = csry;

                                               float newlen2 = GetRulerBufferLength (RulerPointHolding, RulerIndex_Ellipse_Center) *2;

                                                   int mx = RulerDisplayX [RulerIndex_Ellipse_Right] - RulerDisplayX [RulerIndex_Ellipse_Left];
                                                   int my = RulerDisplayY [RulerIndex_Ellipse_Right] - RulerDisplayY [RulerIndex_Ellipse_Left];

                                                   if ( orglen1 > 0 && newlen2 >= 0 )
                                                    {
                                                       if ( newlen2 < RulerPointTouchRadius  && orglen1 > RulerPointTouchRadius )
                                                          newlen2 = (float) RulerPointTouchRadius;

                                                       //add 0.5 to avoid data loss accumulation
                                                       mx = (int) (mx * (newlen2 / orglen1 / 2) + 0.5);
                                                       my = (int) (my * (newlen2 / orglen1 / 2) + 0.5);
                                                    }

                                                   RulerDisplayX [RulerIndex_Ellipse_Up] = RulerDisplayX [RulerIndex_Ellipse_Center] + my;
                                                   RulerDisplayY [RulerIndex_Ellipse_Up] = RulerDisplayY [RulerIndex_Ellipse_Center] - mx;
                                                   RulerDisplayX [RulerIndex_Ellipse_Down] = RulerDisplayX [RulerIndex_Ellipse_Center] - my;
                                                   RulerDisplayY [RulerIndex_Ellipse_Down] = RulerDisplayY [RulerIndex_Ellipse_Center] + mx;
                                            }

                                          //calculate new postion
                                           int idx;
                                           for ( idx = RulerIndex_Ellipse_Center; idx <= RulerIndex_Ellipse_Down; idx ++ )
                                             {
                                                nx = RulerDisplayX [idx];
                                                ny = RulerDisplayY [idx];
                                                if(nx < 0 || ny < 0 || nx >= DisplayBufferWidth || ny >= DisplayBufferHeight ||
                                                    !In_Data_UltraSound (nx, ny) )
                                                {
                                                    break;
                                                }
                                             }

                                           if ( idx > RulerIndex_Ellipse_Down )   //all display points are legal, move
                                             {
                                                for ( idx = RulerIndex_Ellipse_Center; idx <= RulerIndex_Ellipse_Down; idx ++ )
                                                 {
                                                     nx = RulerDisplayX [idx];
                                                     ny = RulerDisplayY [idx];
                                                     RulerPointX [idx] = Locate_Data_X(nx, ny);
                                                     RulerPointY [idx] = Locate_Data_Y(nx, ny);
                                                 }
                                             }

                                          //To avoid error-accumulaition problem, do not reprojection back
                                          //CalculateRulerDisplayPosition (RulerIndex_Ellipse_Center, RulerIndex_Ellipse_Down);

                                           UpdateEllipseRulerMark ();


                                        } //if datax, datay >= 0

                                   } //else: holding ellipse

                                  //will be update at end
                                  //UltraSoundToDisplay();


                              } //if holding some ruler point and in data range

                            else if ( AnnotationHolding > 0 )
                             {
                                AnnotationDataX [AnnotationHolding] = datax_all;
                                AnnotationDataY [AnnotationHolding] = datay;
                             } //if holding annotation

                        }  //if cursor in data range

                   }   //if TouchMode_Mark
               }
              else  if ( TouchPhase == 2 )  //pan + zoom
               {
                   int dx = (TouchX + TouchX2)/2 - (TouchStartX + TouchStartX2) / 2;
                   int dy = (TouchY + TouchY2)/2  - (TouchStartY + TouchStartY2) / 2;
                   int panx, pany;
                   int nx = TouchX2 - TouchX;
                   int ny = TouchY2 - TouchY;
                   int ox = TouchStartX2 - TouchStartX;
                   int oy = TouchStartY2 - TouchStartY;

                        /* Panning
                           ------------------------ */
                       panx =  - (int) (dx / (ZoomScale * displayscale));
                       pany =  - (int) (dy / (ZoomScale * displayscale));


                        /* Zoom (Scale)
                           ------------------------ */
                        ZoomScale = (float)(TouchStartScale * sqrt (double(nx * nx + ny * ny)) /  sqrt ((double)(ox * ox + oy * oy)));
                        if  ( ZoomScale < 1.0 )
                            ZoomScale = 1.0;
                        else if  ( ZoomScale > 3.0 )
                           ZoomScale = 3.0;

                         //keep position of double-touch center
                         int cx = (TouchStartX + TouchStartX2) / 2;
                         int cy = (TouchStartY + TouchStartY2) / 2;
                         int datax = ZoomLeft + (int) (cx / (ZoomScale * displayscale));
                         int datay = ZoomUp + (int) (cy / (ZoomScale * displayscale));
                         int oldx = TouchStartLeft + (int) (cx / (TouchStartScale * displayscale)) + panx;
                         int oldy = TouchStartUp + (int) (cy / (TouchStartScale * displayscale)) + pany;
                         ZoomLeft += oldx - datax;
                         ZoomUp += oldy - datay;

               }

                      LimitZoomingInRange ();
                      UltraSoundToDisplay ();


          } //else if LelTouch_Move

         else if ( act ==  LelTouch_Up)  //end touch
          {
             if ( CurrTouchMode == TouchMode_Mark &&
                  RulerPointHolding > 0 && RulerPointHolding <= RulerPointMax )
               {
                  if (ClearRulerWithSmallLength ())
                     UltraSoundToDisplay();
               }

             LelResetTouch ();
          }


         return 1;
     }

    float LelGetZoomScale (void) { return ZoomScale; }
    int LelGetZoomLeft (void)    { return ZoomLeft; }
    int LelGetZoomUp (void)      { return ZoomUp; }
    


       static void CalculatePixelValueMapping (void)
        {
            int i;
            int vi;
            for ( i = 0; i <= 255; i ++ )
             {
                vi = (int) (i * PixelValueScale + PixelValueAdd + 0.5);
                if ( vi < 0 )
                  vi = 0;
                else if ( vi > 255 )
                  vi = 255;
                PixelValueMapping [i] = (unsigned char) (vi);
             }
        }
    void LelSetPixelValueAdd (float add)
      {
         PixelValueAdd = add;
         CalculatePixelValueMapping  ();
         UltraSoundToDisplay();
      }

    float LelGetPixelValueAdd (void) { return PixelValueAdd; }

    void LelSetPixelValueScale (float scale)
      {
         PixelValueScale = scale;
         CalculatePixelValueMapping  ();
         UltraSoundToDisplay();
      }
    
    float LelGetPixelValueScale (void) { return PixelValueScale; }
                               /*
                                     Brightness/Contrast modification
                                     Display pixel value (0...255) =  ultrasound data value(0...255) * PixelValueScale  + PixelValueAdd
                                     By default, PixelValueAdd = 0.0f, PixelValueScale = 1.0f
                                     value out of legal range (0-255) will be truncated
                                */
    void LelSetCustomPixelValueMapping (unsigned char *map)
     {
         memcpy (PixelValueMapping, map, sizeof (PixelValueMapping));
         UltraSoundToDisplay();
     }
    void LelSetCustomPixelValueMappingByIndex (int index, int value, int refresh)
     {
         if ( index >= 0 && index <= 255 )
          {
             PixelValueMapping [index] = (unsigned char) value;
             if (refresh )
                UltraSoundToDisplay();
           }
     }

    // Cliff 0112
    float LelGetFinalDis(void){return FinalDis;}


    void LelResetZoom (void)
             {
                   ZoomScale = 1.0;
                   ZoomLeft = 0;
                   ZoomUp = 0;
             }

     void LelResetTouch (void)
             {
                   CurrTouchMode = TouchMode_None;
                   TouchPhase = 0;
                   TouchX = TouchY = TouchX2 = TouchY2 = 0;
                   TouchStartX = TouchStartY = TouchStartX2 = TouchStartY2 = 0;
                   TouchStartScale = 1.0;
                   TouchStartLeft = TouchStartUp = 0;
             }

     void LelResetRuler (void)
             {
                 RulerPointHolding = 0;
                 RulerNewStartDataX = -1, RulerNewStartDataY = -1;

                 int i;
                 for ( i = 0; i <= RulerPointMax; i ++ )
                  {
                     RulerPointX [i] = -1;
                     RulerPointY [i] = -1;
                     RulerDisplayX [i] = -1;
                     RulerDisplayY [i] = -1;
                  }

                 for ( i = Annotation_StartIndexForRuler; i <= Annotation_EndIndexForRuler; i ++ )
                    LelSetAnnotation (i, (char*) "");
             }


    int LelGetMarkMode (void)
     {
        return MarkMode;
     }

    void LelSetMarkMode (int mode)
     {
        MarkMode = mode;
        UltraSoundToDisplay();
     }

    void LelClearAllRulers (void)
     {
         LelResetRuler ();
         UltraSoundToDisplay();
     }

    void LelSetRulerVolumeMeasurement (float multiplier)
     {
         RulerVolumeMultiplier = multiplier;
     }

    /*
    static char* GetRulerInfo (char *result)
     {
                float dis, dis2;
                int i;
                for ( i = RulerIndex_Line_Start; i <= RulerIndex_Line_End; i += 2 )
                  if ((dis = GetRulerRealLength (i, i +1)) >= 0 )
                        sprintf ( result + strlen (result), " L%d=%.1fcm",
                         (i -RulerIndex_Line_Start)/2+1,   dis/10);

                if ((dis = GetRulerRealLength (RulerIndex_Ellipse_Left, RulerIndex_Ellipse_Right)) >= 0 &&
                    (dis2 = GetRulerRealLength (RulerIndex_Ellipse_Up, RulerIndex_Ellipse_Down)) >= 0 )
                        sprintf ( result + strlen (result), " Ellipse=%.1f cm / %.1fcm (%.1f cm2)", dis/10, dis2/10,
                             (float) (3.1415926 *  dis * dis2 / 4/100));

        return result;
     }
    */

    static float GetRulerBufferLength (int idx1, int idx2)
     {
                if ( RulerDisplayX [idx1] >= 0 && RulerDisplayX [idx2] >= 0 &&
                     RulerDisplayX [idx1] < DisplayBufferWidth  && RulerDisplayX [idx2] < DisplayBufferWidth &&
                     RulerDisplayY [idx1] >= 0 && RulerDisplayY [idx2] >= 0 &&
                     RulerDisplayY [idx1] < DisplayBufferHeight  && RulerDisplayY [idx2] < DisplayBufferHeight )
                 {
                   int dx = RulerDisplayX [idx1] - RulerDisplayX [idx2];
                   int dy = RulerDisplayY [idx1] - RulerDisplayY [idx2];
                   return (float)  sqrt ( (float) (dx * dx + dy * dy) );
                 }

               return -1;
     }

    static float GetBufferRealLength (int x1, int y1, int x2, int y2)
     {
                     float dis;
                     if ( FanDegree > 0 )
                       dis = distConvex (x1, y1, x2, y2);
                     else
                      dis = distLinear (x1, y1, x2, y2);
                     return dis;
     }

    static float GetRulerRealLength (int idx1, int idx2)
     {
                if ( RulerPointX [idx1] >= 0 && RulerPointX [idx2] >= 0 &&
                     RulerPointY [idx1] >= 0 && RulerPointY [idx2] >= 0 )
                 {
                     return GetBufferRealLength (RulerPointX [idx1], RulerPointY [idx1],
                                         RulerPointX [idx2], RulerPointY [idx2]);
                 }

               return -1;
     }
    // get dis
    
    static void CalculateRulerDisplayPosition (int start, int end)
     {
        int i;

        if ( start < 0 )
          start = 1;
        if ( end < 0 )
          end = RulerPointMax;

        for ( i = start; i <= end; i ++ )
         {
            RulerDisplayX [i] = RulerPointX [i];
            RulerDisplayY [i] = RulerPointY [i];
            if ( RulerDisplayX [i] >= 0 && RulerDisplayY [i] >= 0 )
              CalculateBufferPosition (RulerDisplayX + i, RulerDisplayY + i );
         }
     }




    static int ClearRulerWithSmallLength (void)
            {
                 int i, aidx;
                 int kret = 0;
                 float blen;
                 for ( i = RulerIndex_Line_Start; i <= RulerIndex_Line_End; i += 2 )
                   if ( RulerPointX [i] >= 0 && RulerPointX [i+1] >= 0 )
                     {
                        blen = GetRulerBufferLength (i, i +1) * ZoomScale * CalculateCurrentDisplayScale ();
                        if ( blen == 0 ||  ( i > RulerIndex_Line_Start && blen <= RulerPointDisplayRadius * 2) )
                         {
                            RulerPointX [i] =  RulerPointY [i] = -1;
                            RulerDisplayX [i] = RulerDisplayY [i] = -1;

                            RulerPointX [i+1] =  RulerPointY [i+1] = -1;
                            RulerDisplayX [i+1] = RulerDisplayY [i+1] = -1;

                            aidx = Annotation_StartIndexForRulerLine + (i - RulerIndex_Line_Start)/ 2;
                            LelSetAnnotation (aidx, (char*)"");
                            kret ++;
                         }
                     }

                   return kret;
            }

    static void UpdateEllipseRulerMark (void)
            {
                                       char str [200];
                                       float dis, dis2;
                                       int aidx;
                                       if ((dis = GetRulerRealLength (RulerIndex_Ellipse_Left, RulerIndex_Ellipse_Right)) >= 0 &&
                                            (dis2 = GetRulerRealLength (RulerIndex_Ellipse_Up, RulerIndex_Ellipse_Down)) >= 0 )
                                        {
                                            sprintf (str, "%.2fcm2", (float) (3.1415926 *  dis * dis2 / 4/100));

                                            float dis3;
                                            if ( RulerVolumeMultiplier > 0 && RulerPointX [RulerIndex_Line_Start] >= 0  &&
                                                (dis3 = GetRulerRealLength (RulerIndex_Line_Start, RulerIndex_Line_Start+1)) > 0 )
                                                 sprintf (str + strlen (str), " [V %.2fcm3]",  RulerVolumeMultiplier * dis * dis2 * dis3 / 8/1000);

                                            aidx = Annotation_StartIndexForRulerEllipse;
                                            LelSetAnnotation (aidx, str);
                                            PutAnnotationAtData ( aidx,  RulerPointX [RulerIndex_Ellipse_Center],
                                                                 RulerPointY [RulerIndex_Ellipse_Center]);
                                        }


            }


    void LelResetAnnotation (void)
             {
                memset (AnnotationText, 0, sizeof (AnnotationText));
                memset (AnnotationDataX, 0, sizeof (AnnotationDataX));
                memset (AnnotationDataY, 0, sizeof (AnnotationDataY));
                memset (AnnotationL, 0, sizeof (AnnotationL));
                memset (AnnotationU, 0, sizeof (AnnotationU));
                memset (AnnotationW, 0, sizeof (AnnotationW));
                memset (AnnotationH, 0, sizeof (AnnotationH));
                AnnotationHolding = 0;
                LastAnnotationHolding = 0;
             }

    int LelSetAnnotation (int index, char *text)
     {
        int i;
        if ( index == 0 ) //add new entry
         {
            for ( i = Annotation_StartIndexForUser; i <= LelAnnotation_Max; i ++ )
              if ( !AnnotationText [i][0] )
                break;

             if ( i > LelAnnotation_Max )
                return 0; //table full

             //reset entry
             AnnotationDataX [i] = AnnotationDataY [i] = 0;
             AnnotationL [i] = AnnotationU [i] =  AnnotationW [i] = AnnotationH [i] = 0;

             index = i;
         }

         if ( text != NULL && strlen (text) <= LelAnnotation_MaxLen )
          {
            strcpy (AnnotationText [index], text);
            if ( *text == 0 ) //clear
             {
                 AnnotationW [index] = 0;
                 AnnotationH [index] = 0;
             }
          }

         return index;
     }

    int LelPutAnnotation (int index, int left, int up, int width, int height, int move)
     {
        if ( index >= 1 && index <= LelAnnotation_Max )
         {
            if ( move )
             {
                int cx, cy, dx, dy;
                float  displayscale = CalculateCurrentDisplayScale ();

                cx = left + width / 2;
                cy = up + height / 2;

                //convert display postion to buffer position
                cx = (int) (ZoomLeft + cx / ZoomScale / displayscale);
                cy = (int) (ZoomUp + cy / ZoomScale / displayscale);

                //convert buffer position to data position
                if ( cx < 0 )
                  cx = 0;
                else if ( cx >= DisplayBufferWidth )
                  cx = DisplayBufferWidth - 1;
                if (cy < 0 )
                  cy = 0;
                else if ( cy >= DisplayBufferHeight )
                  cy = DisplayBufferHeight - 1;

                 if ( In_Data_Area (cx, cy))
                  {
                     dx = Locate_Data_X_ALL (cx, cy);
                     dy = Locate_Data_Y (cx, cy);
                  }
                 else
                   dx = DataBufferWidth / 2, dy = DataBufferHeight / 4;

                AnnotationDataX [index] = dx;
                AnnotationDataY [index] = dy;
             }

            AnnotationL [index] = left;
            AnnotationU [index] = up;
            AnnotationW [index] = width;
            AnnotationH [index] = height;
            return index;
         }
        else
           return 0;
     }

    static int PutAnnotationAtData (int index, int x, int y)
     {
        if ( index >= 1 && index <= LelAnnotation_Max )
         {
            int dx, dy;

            AnnotationDataX [index] = x;
            AnnotationDataY [index] = y;

            dx = x, dy = y;
            CalculateDisplayPosition (&dx, &dy);
            AnnotationL [index] = dx - AnnotationW [index] / 2;
            AnnotationU [index] = dy - AnnotationH [index] / 2;
            return index;
         }
        else
           return 0;
     }

    static void ClearAnnotationInMMode (void)
     {
        int i;
        for ( i = 1; i <= LelAnnotation_Max; i ++ )
          {
             if ( Cut_Data_Type (AnnotationDataX [i]) != DataType_UltraSound )
              {
                 AnnotationText [i][0] = 0;
                 if ( i ==  LastAnnotationHolding )
                    LastAnnotationHolding = 0;
              }
          }
     }

    char *LelGetAnnotation (int index, int *x, int *y)
     {
        int nx, ny;

        if ( x== NULL )
           x = &nx;
        if ( y == NULL)
           y = &ny;

        if ( index >= 1 && index <= LelAnnotation_Max )
         {
            *x = AnnotationDataX [index], *y = AnnotationDataY [index];

            //convert data position to buffer position
            CalculateDisplayPosition (x, y);
            return AnnotationText [index];
         }
        else
         {
            *x = *y = 0;
            return (char*) "";
         }
     }

    char *LelGetAnnotationText (int index)
     {
        return LelGetAnnotation (index, NULL, NULL);
     }
    int  LelGetAnnotationX (int index)
     {
        int x;
        LelGetAnnotation (index, &x, NULL);
        return x;
     }
    int  LelGetAnnotationY (int index)
     {
        int y;
        LelGetAnnotation (index, NULL, &y);
        return y;
     }

    int LelGetLastAnnotationHolding (void)
     {
        if ( LastAnnotationHolding >= Annotation_StartIndexForUser )
            return LastAnnotationHolding;
        else
          return 0;
     }

    int LelCountUserAnnotation (void)
     {
        int i;
        int kret = 0;
        for ( i = Annotation_StartIndexForUser; i <= LelAnnotation_Max; i ++ )
          if ( AnnotationText [i][0] != 0 )
             kret ++;

        return kret;
     }

    void LelClearAllAnnotations (void)
     {
        LelResetAnnotation ();
     }


    int LelGetDataFrequency (void)  //return: # frame received from device in previous second
     {
        return FPS_LastDataCount;
     }

    int LelGetDisplayFPS (void)   //return: frame displayed in previous second
      {
         return FPS_LastAccessCount;
      }

    float LelGetBatteryRemaining (void)  //return: 0~100  (%), -1 means not available
      {
          int i = lelapi_getBatteryCapacity();
          if (i == 200)
              i = 50;
          return  (float) i;
      }
    int LelIsBatteryCharging (void)
      {
          int i = lelapi_getBatteryCapacity();
          if (i == 200)
            return 1;
          else
            return 0;
      }
    float LelGetBoardTemperature (void)  //return: temperatre of device head, -10000 means not available
      {
          return lelapi_getTemperature();
      }

   int LelFreezeButtonJustPressed (void)
      {
          if (!Lel_UltraSound_Initialized )
              return 0;

         int waspressed = FreezeButtonPressed;

         if ( FreezeButtonPressed == 0 && lelapi_getButtonRise ())
          {
             FreezeButtonPressed = 1;
             lelapi_clearButtonRise ();
          }
         else if ( FreezeButtonPressed == 1 && lelapi_getButtonFall ())
          {
             FreezeButtonPressed = 0;
             lelapi_clearButtonFall ();
          }
         //FreezeButtonPressed = lelapi_getButtonStatus ();

         if ( FreezeButtonPressed  != waspressed && FreezeButtonPressed )
            return 1;
         else
            return 0;
      }


    int Lel_UpdateDisplayBuffer (void)
    {
        int x, y;
        unsigned char *p;
        static unsigned char bias = 0;

        bias +=16;
        //sprintf (StatusLine, "Count: %d", bias);

        for ( y = 0; y < DisplayBufferHeight; y ++)
            for ( x = 0, p = DisplayBuffer + DisplayBufferWidth * 4 * y;
                 x < DisplayBufferWidth; x++ )
            {
                //Alpha, Red, Green, Blue
                *(p++) = 0xFF;
                *(p++) = (unsigned char) ( y * 255 / DisplayBufferHeight);
                *(p++) = 128;
                *(p++) = (unsigned char) ( bias + x * 127 / DisplayBufferWidth);
            }

        return 1;
    }

   static void HandleNewFrame (void)
    {
       WaitingPostProcessor = 1;
       CheckNewFrame ();
    }
   static void CheckNewFrame (void)
    {
       //try to complete last post processing
       if ( PostProcessing )
        {
            if ( CheckCustomPostProcessor ())  //post processing completed
             {
                 PostProcessing = 0;
                 memcpy (BufB, BufB_Proc, sizeof (BufB_Proc));
                 memcpy (BufC, BufC_Proc, sizeof (BufC_Proc));
                 memcpy (BufP, BufP_Proc, sizeof (BufP_Proc));
                 AddNewFrame ();
             }
        }

       //try to start next post processing
       if ( !PostProcessing )
        {
           if ( WaitingPostProcessor )
            {
                int kpost;
                WaitingPostProcessor = 0;

                memcpy (BufB_Proc, BufB_Recv, sizeof (BufB_Proc));
                memcpy (BufC_Proc, BufC_Recv, sizeof (BufC_Proc));
                memcpy (BufP_Proc, BufP_Recv, sizeof (BufP_Proc));
                CntC_Proc = CntC;
                CntP_Proc = CntP;

                kpost = StartCustomPostProcessor ();

                if ( kpost != -1 )  //-1: incomplete
                 {
                     memcpy (BufB, BufB_Proc, sizeof (BufB_Proc));
                     memcpy (BufC, BufC_Proc, sizeof (BufC_Proc));
                     memcpy (BufP, BufP_Proc, sizeof (BufP_Proc));
                     AddNewFrame ();
                 }
                else
                   PostProcessing = 1;
            }
        }



    }
   static void AddNewFrame (void)
    {
                            BufFlag = 0;
                            if ( CntC_Proc  > 0 )
                                BufFlag |= BufFlag_Color;

                            AddDataToHistory ();
                            AddDataToMMode ();

                            UltraSoundToDisplay ();
                            FPS_CurrDataCount ++;

    }



   static int MedianFilter (byte *image, int width, int height)
    {
       int x, y;
       byte *p, *q, *qup, *qdown, *n;
       byte *buf = BufFilter;
       int imagelinesize = width;
       int buflinesize = imagelinesize +2;
       byte knear [9];
       int i, j, zero, jmin;
       byte nmin;

       //int sortcount = 0, skipcount = 0;   //2017/10/23 only 9 of 32768 pixels sorted when no data

       if ( width + 2 > BufFilter_X || height + 2 > BufFilter_Y || width < 1 || height < 1 )
          return 0;

       //copy image to buffer with rim duplicated
       for ( y = 0; y < height; y ++ )
        {
           q = buf + (y+1) * buflinesize;
           p =  image + y * imagelinesize;
           *q = *p;
           memcpy (q+1, p, imagelinesize);
           q [buflinesize-1] = p [imagelinesize-1];
        }
      //duplicate first and last row
      memcpy (buf, buf + 1 * buflinesize, buflinesize);
      memcpy (buf + (height+1) * buflinesize, buf + height *buflinesize, buflinesize);

      //calculate
      for ( y = 0; y < height; y ++ )
        {
           q = buf + (y+1) * buflinesize;
           p =  image + y * imagelinesize;
           qup = q - buflinesize, qdown = q + buflinesize;
           for ( x = 0; x < width; x ++, p++, q ++, qup++, qdown ++ )
            {
               //copy near pixel
               zero = 0;  n = knear;
               if ( (*(n++) = *(qup-1)) == 0x80 )
                  zero ++;
               if ( (*(n++) = *(qup))== 0x80 )
                  zero ++;
               if ( (*(n++) = *(qup+1))== 0x80 )
                  zero ++;
               if ( (*(n++) = *(q-1)) == 0x80 )
                  zero ++;
               if ( (*(n++) = *(q))== 0x80 )
                  zero ++;
               if ( (*(n++) = *(q+1))== 0x80 )
                  zero ++;
               if ( (*(n++) = *(qdown-1)) == 0x80 )
                  zero ++;
               if ( (*(n++) = *(qdown))== 0x80 )
                  zero ++;
               if ( (*(n++) = *(qdown+1))== 0x80 )
                  zero ++;

               if ( zero < 5 )
                {
                   //apply selection sort (only half)
                   for ( i = 0; i < 5; i ++ )
                    {
                       for ( jmin = i, nmin = knear [i], j = i + 1; j < 9; j ++ )
                          if (  knear [j] < nmin )
                             jmin = j, nmin = knear [j];
                       if  ( jmin != i )
                           knear [jmin] = knear [i], knear [i] = nmin;
                    }
                   *p = knear [4];
                    //sortcount++;
                }
               else  //# of zero >= 5  => just output zero
                {
                  *p = 0x80;

                  //skipcount++;
                }
            }  //for x
        }  //for y

       return 1;
       //return skipcount + sortcount;

    }


     static float distLinear (int x0, int y0, int x1, int y1)
     {
         float soundSpeed = 1540e3; // mm/s
         float sampleRate = 50e6; // Hz
         int decimation = 8;
         float pitch = (float)0.3; // in the unit of mm
         float x_scale = pitch;
         float y_scale = (float)(soundSpeed*(1.0/sampleRate)*decimation/2); // in the unit of mm
         float deltaX_mm = (x1 - x0) * x_scale;
         float deltaY_mm = (y1 - y0) * y_scale;
          return (float) sqrt(deltaX_mm*deltaX_mm + deltaY_mm*deltaY_mm);
    }

    static float distConvex (int x0, int y0, int x1, int y1)
    {
         float soundSpeed = 1540e3; // in the unit of mm/s
         float sampleRate = 25e6; // in the unit of Hz
         int decimation = 12;
         float pitch = (float) 0.495; // in the unit of mm
         float r = 60; // in the unit of mm
         float theta = pitch/r;
         float R = (float)(soundSpeed*(1.0/sampleRate)*decimation/2); // in the unit of mm
         // point 0 at (X0, Y0)
         // point 1 at (X1, Y1)
         float X0 = 0;
         float Y0 = r + y0*R;
         float R1 = r + y1*R;
         float deltaTheta = (x1 - x0)*theta;
         float X1 = R1*sin(deltaTheta);
         float Y1 = R1*cos(deltaTheta);
         float deltaX_mm = X1 - X0;
         float deltaY_mm = Y1 - Y0;
         return (float) sqrt(deltaX_mm*deltaX_mm + deltaY_mm*deltaY_mm);
    }

}; //extern C


//----------------------------------------------------------------------------------------
//    Customizing Functions
//
//    Implement following functions by yourself for customizing
//    this Core Module
//
//----------------------------------------------------------------------------------------

extern "C"
{
       //for smoothing color data
       static  byte  BufC_Past [BufC_X*BufC_Y];  //(size: 32K)

                //  Invoked after APP core initialized, before device is connected
     static int InitializeCustomPostProcessor (void)
       {
          memset(BufC_Past, 128, sizeof(BufC_Past));

          //return 1 on success, 0 if failure
          return 1;
       }

                //  Invoked before APP core is to be destructed
     static void TerminateCustomPostProcessor (void)
       {
       }
                //  Invoked after APP core is connected to devce and identified
                //  the type of device
     static void CustomPostProcessor_AfterDeviceConnected (int headtype)
       {
          //head type: 0x49-Linear  0x47-convex
       }

                //  Invoked after APP core turned on color mode of ultrasound device
     static void CustomPostProcessor_BeforeStartColorMode (void)
       {
          memset(BufC_Past, 0x80, sizeof(BufC_Past));
       }

                //  Invoked after APP core received a new ultrasound image from device
     static int StartCustomPostProcessor (void)
       {
           //    Please post-process following ultrasound images here
           //    (1) BufB_Proc [BufB_X * BufB_Y]
           //    (2) if CntC_Proc > 0, please process BufC_Proc [BufC_X * BufC_Y] together
           //    (3) if CntP_Proc > 0, please process BufP_Proc [BufP_X * BufP_Y] together
           //
           //   return 0:none, 1:complete,
           //   -1:started but not completed. APP core will call CheckCustomPostProcessor ()
           //    repeatly until CheckCustomPostProcessor () returns 1.
           //

           //Apply median filter if you like
           //MedianFilter(BufB_Proc, BufB_X, BufB_Y);

           if ( CntC_Proc > 0 )  //If color data exists
            {
                //Combine with historical data
                int x, y;
                byte *p, *q;
                int avg;
                for ( y = 0; y < BufC_Y; y ++ )
                {
                    q = BufC_Proc + BufC_X * y;
                    p = BufC_Past + BufC_X * y;
                    for ( x = 0; x < BufC_X; x++, p++, q++)
                    {
                        avg = 0x80 + (  ((((int)(*p))-0x80) * 1 + (((int)(*q))-0x80)) / 2 );

                        if ( avg < 0 )
                            avg = 0;
                        else if ( avg > 0xFF )
                            avg = 0xFF;
                        else if ( 0x7C <= avg && avg <= 0x84)
                            avg = 0x80;

                        *q = *p = (byte) avg;
                    }
                }

                //Apply median Filter twice
                MedianFilter(BufC_Proc, BufC_X, BufC_Y);
                MedianFilter(BufC_Proc, BufC_X, BufC_Y);
            }


          return 1;
       }

                //  If you returned -1 in StartCustomPostProcessor (), APP core
                //  will invoke following function periodically to check the status
                //  of your post-processing routine.
     static int CheckCustomPostProcessor (void)
       {
          //return 1 on complete, 0 still in processing
          return 1;
       }


         /*

         //----------------------------------------------------------------------------------------
         //  Multi-thread example
         //
         //  The following example inverts the entire
         //   ultrasound image in another thread
         //----------------------------------------------------------------------------------------

         static volatile int proc_complete = 0;

         #if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(__BORLANDC__)
           static HANDLE g_hThread = NULL;
         #else
           #include <pthread.h>
           static pthread_t g_hThread = NULL;
         #endif

          #if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(__BORLANDC__)
          static DWORD WINAPI postproc(void* pv)
          #else
          static void *postproc(void* pv)
          #endif
           {
              //invert entire ultrasound image
              int x, y;
              for (y = 0; y < BufB_Y; y ++ )
                for ( x= 0; x < BufB_X; x ++ )
                   BufB_Proc [y * BufB_X + x] ^= 0xFF;

              proc_complete = 1;

          #if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(__BORLANDC__)
              return 0;
          #else
              return NULL;
          #endif
          }

         static int StartCustomPostProcessor (void)
           {
             #if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(__BORLANDC__)
                DWORD dwThreadID;
                g_hThread = CreateThread(NULL, 0, postproc, NULL, 0, &dwThreadID);
             #else  //Linux
                pthread_create (&g_hThread,NULL, postproc, NULL);
             #endif
              proc_complete = 0;
              return -1;
           }

         static int CheckCustomPostProcessor (void)
           {
              if ( proc_complete )
                return 1;
              else
                return 0;
           }
         //----------------------------------------------------------------------------------------

         */
    
}; //extern C




