/*

    Leltek UltraSound-Bridging-Header.h

    (c) Leltek 2018

    Header File
    Cross-platform Core Module of Leltek UltraSound APP
     (Win32, Android, iOS)

    APP Core Module Usage

       Initializing:
           Call LelInitialize () to initialize API
           Call LelSetResolutionDiv () to reduce display resolution if you are
                 using a slower CPU such as mobile device
           Call LelSetViewSize () to initialize ultrasound image display buffer

       Ending:
           Call LelDestruct()  to release resources

       Start(Connect)/Stop Ultrasound device
           Call LelStart () to connect device and start to receive data.
                            Please use a second thread for it may take a few
                            moment at the first time.
                            NOTE: for mobile devices, the second thread may
                            not be able to update UI. You may need to notify
                            the main thread for updating UI by some ways such
                            as setting some variables.

           Call LelStop () to stop receive data. call LelStart () to resume


       Display Ultrasound Image & UI
           Call LelGetDisplayBufferWidth(), LelGetDisplayBufferHeight()
                 to get the display buffer size in APP core
           Call LelGetDisplayBuffer() to get the RGB array of display buffer
                and convert it to a device bitmap in your platform, such as
                HBitmap in win32 or CGImage in iOS
           Call LelGetZoomScale(), LelGetZoomLeft(), LelGetZoomUp()
                and extract the visible portion of above device bitmap to a
                second device bitamp which is the same size as the display
                area on your screen
           Call LelGetAnnotation () to draw text of annotations on this
                second device bitmap; call LelPutAnnotation () to inform
                APP core the real size of annotaion on screen, so the
                APP core can move it when user touchs it on screen.
            Put this second device bitmap to your screen

       NOTE: you should re-display ultrasound image after all the
           following operations such as calling LelCycle () or LelTouch ().

       Cycle Timer
           Call LelCycle() periodically for cycle operations of APP core

       On Windows Resized / Mobile Device Screen Rotated
           Call LelSetViewSize () with new display area size on screen

       On Mouse/Screen Touch
           Call LelTouch () to send mouse/touch event to APP core;
                 You can send one or two fingers touch
           Call LelGetLastAnnotationHolding () to check if user selected
                 a new annotation on screen for editing

       Other Control
           Call other APIs for other control

*/

#ifdef __cplusplus
extern "C"
{
#endif

int LelInitialize (const char *AWorkDir);
                  /*
                       Call this function to initialize APP core at beginning of
                       your application.
                       Current working directory will be changed to AWorkDir.

                       Remember to call LelSetViewSize (), LelStart () after
                       calling this funciton.
                       Return 1 on success, 0 on failure
                  */

void LelDestruct (void);
                  /* Call this function to release all resources allocated by
                      APP core, usually at the end of your application
                    */

void LelSetResolutionDiv (int div);
                   /*   If your application will be running on a slower device,
                         call this API with div > 1 to reduce the resolution of
                         displayed ultrasound image.  If you do not call this
                         API, the resolution of displayed ultrasound image will
                         be the same as the display area on your screen.
                    */

void LelSetViewSize (int w, int h);
                   /*  Inform APP core the display area size in w* h  pixels on
                         screen. APP core will create a bitmap buffer of size
                         (w/div, h/div) where div is the parameter you send
                         to APP core by LelSetResolutionDiv (). If you did
                         not call LelSetResolutionDiv, div is 1.
                         Call this API when you change the display area size
                         on screen.
                    */

int  LelStart (void);
                    /*
                        Connect to Leltek ultrasound device and start to
                        receive data.
                            Please use a second thread for it may take a few
                            moment at the first time.
                            NOTE: for mobile devices, the second thread may
                            not be able to update UI. You may need to notify
                            the main thread for updating UI by some ways such
                            as setting some variables.
                     */

void  LelStop (void);
                     /*
                          Stop receiving ultrasound data. Call LelStart () to resume.
                      */

void LelCycle (void);
                     /*
                          Call this function periodically in your application so the
                          APP core can perform some periodical works necessary.

                          It is suggested to update the ultrasound image displayed
                          on your device through APIs such as LelGetDisplayBuffer ()
                          after calling this function.
                      */

int LelSetColorMode (int mode);  //1-on, 0-off
int LelGetColorMode (void);
                      /*  Start/Stop color mode of ultrasound device
                       */

int LelSetMMode (int mode);  //1-on, 0-off
int LelGetMMode (void);
                      /*  Start/Stop M mode of ultrasound device

                            NOTE: It is suggested to call LelGetLastAnnotationHolding ()
                            for the annotation being edited may be clearred

                           It is suggested to update the ultrasound image displayed
                           on your device through APIs such as LelGetDisplayBuffer ()
                           after calling this function.
                       */

int LelSetPower (int mode); //1-30V, 0-20V
int LelSetGain (int mode);  //1-24db, 0-18db
                      // Change power/gain mode

int LelGetHistoryMax (void);
                       // Maxium # of images of stored historical ultrasound data

int LelReadFromHistory (int index);  //index: 0... LelGetHistoryMax ()
                       /*
                           Read from stored historical ultrasound data and put
                           it into the display buffer of APP core.

                           It is suggested to update the ultrasound image displayed
                           on your device through APIs such as LelGetDisplayBuffer ()
                           after calling this function.
                        */

int LelGetDisplayBufferWidth (void);  //width of buffer in pixels
int LelGetDisplayBufferHeight (void); //height of buffer in pixels
int LelGetDisplayBufferSize (void);   //size of buffer in bytes
unsigned char *LelGetDisplayBuffer (void);
                   //RGB array of display buffer; order of RGB depends on your platform

                       /*
                           Retrieve the display buffer in APP core to display ultrasound
                           image on your device.

                           To surport zooming features, if LelGetZoomScale () > 1,
                           You should only display following potion of this display buffer
                           onto screen
                           Top-Left
                             x= LelGetZoomLeft (), y = LelGetZoomUp()
                           Width
                              LelGetDisplayBufferWidth () / LelGetZoomScale ()
                           Height
                              LelGetDisplayBufferHeight () / LelGetZoomScale ()

                           NOTE: remeber to scale the display buffer yourself after
                           it is converted into the device bitmap on your platform
                           If you had called LelSetResolutionDiv (div) with div > 1
                        */

char *LelGetStatusLine (void);
                        //Get a status text for displaying (in UTF8)

int LelGetLastDataFPS (void);
                        //FPS (frame per second) of current ultra sound data receving

int LelGetLastAccessFPS (void);
                        //FPS (frame per second) of current displaying speed

int LelTouch (int act, int total, int x, int y, int x2, int y2);
            //values of act
            #define LelTouch_Down 0
            #define LelTouch_Move 1
            #define LelTouch_Up 2
            /*
                   Sending mouse or screen-touch acts of user to APP core

                   act: touch down or mouse down/ touch move or mouse move/ touch up or mouve up
                   On single-finger touch of mouse move
                      Send total = 1, (x, y) = touch postion (in ultra sound display area)
                         x2=y2=-1
                   On double-finger touch
                      Send total = 2, (x, y) (x2, y2)= touch postion (in ultra sound display area)

                    (x, y) is in the coordinate of ultrasound image display area on screen.
                    For example, (x, y)=(0,0) means upper-left corner of display area on screen

                   NOTE: display buffer may updated and editing annotation may be changed.
                           It is suggested to update the ultrasound image displayed
                           on your device through APIs such as LelGetDisplayBuffer (),
                           LelGetLastAnnotationHolding () after calling this function.
              */

float LelGetZoomScale (void);
int LelGetZoomLeft (void);
int LelGetZoomUp (void);
            /*
                 Get zooming status
                 scale: 1 means not zoomed in; 1.5 means 1.5x (scale up to 150%)
                 (left, up) means the top-left corner on screen is mapped to display
                 buffer at (left, up)
             */

void LelSetPixelValueAdd (float add);
float LelGetPixelValueAdd (void);
void LelSetPixelValueScale (float scale);
float LelGetPixelValueScale (void);
                            /*
                                  Brightness/Contrast modification
                                  Display pixel value (0...255) =  ultrasound data value(0...255) * PixelValueScale  + PixelValueAdd
                                  By default, PixelValueAdd = 0.0f, PixelValueScale = 1.0f
                                  value out of legal range (0-255) will be truncated
                             */
void LelSetCustomPixelValueMapping (unsigned char *map);
                                  /*
                                     Provide your custom mapping from ultrasound data to pixel value
                                     Display pixel value (0...255) =  map [ultrasound data value(0...255)]
                                   */
void LelSetCustomPixelValueMappingByIndex (int index, int value, int refresh);
                                  /*
                                     One-by-one setting version of above function:
                                         set map [index] = value
                                         refresh: if it is non-zero, displayBuffer will be updated.
                                     You can set lots of entries with refresh = 0 and only send refresh = 1
                                     for the last setting to avoid unnecessary display buffer update
                                    */


void LelResetZoom (void);   //reset zooming to 100%
void LelResetTouch (void);    //reset touching status on screen
void LelResetRuler (void);    //clear all rulers on screen
void LelResetAnnotation (void);  //clear all annotations on screen
             /*
                   It is suggested to update the ultrasound image displayed
                   on your device through APIs such as LelGetDisplayBuffer ()
                   after above APIs called.
             */


int LelGetMarkMode(void);
void LelSetMarkMode (int mode);
            //values of mode
            #define LelMarkMode_None 0
            #define LelMarkMode_Line 1
            #define LelMarkMode_Ellipse 2
            #define LelMarkMode_Annotate 3
            /*
                  Setting current UI opreation when touching screen or
                        left-mouse-button down
                    None: panning when zooming (default)
                    Line: measuring distance by line
                    Ellipse: measuring area by ellipse
                    Annotate: adding annotation
             */


void LelClearAllRulers (void);
                /* Clear all rulers on screen

                           It is suggested to update the ultrasound image displayed
                           on your device through APIs such as LelGetDisplayBuffer (),
                  */

void LelSetRulerVolumeMeasurement (float multiplier);
             /*  Turn on volume measurment of rulers.
                   If an ellipse and a line is drawn,
                   following volume measurement will also be displayed:

                   [multiplier] * [radius 1 of ellipse] * [radius 2 of ellipse] * [line length / 2]

                   Set multiplier = 0 to turn off this function.
              */

#define LelAnnotation_Max 99
                  /* Maxinum # of annotations can be added
                       NOTE: some annotaion entries are reserved by APP core
                       for special usage such as marking the measurement of distance
                       or area, so the annotations user can add is fewer than this value
                   */

#define LelAnnotation_MaxLen 400
                   /*  Maxium length in bytes for an annotation. Because the text
                         are stored in UTF8 encoding, the real characters you can use
                         is lesser.
                     */

int LelSetAnnotation (int index, char *text);
                    /*
                       Add or set an annotation

                       index: 1...AnnotationMax, index of annotation to set
                       index == 0 means adding a new one, return index of added annotation (1~)
                             (return 0 if table full)

                       text []: annotation string in UTF8 encoding.
                       text [] = empty string means remove this annotation
                    */

int LelPutAnnotation (int index, int left, int up, int width, int height, int move);
                   /*
                        Inform core the display area on screen of annotation.
                        (left, up) is in the coordinate of ultrasound image display area on screen.
                        For example, (left, up)=(0,0) means upper-left corner of display area.

                        If move is set, core will move the annotation; if not, core will only
                        set the touch area of the annotation.

                        Because APP core is platform-independent, please calculate the size of display
                        text on screen by APIs of your platform, and call LelPutAnnotation ()
                        with move = 0 so that APP core knows the display area, therefore it can
                        allow user to touch and move the annotation.
                    */

char *LelGetAnnotation (int index, int *x, int *y);
                    /*
                       Get text (in UTF8 encoding) and center position of annotation
                       (index:1~)
                       Empty string will be returned if none or index out of range

                       (*x, *y) is  in the coordinate of ultrasound image display area on screen.
                        For example, (*x, *y)=(0,0) means upper-left  corner of display area.

                       NOTE: (*x, *y) is the center position of the annotation; please calculate
                       the postion of text display area yourself by text size through the APIs of
                       your platform, and call LelPutAnnotation () with move = 0 to inform APP
                       core the real display area of the annotatoin, so APP core can move it
                       when user touchs it.
                     */
char *LelGetAnnotationText (int index);
int  LelGetAnnotationX (int index);
int  LelGetAnnotationY (int index);
                     //separated function of LelGetAnnotation ()

int LelGetLastAnnotationHolding (void);
                     /*
                         Get the index of annotation user clicked or holding (0 if none).
                         Please get the text of this annotation by LelGetAnnotation ()
                         and put it into the annotation text edit window of your
                         application, and change the annotation by LelSetAnnotation ()
                         if user changed text in the text edit window of your application.

                         Please note that touching screen, setting MMode and some
                         operations will modify the returned value. In this case, the
                         annotation text edit window of your application should be
                         switched to the text of new annotation holding, or clearred
                         if it returns 0.
                     */

int LelCountUserAnnotation (void);
                     //total # of annotations added by user

void LelClearAllAnnotations (void);
            /*
                Clear all annotations added by user.

                NOTE: It is suggested to call LelGetLastAnnotationHolding ()
                for the annotation being edited may be clearred
              */



           /*
                Functions for checking device status
            */
int LelGetDataFrequency (void);  //return: # of frame received from device in previous second
int LelGetDisplayFPS (void);   //return: # of frame displayed by this APP core in previous second

int LelIsBatteryCharging (void);  //return: 1 if battery is charging, 0 if not

float LelGetBatteryRemaining (void);  //return: 0~100  (%), -1 means not available;
                            //NOTE: if battery is charging, the returned value may be invalid.
                            //Contact Letek for more information.

float LelGetBoardTemperature (void); //return: temperatre (Celsius), -10000 means not available

int LelFreezeButtonJustPressed (void);
                                          /*  return 1 if freeze button just pressed, 0 if not.
                                               NOTE: 1 will be returned only once;
                                                  the following call will get 0.
                                           */





int Lel_UpdateDisplayBuffer (void); //API not used anymore

// Cliff 0112 get line
float LelGetFinalDis(void); //final real dis number get

#ifdef __cplusplus
};  //extern C
#endif


