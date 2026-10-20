// MCBCtrl.h: interface for the MCBCtrl.dll export functions.
//
//////////////////////////////////////////////////////////////////////

#if !defined(MCB_PARAMETER_H__INCLUDED_)
#define MCB_PARAMETER_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)

#else

#include "replenish_type.hpp"
#include "memory.h"
#endif
// define absolute maximum parameter 
#define  MAX_MCBNUM (8)
#define  MAX_DBLINENUM (4)

#define  MAX_MCBADNUM (64)
#define  MAX_DMROWNUM (128)
#define  MAX_DMCOLNUM (8)
#define  MAX_ADMChnum (MAX_MCBADNUM*MAX_DMROWNUM*MAX_DMCOLNUM)
#define  MAX_PTFDIV (32)

#define  FNDM_ROW 8
#define  FNDM_COLUMN 6
#define  FNDM_DACPERCHN 2
#define  FNDM_EXDAC 2
#define  FND_DACBIT 12   // must be 8~16
// define absolute maximum parameter 

//macro define of mcb command for reserve
#define MCB_CMD_CONNECTMCB    (0x0101)
#define MCB_CMD_SETMODE       (0x0200)
#define MCB_CMD_GETDMCHNNUM   (0x0500) 
#define MCB_CMD_GETDMEXINFO   (0x0508) 
#define MCB_CMD_GETDMADBIT    (0x0510) 
//macro define of mcb command for reserve

//macro define of mcb command for debug
#define MCB_CMD_AOUTDASET   (0x8100) 
#define MCB_CMD_TESTMODE    (0x8200) 
//macro define of mcb command for debug

//#define INVALID_DATA_PACKET 6
#define INVALID_DATA_PACKET_NUM (6)
// define MCB DATA Struct
#define MCB_DATA_H_LENGTH (40)

//*******************  MCB Status Message Code Define  ****************************
/*
Status Code:
0x0100-- connecting...
0x0101-- connection established
0x0102-- connection closed
0x0110-- connection fail
0x0111-- socket fail

0x0200-- MB fpga configure OK
0x0201-- ADMB fpga configure OK
0x0204-- Register Set OK
0x0210-- fpga configure file Error
0x0211-- MB fpga configure Error
0x0214-- Register Set Error
0x0300-- Start OK
0x1110-- Receive "set mode or register" cmd when MCB is running
0x1111-- Receive "start" cmd when MCB is running
0x1210-- Receive Hot packet overtime(running)
0x1211-- Receive Hot packet overtime(stop)
0x1300-- No detector module connected
0x1301-- No trigger source
0x1302-- Detect connected detector module over flow
0x1303-- No data
0x1304-- Send data error
0x1400-- Pulse leak, trigger pulse number is different from socket packet number
0x1500-- DetectorModules change range to mode0
0x1501-- DetectorModules change range to mode0

Error Code:
0x0102-- connection closed
0x0110-- connection fail
0x0111-- socket fail
0x0210-- fpga configure file Error
0x0211-- MB fpga configure Error
0x0214-- Register Set Error
0x1010-- Receive "start, set mode or register" cmd when device not ready */
#define  MCB_STSCD_CONNECTING      (0x0100)
#define  MCB_STSCD_CONNECTOK       (0x0101)
#define  MCB_STSCD_DATACONNECTOK   (0x0101)
#define  MCB_STSCD_CONNECTCLOSE    (0x0102)
#define  MCB_STSCD_CTRLCONNECTOK   (0x0103)

#define  MCB_STSCD_CONNECTFAIL     (0x0110)
#define  MCB_STSCD_SOCKETFAIL      (0x0111)
#define  MCB_STSCD_MBFPGACFG_OK    (0x0200)
#define  MCB_STSCD_ADFPGACFG_OK    (0x0201)
#define  MCB_STSCD_REGSET_OK       (0x0204)
#define  MCB_STSCD_FPGAFILE_ERR    (0x0210)
#define  MCB_STSCD_MBFPGACFG_ERR   (0x0211)
#define  MCB_STSCD_REGSET_ERR      (0x0214)
#define  MCB_STSCD_CANSELFTEST_OK  (0x0220) 
#define  MCB_STSCD_CANSELFTEST_ERR (0x0221) 
#define  MCB_STSCD_STARTOK         (0x0300)
#define  MCB_STSCD_SETREGATRUN     (0x1110)
#define  MCB_STSCD_STARTRUN        (0x1111)
//
#define  MCB_STSCD_SETNEWIPRUN     (0x1121)
#define  MCB_STSCD_SETNEWIP_ERR    (0x1122)
#define  MCB_STSCD_SETNEWIP_OK     (0x1123)
//
#define  MCB_STSCD_NOHOTPKTRUN     (0x1210)
#define  MCB_STSCD_NOHOTPKTSTOP    (0x1211)
#define  MCB_STSCD_NODM            (0x1300)
#define  MCB_STSCD_NOTRIGGER       (0x1301)
#define  MCB_STSCD_MISSAA55_ERROR  (0x1306)   // V2.0.1.1: miss 0xAA55 lead code exceeds the limit
#define  MCB_STSCD_DMOVERFLOW      (0x1302)
#define  MCB_STSCD_NODATA          (0x1303)
#define  MCB_STSCD_SENDERR         (0x1304)
#define  MCB_STSCD_DMNUM_ERROR     (0x1305)
#define  MCB_STSCD_SCNTPCNTDIFF    (0x1400)
#define  MCB_STSCD_CHANGE_RANGE0   (0x1500)
#define  MCB_STSCD_CHANGE_RANGE1   (0x1501)

#define  MCB_ERRCD_CONNECTCLOSE    (0x0102)
#define  MCB_ERRCD_CONNECTFAIL     (0x0110)
#define  MCB_ERRCD_SOCKETFAIL      (0x0111)
#define  MCB_ERRCD_FPGAFILE_ERR    (0x0210)
#define  MCB_ERRCD_MBFPGACFG_ERR   (0x0211)
#define  MCB_ERRCD_REGSET_ERR      (0x0214)
#define  MCB_ERRCD_DVNOREADY       (0x1010)
//#define  MCB_ERRCD_DMNUM_ERROR     (0x1020)
//*******************  MCB Status Message Code Define  ****************************

typedef union tagUl_Uc
{
	UINT ul;
	unsigned char uc[4];
} Ul_Uc;

typedef struct tagMCBData_H_USE
{
	LONG scounter;  // socket packet serial number, self-add
	union PCNT // pulse trigger number
	{
		LONG l;
		unsigned char uc[4];
	}pcounter;
	LONG accEnergy; // acc energy sign
	Ul_Uc ain0; // ain0 data
	UINT ptsmdata_l;// exdata lower(ptsm)
	UINT ptsmdata_h;// exdata upper(ptsm)
	Ul_Uc ain1; // ain1 data
	LONG sscnt;     // socket packet serial number, from packet
	UINT regError_DI;     
} MCBData_H_USE;

typedef struct tagMCBData_H
{
    MCBData_H_USE head_use;
    unsigned char rsvch[MCB_DATA_H_LENGTH - sizeof(MCBData_H_USE)];
} MCBData_H;

typedef struct tagMCBData
{
	MCBData_H d_head;
	Ul_Uc addata[MAX_MCBADNUM*MAX_DMROWNUM*MAX_DMCOLNUM];
	tagMCBData()
	{
		memset(this,0,sizeof(*this));
	} 

} MCBData;

typedef struct tagDMEXINFO
{
	UINT nMCB;       //MCB serial number
	UINT nFBlkRowNum;    //Blank Row of first Detector Module
	UINT nLBlkRowNum;    //Blank Row of last Detector Module
	UINT nAllValidChnNum; //all valid channel number
} DMEXINFO;


#endif // !defined(MCB_PARAMETER_H__INCLUDED_)
