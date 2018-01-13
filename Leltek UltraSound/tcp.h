#pragma once

//Kiki
#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(__BORLANDC__)
  #ifndef LELTEK_WIN32
  #define LELTEK_WIN32
  #endif
#endif



//Kiki
#ifdef  LELTEK_WIN32

   #undef UNICODE

   #define WIN32_LEAN_AND_MEAN

   #include <windows.h>
   #include <winsock2.h>
   #include <ws2tcpip.h>
   #include <stdlib.h>
   #include <stdio.h>

   //Kiki 20171111
   #include <time.h>

    // Need to link with Ws2_32.lib
    #pragma comment (lib, "Ws2_32.lib")
    #pragma comment (lib, "Mswsock.lib")
    #pragma comment (lib, "AdvApi32.lib")

#else //Linux

   #include <stdio.h>
   #include <string.h>
   #include <stdlib.h>
   #include <errno.h>
   #include <signal.h>
   #include <errno.h>
   #include <time.h> //for nanosleep ()

   #include <unistd.h>
   #include <sys/types.h>  /* /usr/kvm/sys/sys  */
   #include <sys/time.h>
   #include <sys/socket.h>
   #include <netinet/in.h>
   #include <netinet/tcp.h>
   #include <arpa/inet.h>
   #include <fcntl.h>
   #include <netdb.h>

   //fill hole of windows
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

   #define MAKEWORD(a,b)   ((WORD)(((BYTE)(a))|(((WORD)((BYTE)(b)))<<8)))

   #define SOCKET int

   #define SOCKET_ERROR -1

   typedef struct WSAData {int none; } WSADATA;

   inline int WSAStartup (WORD a, WSAData* b) { return 0; }
   inline int WSAGetLastError (void) { return errno; }

   #define closesocket close
   #define ADDRLENPREFIX  (socklen_t *)

   inline void WSACleanup (void) {}
   #define INVALID_SOCKET  (SOCKET)(~0)

   inline void ZeroMemory (void *p, size_t len)  {   memset (p, 0, len); }

   inline void Sleep (int len)
    {
              struct timespec kperiod = {0, 1000000L*len};
              nanosleep (&kperiod, NULL);
    }

   inline void showMsg (const char *prefix, ...) { }





//Kiki UNF
#ifndef LELAPI_API
#define LELAPI_API
#endif



#endif  //Linux //Kiki





extern "C" LELAPI_API int ut_tcpSendCtrl(BYTE *buf, int len);
extern "C" LELAPI_API int ut_tcpRecvCtrl(BYTE *buf, int len);
extern "C" LELAPI_API int ut_tcpRecvImage(BYTE *buf, int len);
extern "C" LELAPI_API bool ut_tcpReadReg(WORD adr, BYTE *buf, WORD len);
extern "C" LELAPI_API bool ut_tcpWriteReg(WORD adr, BYTE *buf, WORD len);
extern "C" LELAPI_API bool ut_tcpWriteTable(WORD adr, BYTE *buf, WORD len);
extern "C" LELAPI_API bool ut_tcpReadImage(UINT *pHeader, UINT *buf, UINT maxLen, WORD *pLen);





#define TCP_MAX_LEN 1460
#define TCP_MAX_PAYLOAD (1460-sizeof(TcpPktBase))
#define TCP_CMD_ENC(cmd) (cmd|((~cmd)&0xFF)<<8)

typedef enum  eTcpCmd {
  TableReq = TCP_CMD_ENC(0xA5), // R-Table request
  TableAck = TCP_CMD_ENC(0x5A), // R-Table acknowledge
  WriteReq = TCP_CMD_ENC(0x42), // Register write request
  WriteAck = TCP_CMD_ENC(0x24), // Register write acknowledge
  ReadReq  = TCP_CMD_ENC(0x81), // Register read request
  ReadAck  = TCP_CMD_ENC(0x18), // Register read acknowledge
  ImageReq = TCP_CMD_ENC(0xC3), // Image request
  ImageAck = TCP_CMD_ENC(0x3C), // Image acknowledge (not used)
  PingReq  = TCP_CMD_ENC(0x96), // Ping request
  PingAck  = TCP_CMD_ENC(0x69), // Ping acknowledge
  CfgReq   = TCP_CMD_ENC(0xE7), // Cfg request
  CfgAck   = TCP_CMD_ENC(0x7E), // Cfg acknowledge
  Undefined = TCP_CMD_ENC(0x00) // undefined command
} eTcpCmd;

typedef enum  eTcpCmdCfg {
  TcpCmdCfgChannel      = 0,                    // channel, refer to http://www.cisco.com/en/US/products/ps6305/products_configuration_guide_chapter09186a00804ddd8a.html
  TcpCmdCfgSsid         = 1,                    // the name of SSID
  TcpCmdCfgCountry      = 2,                    // country code
} eTcpCmdCfg;

typedef struct {
  WORD  len;
  WORD  cmd;
  WORD  seq;
  WORD  adr;
  BYTE  payload[0];
} TcpHeader;

typedef struct {
  BYTE  cmd;
  BYTE  seq;
  BYTE  read;
  BYTE  data;
} TcpImageHeader;

class TcpPktBase {
public:
  TcpPktBase(eTcpCmd cmd, WORD len = 0, WORD adr = 0, BYTE *payload = NULL) {
    if (cmd == Undefined) {
      len = TCP_MAX_PAYLOAD;
    }
    pkt = (TcpHeader *)new BYTE[sizeof(TcpHeader) + len];
    pkt->len = len;
    pkt->cmd = cmd;
    pkt->adr = adr;
    if (cmd == Undefined) {
      pkt->seq = -1;
    }
    else {
      pkt->seq = seq++;
    }
    if (payload != NULL) {
      memcpy(pkt->payload, payload, len);
    }
  }
  int send() {
    WORD len = sizeof(TcpHeader) + pkt->len;
    int status = ut_tcpSendCtrl((BYTE *)pkt, len);
    if (len == status) {
      return 0;
    }
    else {
      return -1;
    }
  }
  int recv() {
    WORD len = pkt->len;
    bool isImage = (pkt->cmd == ImageReq) || (pkt->cmd == ImageAck);
    int status = isImage ? ut_tcpRecvImage((BYTE *)pkt, sizeof(TcpHeader)) : ut_tcpRecvCtrl((BYTE *)pkt, sizeof(TcpHeader));
    //  int status = true ? ut_tcpRecvImage((BYTE *)pkt,sizeof(TcpHeader)) : ut_tcpRecvCtrl((BYTE *)pkt,sizeof(TcpHeader));
    if (status != sizeof(TcpHeader)) return -1;
    if (pkt->len == 0) return 0;
    if (pkt->len > len) {
      // coming packet is larger than allocated size, resize pkt.
      TcpHeader *newPkt = (TcpHeader *)new BYTE[sizeof(TcpHeader) + pkt->len];
      memcpy(newPkt, pkt, sizeof(TcpHeader));
      delete pkt;
      pkt = newPkt;
    }
    status = isImage ? ut_tcpRecvImage(pkt->payload, pkt->len) : ut_tcpRecvCtrl(pkt->payload, pkt->len);
    //  status = true ? ut_tcpRecvImage(pkt->payload, pkt->len) : ut_tcpRecvCtrl(pkt->payload, pkt->len);
    if (status != pkt->len)
      return -1;
    else
      return 0;
  }
  void print() {
    showMsg("TcpPktBase %x, len=%d, cmd=%x, seq=%x, adr=%x", this, pkt->len, pkt->cmd, pkt->seq, pkt->adr);
    int len = pkt->len;
    if (len > 21) len = 21;
    for (int i = 0; i < len; i++) {
      showMsg("[%d]=%x", i, pkt->payload[i]);
    }
  }
  TcpHeader *pkt;
  static WORD seq;
  ~TcpPktBase() {
    delete pkt;
  }
};

class TcpPktTableReq : public TcpPktBase {
public:
  TcpPktTableReq(WORD len, WORD adr, BYTE *data = NULL) : TcpPktBase(TableReq, len, adr, data) {
  }
};

class TcpPktTableAck : public TcpPktBase {
public:
  TcpPktTableAck(WORD adr) : TcpPktBase(TableAck, 0, adr) {
  }
};

class TcpPktWriteReq : public TcpPktBase {
public:
  TcpPktWriteReq(WORD len, WORD adr, BYTE *data = NULL) : TcpPktBase(WriteReq, len, adr, data) {
  }
};

class TcpPktWriteAck : public TcpPktBase {
public:
  TcpPktWriteAck(WORD adr) : TcpPktBase(WriteAck, 0, adr) {
  }
};

class TcpPktReadReq : public TcpPktBase {
public:
  TcpPktReadReq(WORD len, WORD adr) : TcpPktBase(ReadReq, 2, adr, (BYTE *)&len) {
  }
};

class TcpPktReadAck : public TcpPktBase {
public:
  TcpPktReadAck(WORD len, WORD adr, BYTE *data = NULL) : TcpPktBase(ReadAck, len, adr, data) {
  }
};

WORD TcpPktBase::seq = 0;

class MyWinSock {
public:

  //Kiki
  MyWinSock ()
   {
     ConnectSocket = INVALID_SOCKET;
     result = NULL;
   }

  int sendPkt(BYTE *sendBuf, int sendLen, bool wait = true) {

    //Kiki: should initialize to 0
    int iResult = 0;

    int residueLen = sendLen;
    //showMsg("MyWinSock::sendPkt %d bytes.", sendLen);
    if (ConnectSocket == INVALID_SOCKET) {
      showMsg("MyWinSock::ConnectSocket is not valid in sendPkt!");
    }
    else {
      do {
        //showMsg("MyWinSock::sendPkt.send %d bytes.", residueLen);

        //Kiki  add (int)
        iResult = (int) send(ConnectSocket, (char *)sendBuf, residueLen, 0);

        //showMsg("MyWinSock::sendPkt.sent %d bytes.", iResult);
        if (iResult == SOCKET_ERROR) {
          int nError = WSAGetLastError();

      //Kiki
      #ifdef  LELTEK_WIN32
          if (nError != WSAEWOULDBLOCK&&nError != 0)
      #else
          if (nError != EAGAIN && nError !=  EWOULDBLOCK &&  nError != 0)
      #endif
          {
      //Kiki
      #ifdef  LELTEK_WIN32
            if (nError == WSAECONNRESET) {
              showMsg("send failed with error: %d: An existing connection was forcibly closed by the remote host.\n", nError);
            }
            else if (nError == WSANOTINITIALISED) {
              showMsg("send failed with error: %d: Either the application has not called WSAStartup, or WSAStartup failed.\n", nError);
            }
            else
      #endif
              showMsg("send failed with error: %d\n", nError);

            closesocket(ConnectSocket);
            WSACleanup();
            break;
          }
          else {
            Sleep(1);
          }
        }
        else {
          if (iResult > residueLen) {
            showMsg("send too much: expected %d bytes, sent %d bytes.\n", residueLen, iResult);
          }
          residueLen -= iResult;
          sendBuf += iResult;
          //                showMsg("Bytes Sent: %ld\n", iResult);
        }
      } while (wait && (residueLen > 0));
    }
    return iResult;
  }
  int recvPkt(BYTE *recvBuf, int recvLen, bool wait = true) {

    //Kiki: should initialize to 0
    int iResult = 0;

    int residueLen = recvLen;
    //showMsg("MyWinSock::[%d] recvPkt %d bytes.", ConnectSocket, recvLen);
    if (ConnectSocket == INVALID_SOCKET) {
      showMsg("MyWinSock::ConnectSocket is not valid in recvPkt!");
    }
    else {
      do {
        //showMsg("MyWinSock::recvPkt.recv %d bytes.", residueLen);

        //Kiki  add (int)
        iResult = (int) recv(ConnectSocket, (char *)recvBuf, residueLen, 0);

        //showMsg("MyWinSock::recvPkt.received %d bytes: %x %x %x %x", iResult,recvBuf[0],recvBuf[1],recvBuf[2],recvBuf[3]);
        if (iResult == SOCKET_ERROR) {
          int nError = WSAGetLastError();
          //showMsg("MyWinSock::recvPkt.WSAGetLastError() = %d", nError);

        //Kiki
        #ifdef  LELTEK_WIN32
          if (nError != WSAEWOULDBLOCK&&nError != 0)
        #else
          if (nError != EAGAIN && nError !=  EWOULDBLOCK &&
              nError != 0)
        #endif
          {
            showMsg("recv failed with error: %d\n", nError);
            closesocket(ConnectSocket);
            WSACleanup();
            break;
          }
          else {
            // check if socket connected by zero-length receive

            //Kiki: add (int)
            iResult = (int) recv(ConnectSocket, (char *)recvBuf, 0, 0);

            int nError = WSAGetLastError();
            //                  showMsg("recv zero iResult=%d, nError=%d\n", iResult, nError);
            if (iResult == SOCKET_ERROR) {

            //Kiki
            #ifdef  LELTEK_WIN32
              if (nError != WSAEWOULDBLOCK&&nError != 0)
            #else
              if (nError != EAGAIN && nError !=  EWOULDBLOCK &&  nError != 0)
            #endif
              {
                showMsg("recv failed with error 2nd time: %d\n", nError);
                closesocket(ConnectSocket);
                WSACleanup();
                break;
              }
              else {
                Sleep(1);
              }
            }
          }
        }
        else {
          if (iResult > residueLen) {
            showMsg("recv too much: expected %d bytes, received %d bytes.\n", residueLen, iResult);
          }
          residueLen -= iResult;
          recvBuf += iResult;
          //                showMsg("Bytes Recv: %ld\n", iResult);
        }
      } while (wait && (residueLen > 0));
    }
    return iResult;
  }
  WSADATA wsaData;

  //Kiki
  /*
  SOCKET ConnectSocket = INVALID_SOCKET;
  struct addrinfo *result = NULL;
  */
  SOCKET ConnectSocket;
  struct addrinfo *result;

  //    struct addrinfo *ptr = NULL;
  struct addrinfo hints;
  ~MyWinSock() {
    if (ConnectSocket == INVALID_SOCKET) {
      showMsg("MyWinSock::ConnectSocket is not valid in ~MyWinSock!");
    }
    else {
      closesocket(ConnectSocket);
      WSACleanup();
    }
  }
};

class MyWinSockServer : public MyWinSock {
public:
  MyWinSockServer(char *IpAddr, char * port) {
    // Initialize Winsock
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult) {
      showMsg("WinSock.WSAStartup fail with error: %d", iResult);

      //Kiki
      ListenSocket = INVALID_SOCKET;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(IpAddr, port, &hints, &result);
    if (iResult != 0) {
      printf("getaddrinfo failed with error: %d\n", iResult);
      WSACleanup();
      return;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
      printf("socket failed with error: %d\n", WSAGetLastError());   //Kiki: fix %ld to %d
      freeaddrinfo(result);
      WSACleanup();
      return;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
      printf("bind failed with error: %d\n", WSAGetLastError());
      freeaddrinfo(result);
      closesocket(ListenSocket);
      WSACleanup();
      return;
    }

    freeaddrinfo(result);

    showMsg("MyWinSockServer: waiting for client to connect.");

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
      printf("listen failed with error: %d\n", WSAGetLastError());
      closesocket(ListenSocket);
      WSACleanup();
      return;
    }

    showMsg("MyWinSockServer: accept client connection.");

    // Accept a client socket
    ConnectSocket = accept(ListenSocket, NULL, NULL);
    if (ConnectSocket == INVALID_SOCKET) {
      printf("accept failed with error: %d\n", WSAGetLastError());
      closesocket(ListenSocket);
      WSACleanup();
      return;
    }

    showMsg("MyWinSockServer: connected.");

    // No longer need server socket
    closesocket(ListenSocket);
  }
  //Kiki
  //SOCKET ListenSocket = INVALID_SOCKET;
  SOCKET ListenSocket;
};


  //Kiki 20171111  This class is re-written
  // --------------------------------------------------------
class MyWinSockClient : public MyWinSock {
public:
  MyWinSockClient(char *IpAddr, char * port) {
    // Initialize Winsock
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult) {
      showMsg("WinSock.WSAStartup fail with error: %d", iResult);
    }
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    // Resolve the server address and port
    iResult = getaddrinfo(IpAddr, port, &hints, &result);
    if (iResult != 0) {
      showMsg("getaddrinfo failed with error: %d\n", iResult);
      WSACleanup(); // fail process
//          return 1;
    }
    else {
      // Attempt to connect to an address until one succeeds

        showMsg("Connecting socket %s:%s!\n", IpAddr, port);

          struct addrinfo *ptr = result;

          // Create a SOCKET for connecting to server
          ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);

         if (ConnectSocket == INVALID_SOCKET) {
            showMsg("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup(); // fail process
           }
          else //socket create syccess
           {
               //Kiki
               #ifdef  LELTEK_WIN32
                  unsigned long val = 1;
                  ioctlsocket(ConnectSocket, FIONBIO, &val); /* 變更 socket 1 特性為 Non-Blocking */
               #else
                   fcntl ( ConnectSocket, F_SETFL, O_NONBLOCK | O_ASYNC );
               #endif


               // Connect to server.

               unsigned long starttime, currtime;
               int kret, kcon, kerr;
               starttime= (unsigned long) time (NULL);
               kerr = 0;

               showMsg("Connecting %x...\n", ptr);

               do
                {

                     iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);

                     kcon = iResult;

                     // Following code is copied from MoonSuperb Tech.
                     // (c) MoonSuperb Technology 2017
                     //--------------------------------------------------------
                     #ifdef LELTEK_WIN32

                       kret = WSAGetLastError ();
                       if ( kret == WSAEALREADY )
                          ; //still connecting
                       else if ( kret == WSAEWOULDBLOCK ||  kret == WSAEINVAL )
                          ; //(possible error from WinSock 1.1)   //still connecting
                       else if ( kret == WSAEISCONN )  /* Connection Success */
                        {
                          kcon = 0;  //change to success
                        }
                     #else

                        kret = errno;
                        if ( kret == EALREADY || kret == EINPROGRESS )
                           ;  //still connecting
                        else if ( kret == ECONNREFUSED || kret == EADDRNOTAVAIL ||
                                 kret == ENETUNREACH )
                          {
                            kerr = 1; //failure
                          }
                        else if ( kret == EISCONN )  //2014/08/02 Apple iOS will return kcon <0 with this on success
                           kcon = 0; //change to success
                        else
                           kerr = 1; //failre

                     #endif
                     //--------------------------------------------------------

                     currtime = (unsigned long) time (NULL);

                } while (kcon < 0 && !kerr &&  (currtime - starttime) < 3L );

              freeaddrinfo(result);

              if ( kcon >= 0 )  //success
               {
                        showMsg("Socket connected to %s:%s!\n", IpAddr, port);
               }
              else
               {
                       closesocket(ConnectSocket);
                       ConnectSocket = INVALID_SOCKET;
                        showMsg("Unable to connect to server %s:%s!\n", IpAddr, port);
                        WSACleanup();

               }



           } //else socket create success
    } //else getaddrinfo () success

  } //func
};  //class
  //Kiki 20171111  AboveThis class is re-written
  // --------------------------------------------------------




//MyWinSockServer *ptrTcpSocketCheck;
MyWinSockClient *ptrTcpSocketCtrl, *ptrTcpSocketImage;

LELAPI_API void ut_initTcp() {
  showMsg("utex::ut_initTcp.");
  //    if(ptrTcpSocketCheck!=NULL) {
  //    delete ptrTcpSocketCheck;
  //    }
  //    ptrTcpSocketCheck = new MyWinSockServer(NULL,"5001");

  if (ptrTcpSocketCtrl != NULL) {
    delete ptrTcpSocketCtrl;
  }
  //    ptrTcpSocketCtrl = new MyWinSockClient("192.168.173.102","5001");
  //Kiki: add (char*)
  //ptrTcpSocketCtrl = new MyWinSockClient("192.168.1.1", "5001");
  ptrTcpSocketCtrl = new MyWinSockClient((char*)"192.168.1.1", (char*)"5001");

  if (ptrTcpSocketImage != NULL) {
    delete ptrTcpSocketImage;
  }
  //    ptrTcpSocketImage = new MyWinSockClient("192.168.173.102","5002");

  //Kiki: add (char*)
  //ptrTcpSocketImage = new MyWinSockClient("192.168.1.1", "5002");
  ptrTcpSocketImage = new MyWinSockClient( (char*)"192.168.1.1", (char*)"5002");
}

LELAPI_API void ut_exitTcp() {
  showMsg("utex::ut_exitTcp.");
  //    if(ptrTcpSocketCheck!=NULL) {
  //    delete ptrTcpSocketCheck;
  //    ptrTcpSocketCheck=NULL;
  //    }

  if (ptrTcpSocketCtrl != NULL) {
    delete ptrTcpSocketCtrl;
    ptrTcpSocketCtrl = NULL;
  }

  if (ptrTcpSocketImage != NULL) {
    delete ptrTcpSocketImage;
    ptrTcpSocketImage = NULL;
  }
}

LELAPI_API int ut_tcpSendCtrl(BYTE *buf, int len) {
  if (ptrTcpSocketCtrl == NULL)
    return SOCKET_ERROR;
  else
    return ptrTcpSocketCtrl->sendPkt(buf, len);
}

LELAPI_API int ut_tcpRecvCtrl(BYTE *buf, int len) {
  if (ptrTcpSocketCtrl == NULL)
    return SOCKET_ERROR;
  else
    return ptrTcpSocketCtrl->recvPkt(buf, len);
}

LELAPI_API int ut_tcpRecvImage(BYTE *buf, int len) {
  if (ptrTcpSocketImage == NULL)
    return SOCKET_ERROR;
  else
    return ptrTcpSocketImage->recvPkt(buf, len);
}

LELAPI_API bool ut_tcpWriteTable(WORD adr, BYTE *buf, WORD len) {
//  showMsg("ut_tcpWriteTable[%d]=%x",adr,buf[0]);
      // "len" in bytes
  int iStatus;
  //    for(int i=0;i<len;i++) {
  //showMsg("ut_tcpTableReg[%d]=%x",i,buf[i]);
  //    }
  TcpPktTableReq reqTcpPkt(len, adr, buf);
  //    reqTcpPkt.print();
  iStatus = reqTcpPkt.send();
  if (iStatus != 0) {
    showMsg("ut_tcpWriteTable[%d]=%x fail at send.", adr, buf[0]);
    return false;
  }
  TcpPktBase ackTcpPkt(Undefined);
  iStatus = ackTcpPkt.recv();
  if (iStatus != 0) {
    showMsg("ut_tcpWriteTable[%d]=%x fail at recv.", adr, buf[0]);
    return false;
  }
  if ((ackTcpPkt.pkt->cmd == TableAck) &&
    (ackTcpPkt.pkt->adr == reqTcpPkt.pkt->adr) &&
    (ackTcpPkt.pkt->len == 0) &&
    (ackTcpPkt.pkt->seq == reqTcpPkt.pkt->seq)) {
    //showMsg("ut_tcpWriteTable[%d]=%x done.",adr,buf[0]);
    return true;
  }
  else {
    showMsg("ut_tcpTableReg ack packet format error!");
    reqTcpPkt.print();
    ackTcpPkt.print();
    return false;
  }
}

LELAPI_API bool ut_tcpWriteReg(WORD adr, BYTE *buf, WORD len) {
  showMsg("ut_tcpWriteReg[%d]=%x", adr, buf[0]);
  int iStatus;
  TcpPktWriteReq reqTcpPkt(len, adr, buf);
  //    reqTcpPkt.print();
  iStatus = reqTcpPkt.send();
  if (iStatus != 0) {
    return false;
  }
  TcpPktBase ackTcpPkt(Undefined);
  iStatus = ackTcpPkt.recv();
  if (iStatus != 0) {
    return false;
  }
  if (ackTcpPkt.pkt->cmd == WriteAck &&  //Kiki: remove useless ()
    (ackTcpPkt.pkt->adr == reqTcpPkt.pkt->adr) &&
    (ackTcpPkt.pkt->len == 0) &&
    (ackTcpPkt.pkt->seq == reqTcpPkt.pkt->seq)) {
    return true;
  }
  else {
    showMsg("ut_tcpWriteReg ack packet format error!");
    reqTcpPkt.print();
    ackTcpPkt.print();
    return false;
  }
}

LELAPI_API bool ut_tcpReadReg(WORD adr, BYTE *buf, WORD len) {
  showMsg("ut_tcpReadReg[%d]:", adr);
  int iStatus;
  TcpPktReadReq reqTcpPkt(len, adr);
  iStatus = *(WORD *)reqTcpPkt.pkt->payload;
  if (len != iStatus) {
    showMsg("ut_tcpReadReg gen len error: %d %d", len, iStatus);
  }
  //    reqTcpPkt.print();
  iStatus = reqTcpPkt.send();
  if (iStatus != 0) {
    return false;
  }
  TcpPktBase ackTcpPkt(Undefined);
  iStatus = ackTcpPkt.recv();
  if (iStatus != 0) {
    return false;
  }
  if ((ackTcpPkt.pkt->cmd == ReadAck) &&
    (ackTcpPkt.pkt->adr == reqTcpPkt.pkt->adr) &&
    (ackTcpPkt.pkt->len == len) &&
    (ackTcpPkt.pkt->seq == reqTcpPkt.pkt->seq)) {
    memcpy(buf, ackTcpPkt.pkt->payload, ackTcpPkt.pkt->len);
    showMsg("ut_tcpReadReg ack: %x %x", *ackTcpPkt.pkt->payload, ackTcpPkt.pkt->len);
    return true;
  }
  else {
    showMsg("ut_tcpReadReg ack packet format error!");
    reqTcpPkt.print();
    ackTcpPkt.print();
    return false;
  }
}

LELAPI_API bool ut_tcpReadImage(UINT *pHeader, UINT *buf, UINT maxLen, WORD *pLen) {
  int iStatus;
  UINT copyLen;
  TcpPktBase ackTcpPkt(ImageAck);
  iStatus = ackTcpPkt.recv();
  if (iStatus != 0) {
    return false;
  }

  //Kiki: remove useless ()
  if (ackTcpPkt.pkt->cmd == ImageAck
    //  &&
    //  ackTcpPkt.pkt->len==len &&
    //  ackTcpPkt.pkt->seq==reqTcpPkt.pkt->seq
    ) {


    //showMsg("ut_tcpReadImage ack packet:");
    //ackTcpPkt.print();

    memcpy(pHeader, ackTcpPkt.pkt->payload, sizeof(UINT));
    copyLen = ackTcpPkt.pkt->len - sizeof(UINT);
    if (copyLen > maxLen) {
      copyLen = maxLen;
      showMsg("ut_tcpReadImage too many data!");
    }
    memcpy(buf, ackTcpPkt.pkt->payload + sizeof(UINT), copyLen);
    *pLen = ackTcpPkt.pkt->len - sizeof(UINT);
    return true;
  }
  else {
    showMsg("ut_tcpReadImage ack packet format error!");
    ackTcpPkt.print();
    return false;
  }
}

