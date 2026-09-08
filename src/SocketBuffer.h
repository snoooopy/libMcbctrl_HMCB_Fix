//////////////////////////////////////////////////////////////////////////////
// Filename:          SocketBuffer.h
// Version:           1.00.a
// Description:       Socket Buffer Define Header File
// Date:              2008.08.11
//////////////////////////////////////////////////////////////////////////////

#ifndef SOCKETBUFFER_H
#define SOCKETBUFFER_H

//*******************  MCB Message Define  ************************/
#define  MCB_READY_START      ( -3)
#define  MCB_STOP_NORMAL      ( -1)
#define  MCB_STOP_CMD         ( -2)
#define  MCB_NULLDATA         (-10)
#define  MCB_ADFPGA_OK        (-22)          //AD fpga configure ok
#define  MCB_REGSETOK         (-30)
#define  MCB_REGSETERR        (-31)
#define  MCB_NO_DETECTOR      (-40) 
#define  MCB_NO_SYN           (-41)
#define  MCB_PULSE_LEAK       (-42)
#define  MCB_DETECTOR_ERR     (-43)
#define  MCB_CAN_SELFTEST_ERR (-45) 
#define  MCB_CAN_SELFTEST_OK  (-46) 
#define  MCB_NO_DATA          (-47) 
#define  MCB_SEND_ERR         (-48) 
#define  MCB_RANGE_CHANGE     (-50) 
//*******************  MCB Message Define  ***************************/

//*********** data structure define ******************/
#pragma pack(push) //保存对齐状态 
#pragma pack(1)//设定为1字节对齐 

typedef struct tagRegister
{
	unsigned char regCtrlA;
	unsigned char regCtrlB;
	unsigned char regStatus;
	unsigned char regError;
	unsigned char regRam;
	unsigned char regADMN;
	unsigned char regINTOFFTIME;
	unsigned char regRSV[11];
    unsigned short int regPOTMW;
	unsigned short int regSDAH;
	unsigned short int regSDAL;
	unsigned long regPOTMT;
	unsigned long regINTT;
	unsigned long regDataRdyT;//积分结束时刻与所有DB数据准备好时刻之间的间隔，经验值为480uS
	unsigned long regDDILatchT;//DDI锁存时间
} Register;

typedef struct tagHead  // control socket packet header
{
	char control[8];      //control command
	Register reg;		  // main board register, 32 byte
} Head;                   

typedef struct tagCounter
{
	long scounter;  // counter of socket packet;
	unsigned short int  dcount;    //number of trans configure data times
	unsigned short int  bnumber;    //number of configure data bytes
} Counter;

typedef union tagTriByte  // define 3 bytes UINT struct
{
	unsigned char uc[3];
	struct OrderByte
	{
		unsigned char ch;    //high byte
		unsigned char cm;    //middle byte
		unsigned char cl;    //low byte
	}ob;
} TriByte;                   

//typedef struct tagHead_D       // data socket packet header(4bytes)
//{
//	short int scnt_sts;        // data socket packet number (>=0) or MCB status(<0)
//	short int subscnt_ver;     // data sub socket packet number when scnt_sts >=0 or Version number when scnt_sts <0
//}Head_D;
//
//typedef struct tagAux_Data  // aux-data for pcounter, ad-data, DI, ex-data, regError, etc... (20bytes)
//{
//	TriByte pcounter;
//	unsigned char DI;
//	TriByte AIN;
//	unsigned char regError;
//	unsigned long int ex_data_upper;
//	unsigned long int ex_data_lower;
//	union RSV_FBD
//	{
//		unsigned char resv[4];  //reserve 4 bytes for AUX-DATA
//		unsigned short int fbd[2];  // feedback data with status
//	} rsv_fbd;
//}Aux_Data;
//
//typedef union tabTCP_Buffer        //for socket buffer
//{
//	struct TCP_BufferCtrl        //socket buffer for control
//	{
//		Head head; //Head data;
//		Counter cnt; //Counter data;
//		union Data_c         // data
//		{
//			struct MCB_INFO 
//			{
//				unsigned char nMCB;  //MCB number, =0 master, 1 slave
//				unsigned char nADM;  //ADM number of the MCB
//				unsigned char nADMRow;  //Row number of the ADM
//				unsigned char nADMCol;  //Column number of the ADM
//				unsigned short int nADMChn; //detector channel of the ADM should be nADMRow*nADMCol
//				unsigned char nADBit;  //AD Resolution
//				unsigned char nADM_B;  //ADM number in B port of the MCB
//			}mcbInfo;
//			unsigned char datac[RECV_BUFSIZE -sizeof(Head) -sizeof(Counter)];
//			unsigned short int data[(RECV_BUFSIZE -sizeof(Head) -sizeof(Counter))/2];
//			long int datal[(RECV_BUFSIZE -sizeof(Head) -sizeof(Counter))/4];
//		} d;
//	}buffer_ctrl;
//	union TCP_BufferData        //socket buffer for data
//	{
//		struct TCP_BufferD 
//		{
//			Head_D head_d;  // data socket packet header(4bytes)
//			Aux_Data aux_d; // aux-data for pcounter, ad-data, DI, ex-data, regError, etc... (20bytes)
//			TriByte tbADM_d[(SEND_BUFSIZE -sizeof(Head_D) -sizeof(Aux_Data))/3];
//		} d_st;
//		union Data_d         // data
//		{
//			char datac[SEND_BUFSIZE];
//			unsigned char datauc[SEND_BUFSIZE];
//			unsigned short int data[SEND_BUFSIZE/2];
//			long int datal[SEND_BUFSIZE/4];
//			unsigned long int dataul[SEND_BUFSIZE/4];
//		} d;
//	}buffer_data;
//} TCP_Buffer;

#pragma pack(pop)//恢复对齐状态
//*********** data structure define ******************/

#endif //SOCKETBUFFER_H
