  // MCBCtrl.h: interface for the MCBCtrl.dll export functions.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MCBCTRL_H__INCLUDED_)
#define AFX_MCBCTRL_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)

#else
#include "replenish_type.hpp"
#define WM_USER                         0x0400
#endif
// macro define of mcb command
#define MCB_CMD_SETCONFIGFILE       (0x0090)
#define MCB_CMD_INITIAL       (0x0100)
#define MCB_CMD_CLOSE         (0x0110)
#define MCB_CMD_SETRANGE      (0x0204)
#define MCB_CMD_SETRANGEMODE  (0x0205)
#define MCB_CMD_SET2TYPERANGE (0x0206)
#define MCB_CMD_SETPOTMT      (0x0210)
#define MCB_CMD_SETPOTMW      (0x0211)
#define MCB_CMD_SETINTTM      (0x0212)
#define MCB_CMD_SETCTSAMPLE   (0x0220)
#define MCB_CMD_SETPTFDIV     (0x0230)
#define MCB_CMD_SETFNDMPARA   (0x0240)
#define MCB_CMD_GETFNDMPARA   (0x0241)
#define MCB_CMD_SETALLDATA    (0x0250)
#define MCB_CMD_START         (0x0300) 
#define MCB_CMD_STOP          (0x0301) 
#define MCB_CMD_TRIGSET       (0x0400) 
#define MCB_CMD_SETDEMODE     (0x0410) 
#define MCB_CMD_SETNSTYPE     (0x0420) 
#define MCB_CMD_GETMCBNUM     (0x0501) 
#define MCB_CMD_GETDMINFO     (0x0502) 
#define MCB_CMD_SETNEWIP	  (0x0601)
//#define MCB_CMD_DUMPALLECSREG (0x0602)
#define MCB_CMD_DUMPALLECSREG (0x060F)


//////////////////////////////////////////////
// mxg_add for YH's demmand begin
#define MCB_CMD_GETDVREADY                 (0x0602)
#define MCB_CMD_GETRANGE                     (0x0603)
#define MCB_CMD_GETRANGEMODE          (0x0604)   // mxg_add for YH's demmand
#define MCB_CMD_GETPOTMT                     (0x0605)   // mxg_add for YH's demmand
#define MCB_CMD_GETPOTMW                    (0x0606)   // mxg_add for YH's demmand
#define MCB_CMD_GETINTTM                      (0x0607)
#define MCB_CMD_TRIGGET                         (0x0608)
#define MCB_CMD_GETRUNSTATUS             (0x0609)
#define MCB_CMD_GETREADCOUNT            (0x0610)

#define MCB_CMD_SETSOCKETERRORSTRING      (0x0700)
  // mxg_add for YH's demmand end
#define MBC_THREAD_QUIT         0
// macro define of mcb command

// struct define of DMINFO
typedef struct tagDMINFO
{
	UINT nMCB;       //MCB serial number
	UINT nDMNum;     //Detector Module number of the MCB
	UINT nRowNum;    //Row of each Detector Module
	UINT nColNum;    //Column of each Detector Module
	UINT nAllChnNum; //All valid detector channel of the MCB
} DMINFO;
// struct define of DMINFO

// struct define of DMINFO
typedef struct tagDMINFOHMCB4
{
    UINT nMCB;       //MCB serial number
    UINT nDMNum;     //Detector Module number of the MCB
    UINT nDBLineEnNum;  //DB line enable number
    UINT nDBDMNUM[4];//Detector Module number of sub-dbline of the MCB
    UINT nRowNum;    //Row of each Detector Module
    UINT nColNum;    //Column of each Detector Module
    UINT nAllChnNum; //All valid detector channel of the MCB
} DMINFOHMCB4;
// struct define of DMINFO

// struct define of FNDMPARA
typedef struct tagFNDMPARA  //Fast Neutron Module parameter
{
	UINT nChannels;  //channel number of needed to set parameter 
	struct CHPARA   // channel parameter
	{
		UINT nFNDM;  //FNDM serial number
		UINT nType;  //type of parameter, 0-bias, 1-lower threshold, 2-upper threshold
		UINT nRow;  //Row number of the channel, ignorable when type is bias
		UINT nColnum;  //Column number of the channel, ignorable when type is bias
		UINT nAdjMode;  //Adjust parameter mode, 0-absolute value, 1-relative value
		INT nValue;
	}ChannelPara[64*128];  //max channel number is 64(module)*128(channels/module)
} FNDMPARA;
// struct define of FNDMPARA

// macro define of mcb message
#define MCB_MSG_STATUS        (WM_USER+0x100)
#define MCB_MSG_DATA          (WM_USER+0x200)
#define MCB_DVSTS             (0x0100)
#define MCB_DVREADY           (0x0101)
#define MCB_DVERR             (0x0200)
#define MCB_STARTERR          (0x0201)
#define MCB_STOPERR           (0x0202)
#define MCB_STOPOK            (0x0300)
// macro define of mcb message

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
extern "C" AFX_EXT_CLASS void SetMCBCommand(WPARAM wParam, LPARAM lParam =0);
typedef void (WINAPI *PFCALLBACK)(UINT msg, WPARAM wParam, LPARAM lParam);
#else
// linux 适配版本 ， 头文件 最后最好能整理到统一的文件中 ssx 2023年10月07日14:40:22
#include <stdlib.h>
#include <stdint.h>

#if defined __x86_64__
typedef unsigned long                 WPARAM;
typedef long                   LPARAM;
#else
typedef     uint32_t              WPARAM;
typedef     int32_t                LPARAM;
#endif
/*extern "C" */ void SetMCBCommand(WPARAM wParam, LPARAM lParam =0);
typedef void (*PFCALLBACK)(UINT msg, WPARAM wParam, LPARAM lParam);
#endif


#endif // !defined(AFX_MCBCTRL_H__INCLUDED_)
