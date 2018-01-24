


//Kiki
#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(__BORLANDC__)
  #ifndef LELTEK_WIN32
  #define LELTEK_WIN32
  #endif
#endif

#include "stdafx.h"

//Kiki
#ifdef LELTEK_WIN32

    //Kiki vs2013
    #undef UNICODE
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>

     //#include <streams.h>
     #include <stdio.h>
     #include <iostream>
     #include <sstream>
     #include <math.h>
     #include <sys/stat.h>


     #include "lelapi.h"
     #include "showMsg.h"
     #include "utCtrl.h"
     #include "tcp.h"
     #include "frameBuf.h"
     #include "graymap.h"  //Kiki 201801

     //Kiki no tuple
     //#include "parseTxt.h"

#else  //Linux

     //#include <streams.h>
     #include <stdio.h>
     #include <iostream>
     #include <sstream>
     #include <math.h>
     #include <sys/stat.h>
     #include <pthread.h>

     //Kiki 201801
     #include "showMsg.h" //include this before tcp.h

     #include "tcp.h"    //Include this first which will patch win32 api

     #include "lelapi.h"

     #include "utCtrl.h"
     #include "frameBuf.h"
     #include "graymap.h"

     //Kiki no tuple
     //#include "parseTxt.h"

     #ifndef __cdecl
     #define __cdecl
     #endif

     #ifndef WINAPI
     #define WINAPI
     #endif


#endif // LELTEK_WIN32

using namespace std;

#define SYNC_BMODE 0x00FFFF00
#define SYNC_CMODE 0x00EEEE00
#define SYNC_INFO 0x00AAAA00

#define RTABLE_MAX_TRANSFER_BYTE 504
#define RTABLE_MAX_TRANSFER_DW (RTABLE_MAX_TRANSFER_BYTE/4)

#define tx_en_addr 0x13
#define tx_en_start 0x6
#define tx_en_stop 0x8

#define power_ctl_addr 0x58
#define probe_id_addr 0x15

#define button_addr 0x57
#define button_status_bit 0
#define button_rise_bit 1
#define button_fall_bit 2

WORD g_probeID = 0x1F;
string g_dirname = "";

#define lenImgHeader 64
#define numImg 4
#define depthBMode 512
#define depthCMode 256
#define widthBMode 128
#define widthCMode 128
#define lenBMode (widthBMode*depthBMode)
#define lenCMode (widthCMode*depthCMode)
#define lenRaw (9*1024)

volatile int pushIndexImg;
volatile int pushIndexImgInc;
volatile int popIndexImg;
BYTE g_raw[lenRaw+2048];
BYTE g_bModeImg[numImg][lenBMode];
BYTE g_cModeImg[numImg][lenCMode];
volatile bool g_cModeVld[numImg];
UtVideoHeader g_frameHeader[numImg];

float fpgaTemperature;
int batteryCapacity;
int buttonStatus;
//bool buttonRise;
//bool buttonFall;
const float absTemp = -273.15f;
const float ratioTemp = 503.975f;

volatile bool newInfo = false;

int g_grayMapIndex = 0;

bool lelapi_write_byte(WORD addr, BYTE write_data) {
    return ut_tcpWriteReg(addr, &write_data, 1);
}

bool lelapi_write_word(WORD addr, WORD write_data) {
    return ut_tcpWriteReg(addr, (BYTE *)&write_data, 2);
}

bool lelapi_write_dw(WORD addr, UINT write_data) {
    return ut_tcpWriteReg(addr, (BYTE *)&write_data, 4);
}

BYTE lelapi_read_byte(WORD addr, bool *status=NULL) {
    bool tmp;
    BYTE data;
    tmp = ut_tcpReadReg(addr, &data, 1);
    if(status!=NULL) *status = tmp;
    return data;
}

WORD lelapi_read_word(WORD addr, bool *status=NULL) {
    bool tmp;
    WORD data;
    tmp = ut_tcpReadReg(addr, (BYTE *)&data, 2);
    if(status!=NULL) *status = tmp;
    return data;
}

UINT lelapi_read_dw(WORD addr, bool *status=NULL) {
    bool tmp;
    UINT data;
    tmp = ut_tcpReadReg(addr, (BYTE *)&data, 4);
    if(status!=NULL) *status = tmp;
    return data;
}

bool lelapi_load_rtable(WORD addr, UINT *table, WORD len) { // table[0]=length
    bool status = true;
    if(status) {
        while(len > RTABLE_MAX_TRANSFER_DW) {
            status = ut_tcpWriteTable(addr,(BYTE *)table,RTABLE_MAX_TRANSFER_BYTE);
            addr+=RTABLE_MAX_TRANSFER_DW;
            table+=RTABLE_MAX_TRANSFER_DW;
            len-=RTABLE_MAX_TRANSFER_DW;
        }
    }
    if(status) {
        status = ut_tcpWriteTable(addr,(BYTE *)table,len*4);
    }
    return status;
}

bool lelapi_load_rtable_file(WORD addr, string filename) {
  bool status;

  //Kiki no tuple
  /*
  auto v = ParseTxt::parseRTable(filename);
  //showMsg("table=%s, lines=%d, addr=%x\n", filename.c_str(),v.size(),addr);
  if(v.size()==0) {
      showMsg("Rtable %s for offset %x read fail.",filename.c_str(),addr);
  }
  status = lelapi_load_rtable(addr,&v[0],v.size());
  */

  FILE *fp;
  if ( (fp = fopen (filename.c_str(), "rb")) != NULL )
   {
      //estimate file size
      int filesize;
      UINT *v;
      int vtotal, vsize;
      char line [1024];
       char *pchar;

      fseek (fp, 0L, 2);
      filesize = (int) ftell (fp);
      fseek (fp, 0L, 0);

      vsize = filesize / 4;  //estimated size
      if ((v = new UINT [vsize]) != NULL )
       {
           for ( vtotal = 0;  fgets (line, 1000, fp) != NULL && vtotal <vsize; )
            {
                 for ( pchar = line; *pchar; pchar ++ )
                    if ( *pchar == '#' )
                     {
                        *pchar = 0;
                        break;
                     }

               if ( sscanf (line, "%x", v + vtotal) == 1 )
                  vtotal ++;
            }

           if ( vtotal > 0 )
              status = lelapi_load_rtable(addr, v, vtotal);
           else
               status = true; //empty table

          delete v;
       }
      else
         status = false;

      fclose (fp);
   }
  else
   {
     status = false;
   }


  return status;
}

bool lelapi_load_reg(string filename) {
  bool status = true;

  //Kiki no tuple
  /*
  auto v2 = ParseTxt::parseInit(filename);

  for (auto value : v2)
  {
    auto& cmd = get<0>(value);
    if(cmd=="wr") {
        auto& addr = get<1>(value);
        vector<uint8_t>& v3 = get<2>(value);
        status = ut_tcpWriteReg(addr, &v3[0], v3.size());
        if(!status) break;
    } else if(cmd=="wait") {
        auto& wait_ms = get<1>(value);
        Sleep(wait_ms);
    }
  }
  */

  FILE *fp;
  if ( (fp = fopen (filename.c_str(), "rb")) != NULL )
   {
      char cmd [90];
      WORD addr;
      BYTE value [10];    //at most 10 values
      int iaddr, ivalue [10];
      char line [1024];
      int kscan, i;
       char *pchar;

      while (fgets (line, 1000, fp) != NULL )
       {
                 for ( pchar = line; *pchar; pchar ++ )
                    if ( *pchar == '#' )
                     {
                        *pchar = 0;
                        break;
                     }

          if ( (kscan = sscanf (line, "%80s %x %x %x %x %x %x %x %x %x %x %x",
                  cmd, &iaddr,
                  ivalue + 0,  ivalue + 1, ivalue + 2, ivalue + 3, ivalue + 4,
                  ivalue + 5,  ivalue + 6, ivalue + 7, ivalue + 8, ivalue + 9)) >= 2 )
           {
              if ( !strcmp (cmd, "wr") )
               {
                  if ( kscan > 2 )
                   {
                      addr = (WORD) iaddr;
                      for (i = 0; i < kscan- 2; i ++ )
                         value [i] = (BYTE) ivalue [i];
                       status = ut_tcpWriteReg(addr, value, kscan - 2);
                       if(!status)
                           break;
                    }
               }
              else if( !strcmp (cmd, "wait") )
               {
                  Sleep(iaddr);
               }
           }
       } //while

      fclose (fp);
   }
  else
   {
     status = false;
   }

  return status;
}

bool lelapi_load_rtables() {
    bool status = true;
    if(status) status = lelapi_load_rtable_file(R_TABLE_ID              ,g_dirname+"rTable.txt");
    if(status) status = lelapi_load_rtable_file(TGC_B_ID                ,g_dirname+"tgcTableB.txt");
    if(status) status = lelapi_load_rtable_file(TGC_C_ID                ,g_dirname+"tgcTableC.txt");
    if(status) status = lelapi_load_rtable_file(GRAY256_ID              ,g_dirname+"logTable.txt");
    if(status) status = lelapi_load_rtable_file(TX_PATTERN32_B_ID       ,g_dirname+"txPattern5000.txt");
    if(status) status = lelapi_load_rtable_file(TX_PATTERN32_C_ID       ,g_dirname+"txPattern6000.txt");
    if(status) status = lelapi_load_rtable_file(TZ_TABLE_ID             ,g_dirname+"tzTable.txt");
    if(status) status = lelapi_load_rtable_file(TX_TABLE_TX_ID          ,g_dirname+"txTable.txt");
    if(status) status = lelapi_load_rtable_file(TX_TABLE_RX_ID          ,g_dirname+"rxTable.txt");
    return status;
}

//Kiki  add "volatile"
volatile bool g_bExitThread = false;

//Kiki
#ifdef LELTEK_WIN32
  HANDLE g_hThread = NULL;
#else //Linux
  pthread_t g_hThread = NULL;
#endif


bool reqPauseMonitorProc = false;
bool ackPauseMonitorProc;

void reshape(BYTE *dest, BYTE *src, int len, int destWidth=128, int srcWidth=16) {
    while(len > 0) {
        memcpy(dest,src,srcWidth);
        len-=srcWidth;
        src+=srcWidth;
        dest+=destWidth;
    }
}

WORD temp2hex (float temp) {
    return (WORD)round((temp-absTemp)/ratioTemp*65536);
}

float hex2temp (WORD hex) {
    return ratioTemp*hex/65536+absTemp;
}

LELAPI_API int ut_calcBatteryCapacity (WORD vbat, BYTE vbus, float temp) {
    static int minBatteryLevel = 100;
    static int minBatteryCount = 0;
    static int minBatteryCountLimit = 10;

    int newBatteryLevel;
    if (vbus == 1) {
        newBatteryLevel = 200;
        minBatteryLevel = 100;
        minBatteryCount = 0;
    } else {
        int vbat_comp = vbat + (44 - temp) * 44;
        if (vbat_comp > 0x5AA0) newBatteryLevel = 100;
        else if (vbat_comp > 0x5700)
            newBatteryLevel = 75 + (100 - 75) * (vbat_comp - 0x5700) / (0x5AA0 - 0x5700);
        else if (vbat_comp > 0x5420)
            newBatteryLevel = 50 + (75 - 50) * (vbat_comp - 0x5420) / (0x5700 - 0x5420);
        else if (vbat_comp > 0x5260)
            newBatteryLevel = 25 + (50 - 25) * (vbat_comp - 0x5260) / (0x5420 - 0x5260);
        else if (vbat_comp > 0x5060)
            newBatteryLevel = 0 + (25 - 0) * (vbat_comp - 0x5060) / (0x5260 - 0x5060);
        else newBatteryLevel = 0;
        if (minBatteryLevel > newBatteryLevel) {
            minBatteryCount++;
            if (minBatteryCount > minBatteryCountLimit) {
                minBatteryCount = 0;
                minBatteryLevel = newBatteryLevel;
            } else {
                newBatteryLevel = minBatteryLevel;
            }
        } else {
            newBatteryLevel = minBatteryLevel;
            minBatteryCount = 0;
        }
    }
    return newBatteryLevel;
}

void updateInfo(UtVideoHeader* ptrHeader) {
    fpgaTemperature = hex2temp(ptrHeader->measured_temp);
    batteryCapacity = ut_calcBatteryCapacity (
            ptrHeader->init_vbat,
            ptrHeader->vbus,
            fpgaTemperature
        );
    buttonStatus = (ptrHeader->button) & (1<<button_status_bit);
    //buttonRise = ((ptrHeader->button) & (1<<button_rise_bit))!=0;
    //buttonFall = ((ptrHeader->button) & (1<<button_fall_bit))!=0;
    newInfo = true;
}

inline unsigned char grayMapping(unsigned char data, int index) {
    return grayMapArray[index][data];
}

//Kiki
#ifdef LELTEK_WIN32
static DWORD WINAPI MonitorProc(void* pv)
#else
static void *MonitorProc(void* pv)
#endif
 {
    bool status;
    UtVideoHeader *hdrImg = (UtVideoHeader *)g_raw;
    // Tcp
    WORD tcpLen;
    TcpImageHeader tcpHeader;
    UINT maxLen = lenRaw;
    showMsg("MonitorProc begin.");
    while(!g_bExitThread) {
        int lenRead = 0;
        BYTE *pTcpImage = g_raw;
        do {
//showMsg("ut_tcpReadImage receive... %d/%d\n", lenRead, lenRaw);
          status = ut_tcpReadImage((UINT *)&tcpHeader, (UINT *)pTcpImage, maxLen, &tcpLen);
//showMsg("ut_tcpReadImage got %d.%d \n", tcpHeader.cmd, tcpLen);
          if ((tcpHeader.cmd == 1)||(tcpHeader.cmd == 3)) {
            pTcpImage += tcpLen;
            lenRead += tcpLen;
            if (lenRead > lenRaw) {
              showMsg("ut_tcpReadImage : too large lenRead = %d, len=%d seq=%d\n", lenRead, tcpLen, tcpHeader.seq);
              break;
            }
          }
        } while (tcpHeader.cmd != 3);
        if(hdrImg->sync==SYNC_BMODE) {
            if(lenRead!=1024*9) {
                showMsg("MonitorProc: B mode length error, sync=%x, len=%d, imgblock=%d\n",hdrImg->sync, lenRead, hdrImg->tx_imgblock);
            } else {
                int len = lenRead-lenImgHeader;
                if(len > 16*depthBMode) len = 16*depthBMode;
                reshape(g_bModeImg[pushIndexImg]+16*(hdrImg->tx_imgblock-8),g_raw+lenImgHeader,len);
                if(hdrImg->last_imgblock) {
                    g_frameHeader[pushIndexImg] = *hdrImg;
                    pushIndexImg = pushIndexImgInc;
                    pushIndexImgInc = (pushIndexImgInc+1)%numImg;
                }
            }
        } else if(hdrImg->sync==SYNC_CMODE) {
            if(lenRead!=1024*5) {
                showMsg("MonitorProc: C mode length error, sync=%x, len=%d, imgblock=%d\n",hdrImg->sync, lenRead, hdrImg->tx_imgblock);
            } else {
                int len = lenRead-lenImgHeader;
                if(len > 16*depthCMode) len = 16*depthCMode;
                reshape(g_cModeImg[pushIndexImg]+16*(hdrImg->tx_imgblock-8),g_raw+lenImgHeader,len);
                g_cModeVld[pushIndexImg] = true;
                if(hdrImg->last_imgblock) {
                    g_frameHeader[pushIndexImg] = *hdrImg;
                    pushIndexImg = pushIndexImgInc;
                    pushIndexImgInc = (pushIndexImgInc+1)%numImg;
                }
            }
        } else if(hdrImg->sync==SYNC_INFO) {
            updateInfo(hdrImg);
            showMsg("MonitorProc: info packet.\n");
        } else {
            showMsg("MonitorProc: unknown packet!\n");
        }
    }
    showMsg("MonitorProc end.");

//Kiki
#ifdef LELTEK_WIN32
    return 0;
#else
    return NULL;
#endif
}


LELAPI_API bool lelapi_init() {
    ut_initTcp();
    return true;
}

LELAPI_API void lelapi_exit() {
    //bool status;  //Kiki 201801
    if (g_hThread)
    {
      //showMsg("ut_StopVideo::...g_hThread");
      g_bExitThread = true;

      //Kiki
      #ifdef LELTEK_WIN32
            WaitForSingleObject(g_hThread, INFINITE);
            CloseHandle(g_hThread);
            g_hThread = NULL;
            WaitForSingleObject(g_hThread, INFINITE);
      #else  //Linux
            pthread_join (g_hThread, NULL);
            g_hThread = NULL;
      #endif
    }
    ut_exitTcp();
}

LELAPI_API int lelapi_detect() {
    bool status;
    BYTE ReadData = lelapi_read_byte(probe_id_addr,&status);
    if(!status) {
        ReadData = -1;
    }
    return ReadData;
}

LELAPI_API bool lelapi_set() {
    bool status = false;
    ostringstream buf;
    g_probeID = lelapi_detect();
    buf << "./cfg/" << hex << std::uppercase << g_probeID << "/";
    g_dirname = buf.str();
//showMsg("g_dirname=%s\n", g_dirname.c_str());
    if(g_probeID>=0) {
        status = lelapi_load_rtables();
    }
    if(status) status = lelapi_load_reg(g_dirname+"init.txt");
    if(status) status = lelapi_load_reg(g_dirname+"bMode.txt");
    return status;
}

LELAPI_API bool lelapi_ImgData(UINT *pBufU, UINT &CntU, BYTE *pBufB, UINT &CntB, BYTE *pBufC, UINT &CntC, BYTE *pBufP, UINT &CntP) {
    CntU=0;
    CntP=0;
    if(pushIndexImg!=popIndexImg) {
        //memcpy(pBufB,g_bModeImg[popIndexImg],lenBMode);
        for(int i=0;i<lenBMode;i++) {
            pBufB[i] = grayMapping(g_bModeImg[popIndexImg][i],g_grayMapIndex);
        }
        if(g_cModeVld[popIndexImg]) {
            memcpy(pBufC,g_cModeImg[popIndexImg],lenCMode);
            g_cModeVld[popIndexImg]=false;
            CntC = 1;
        } else {
            CntC = 0;
        }
        updateInfo(&(g_frameHeader[popIndexImg]));
        popIndexImg = (popIndexImg+1)%numImg;
        CntB = 1;
    } else {
        CntB = 0;
        CntC = 0;
    }
    return true;
}

LELAPI_API bool lelapi_BModeSetting(bool uncompressed) {
    // TODO: set reg_cfout_16 = 1 (or 0)
    return true;
}

LELAPI_API bool lelapi_CModeSetting(bool enable) {
    bool status;
    if(enable) {
        status = lelapi_load_reg(g_dirname+"cMode.txt");
    } else {
        status = lelapi_load_reg(g_dirname+"bMode.txt");
    }
    return status;
}

LELAPI_API bool lelapi_LNA(bool gain) {// True=24dB, False=18dB
    // TODO: LNA setting
    return true;
}

LELAPI_API bool lelapi_Power(bool power) {// True=30V, False=20V
    bool status;
    if(power) {
        status=lelapi_write_byte(power_ctl_addr,0x24);
    } else {
        status=lelapi_write_byte(power_ctl_addr,0x0); // check with Thomas and Ted
    }
    return status;
}

LELAPI_API bool lelapi_start() {
    bool status;

//    if (g_hThread) {
//        status = lelapi_stop();
//    }

    pushIndexImg = 0;
    pushIndexImgInc = 1;
    popIndexImg = 0;

    g_bExitThread = false;

    for(int i=0;i<numImg;i++) {
        for(int j=0;j<lenBMode;j++) {
            g_bModeImg[i][j]=0;
        }
        for(int j=0;j<lenCMode;j++) {
            g_cModeImg[i][j]=128;
        }
        g_cModeVld[i]=false;
    }

    if (!g_hThread) { // luke: 2018-01-10: create thread only if the thread of MonitorProc() does not exist
    //Kiki
#ifdef LELTEK_WIN32
    DWORD dwThreadID;
    g_hThread = CreateThread(NULL, 0, MonitorProc, NULL, 0, &dwThreadID);
#else  //Linux
    pthread_create (&g_hThread,NULL, MonitorProc, NULL);
#endif
    }

    status=lelapi_write_byte(tx_en_addr, tx_en_start); // reg_tx_en=1
    return true;
}

LELAPI_API bool lelapi_stop() {
    bool status = lelapi_write_byte(tx_en_addr, tx_en_stop); // reg_tx_en=0
    return status;
}

LELAPI_API float lelapi_getTemperature() {
    return fpgaTemperature;
}

LELAPI_API int lelapi_getBatteryCapacity() {
    return batteryCapacity;
}

LELAPI_API int lelapi_getButtonStatus() {
    return buttonStatus;
}

LELAPI_API bool lelapi_getButtonRise() {
    bool status;
    return (lelapi_read_byte(button_addr,&status)& (1<<button_rise_bit))!=0;
}

LELAPI_API bool lelapi_clearButtonRise() {
    bool status = lelapi_write_byte(button_addr, 1<<button_rise_bit); // write 1 to clear
    return status;
}

LELAPI_API bool lelapi_getButtonFall() {
    bool status;
    return (lelapi_read_byte(button_addr,&status)& (1<<button_fall_bit))!=0;
}

LELAPI_API bool lelapi_clearButtonFall() {
    bool status = lelapi_write_byte(button_addr, 1<<button_fall_bit); // write 1 to clear
    return status;
}

LELAPI_API void lelapi_setGrayMapIndex(int index) {
    g_grayMapIndex = index;
}
