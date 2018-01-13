#pragma once
//---------------------------------------------------------------------------

#ifndef utCtrlH
#define utCtrlH
//---------------------------------------------------------------------------

#   pragma pack(1)

struct InfoParam {
    double frameRate;
    WORD hwTemp;
    WORD vBat;
    WORD bModeDepth;
    WORD bModeWidth;
    BYTE probeId;
    BYTE bModeDec16;
};

struct UtVideoHeader
{
	DWORD sync; //00 FF FF 00
   	WORD seqNo;
	WORD len;
	DWORD lenRead;
	BYTE dummy_12[4];
	WORD idle_vbat;
	WORD init_vbat;
	WORD bmode_vbat;
	WORD cf_vbat;
	BYTE dummy_24[8];
	// bit 511:256
	BYTE tx_imgblock;	//
	BYTE size_imgblock;	//
	BYTE rx_demod_freq :4;	// [3:0]
	BYTE dummy_bits1 :4;	// [7:4]
	BYTE bmode_on :1;	// [0]=bmode_on
	BYTE cf_on :1;		// [1]=cf_on
	BYTE pw_on :1;		// [2]=pw_on
	BYTE tx_cycles :4;	// [6:3]=tx_cycles
	BYTE reg_pw_cntdop;	// [7:0]
	BYTE reg_cf_cntdop;	// [7:0]
	BYTE reg_xdc_type;	// [1:0]
	BYTE tx_cntdop;		// [7:0]
	DWORD timer_global;	// [31:0]
	WORD measured_temp;	// [15:0]
	WORD measured_vbat;	// [15:0]
	BYTE button;
	BYTE last_imgblock :1;	// [0]
	BYTE dummy_bits2 :7;	// [7:1]
	BYTE bmode_decimation_new :4;// [3:0]
	BYTE cf_decimation_new :4;// [7:4]
	BYTE g_sensor;		// [7:0]
	BYTE alarm;
	BYTE vbus;
	BYTE dummy_byte1;
	BYTE dummy_byte2;
	BYTE reg_trx_imgfull;	// 
	BYTE b_mode :1;		// [0]
	BYTE tx_angle :7;	// [7:1]
	BYTE cf_mode :1;	// [0]
	BYTE reg_cf_cfenq :1;	// [1]
	BYTE reg_cf_pwrenq :1;	// [2]
	BYTE reg_cf_resolution :1;// [3]
	BYTE reg_bmode_decimation :2;// [5:4]
	BYTE reg_cf_decimation :2;	// [7:6]
	BYTE pw_mode :1;	// [0]
	BYTE reg_cf_out16;	// [0]
	BYTE probe_id;		// [4:0]
	WORD RevID_usb;		// [15:0]
};

struct UtRequestHeader
{
	BYTE	cmd;
	BYTE	seqNo;	
	WORD	dataLen;
};
			
struct UtResponseHeader
{
	BYTE	status;
	BYTE	seqNo;
	WORD	dataLen;
};

//
struct UtReadRegRequest
{
	UtRequestHeader header;
	WORD	startAddr;
	WORD	len;
    BYTE	dummy[8];
};

struct UtReadRegResponse
{
	UtResponseHeader header;
	BYTE	data[0];
};

//
struct UtWriteRegRequest
{
	UtRequestHeader header;
	WORD	startAddr;
	WORD	len;
	BYTE	data[0];
};

struct UtWriteRegResponse
{
	UtResponseHeader header;
};

enum utCmd
{
    CMD_READ_REG = 0x0,
    CMD_WRITE_REG = 0x1,
    CMD_WRITE_BLOCK = 0x2,
};

enum eTableAddr
{
  R_TABLE_ID = 0,
  TX_PATTERN_IDTX_PATTERN_ID = 0x4000,
  TGC_B_ID = 0x4100,
  TGC_C_ID = 0x4140,
  GRAY256_ID = 0x4200,
  TX_PATTERN32_B_ID = 0x5000,
  TX_PATTERN32_C_ID = 0x6000,
  TZ_TABLE_ID = 0x8000,
  TX_TABLE_ID = 0xC000,
  TX_TABLE_LINEAR_ID = 0xC000,
  TX_TABLE_CONVEX_ID = 0xD000,
  TX_TABLE_TX_ID = 0xE000,
  TX_TABLE_RX_ID = 0xE800
};



#   pragma pack()

class UtCtrl {

public:
    bool regRead(WORD startAddr, BYTE *pinBuf, WORD readLen);
    bool regWrite(WORD startAddr, BYTE *poutBuf, WORD writeLen);
    bool blockWrite(WORD startAddr, BYTE *poutBuf, WORD writeLen);
    bool hwReset(void);

private:

};


#endif
