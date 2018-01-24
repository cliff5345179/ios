// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the LELAPI_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// LELAPI_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.


//Kiki
#ifndef __LELAPI_H__
#define __LELAPI_H__

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(__BORLANDC__)

  //Kiki vs2013
  #ifdef LELAPI_WIN_EXE
      #define LELAPI_API
  #else

    #ifdef LELAPI_EXPORTS
    #define LELAPI_API __declspec(dllexport)
    #else
    #define LELAPI_API __declspec(dllimport)
    #endif

  //Kiki vs2013
  #endif

#else  //Linux
  #ifndef LELAPI_API
  #define LELAPI_API
  #endif

   #ifndef BYTE
   #define BYTE unsigned char
   #endif
   #ifndef WORD
   #define WORD unsigned short int
   #endif
   #ifndef DWORD
   #define DWORD unsigned int
   #endif
   #ifndef UINT
   #define UINT unsigned int
   #endif

#endif //if win32
//Kiki

extern "C" LELAPI_API bool lelapi_init();
extern "C" LELAPI_API void lelapi_exit();
extern "C" LELAPI_API int lelapi_detect();
extern "C" LELAPI_API bool lelapi_set();
extern "C" LELAPI_API bool lelapi_ImgData(UINT *pBufU, UINT &CntU, BYTE *pBufB, UINT &CntB, BYTE *pBufC, UINT &CntC, BYTE *pBufP, UINT &CntP);
extern "C" LELAPI_API bool lelapi_BModeSetting(bool uncompressed);
extern "C" LELAPI_API bool lelapi_CModeSetting(bool enable);

extern "C" LELAPI_API bool lelapi_LNA(bool gain); // True=24dB, False=18dB
extern "C" LELAPI_API bool lelapi_Power(bool power); // True=30V, False=20V

extern "C" LELAPI_API bool lelapi_start();
extern "C" LELAPI_API bool lelapi_stop();

extern "C" LELAPI_API float lelapi_getTemperature(); // in the unit of degree Celsius
extern "C" LELAPI_API int lelapi_getBatteryCapacity(); // 0~100: when not charged, show battery capacity %; 200: charging
extern "C" LELAPI_API int lelapi_getButtonStatus(); // 0: button is released; 1: button is pressed
extern "C" LELAPI_API bool lelapi_getButtonRise(); // 0: not yet pressed or the press status is cleared; 1: button was pressed and not yet cleared
extern "C" LELAPI_API bool lelapi_clearButtonRise(); // clear the pressed status
extern "C" LELAPI_API bool lelapi_getButtonFall(); // 0: not yet released or the released status is cleared; 1: button was released and not yet cleared
extern "C" LELAPI_API bool lelapi_clearButtonFall(); // clear the released status

extern "C" LELAPI_API void lelapi_setGrayMapIndex(int index); // valid range of "index": 0~7

//Kiki
#endif //ifndef __LELAPI_H__

