#pragma once

//Kiki 201801
void (*lelapi_output_message) (char *msg) = NULL;


 //Kiki 201801: change PCHAR to const char* for iOS compiler need "const"
void showMsg(const char* DebugMessage, ...)
{
    va_list args;

    va_start(args, DebugMessage);

    char buf[2048];  //Kiki 201801: change MAX_PATH to 2048

    vsprintf(buf, DebugMessage, args);

    //Kiki 201801
    #if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(__BORLANDC__)
       //Kiki vs2013 add "A" after API name
       OutputDebugStringA(buf);
    #endif

    //Kiki 201801
    if ( lelapi_output_message != NULL )
      (*lelapi_output_message) (buf);

    va_end(args);
}

