  // MCBCtrlC.h: interface for the CMCBCtrlC class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(AFX_MCBCTRLC_H__4C9078C7_F07D_4BFD_850D_39CB39720350__INCLUDED_)
#define AFX_MCBCTRLC_H__4C9078C7_F07D_4BFD_850D_39CB39720350__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MCB_param.h"
#include "SocketBuffer.h"
#include "RequestSocket.h"
/////////////////////////////////////////
#include "including.h"
#include "type_def.h"
#include "MCBDataBuffer.h"
/////////////////////////////////////////
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)

#else
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#endif
#define  MAX_DDCRANGE 7
#define  MIN_DDCRANGE 0

#ifdef VC6_BUILD		 // Build project with VC6
#define std::stringA std::string  // use strcpy when use VC6
    #if defined(UNICODE) || defined(_UNICODE)
    #include "nucutil.h"
    #endif //UNICODE,_UNICODE
#endif  //VC6_BUILD

extern PFCALLBACK gCallBack;
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
extern HINSTANCE ghInstance;
#endif
extern std::string g_strIniFilePath;
extern bool m_bMCBxInInitialing;
typedef struct tagRECVPARAM
{
    INT8U nMCBIndex;
    UINT nDBLineEnNum;  //DB line enable number
    ULONG pkg_num;

    INT8U cmd_code;
    INT8U cmd_code_match_return_en;
    INT8U cmd_exct_result_report_en;
    INT8U cmd_mismatched_pkg_skip_en;
    INT8U pkg_rate_cal_en;
    INT8U ADFormat_Bit;//AD×ª»»Î»Êı
    UINT nRowNum[MAX_MCBNUM];//Ã¿ĞĞ/ÁĞ±ÊÊı
    UINT nColNum[MAX_MCBNUM];//ĞĞÁĞÊı
    UINT nDMNum[MAX_MCBNUM];//Ã¿¸öDBLine´øµÄÓĞĞ§Ä£¿éÊı
    bool  bDebug;
}RECVPARAM;


class /*AFX_EXT_CLASS*/ CMCBCtrlC /*: public CObject*/
{
public:
    CMCBCtrlC();
    virtual ~CMCBCtrlC();

public:
    //ÏûÏ¢»Øµ÷
    static void SendMCBMsg(UINT msg, WPARAM wParam =0, LPARAM lparam =0);
    //ºËĞÄ¹¦ÄÜº¯Êı
    void ReadConfigParaFromIni();
    void InitialAllMCB();
    void InitialMCB(unsigned short nMCB);
    /////////////////////////////////////////////////////
    //HMCB-4 functions
    /////////////////////////////////////////////////////
    //ÊÇ·ñÊ¹ÄÜÊ¹ÓÃHMCB*µÄ4¶Ë¿Ú
    static bool m_bEnMCBx4;
	//¸ßµÍÄÜÖ¸Ê¾ĞÅºÅ½èÓÃÎª¹³îõĞÅºÅ
	static bool m_bGoudang_S4;
	//ÈıÄÜ¼ÓËÙÆ÷Ê¹ÄÜ(ÈıÄÜ¼ÓËÙÆ÷£¨DI(3)Îª¸ß£¬ÔòÎª0.7MeV£¬ÄÜÁ¿Ö¸Ê¾ÖµÎª2»ò3£¬·ñÔòÎª¸ßµÍË«ÄÜ£¬ÄÜÁ¿Ö¸Ê¾ÖµÎª0»ò1£©)
	static bool m_bThreeEnergyAcc_En;

    //
    //ÊÇ·ñÊ¹ÄÜÊ¹ÓÃDAQ Tirgger¿ØÖÆ²É¼¯ÆôÍ£
//    static bool m_bEnCtrlDAQTrig;
//    //4¶Ë¿ÚHMCBµÄDBLine¸öÊı
//    static int m_nMCBx4DBLine;
    /////////////////////////////////////////////////////
    /////////////////////////////////////////////////////
    void InitialMCB_HMCB4(unsigned short nMCB);
    void Start_HMCB4(long lcounter);
    INT8U ResetCBLogicBeforeStart_HMCB4(unsigned short nMCB);
    bool ConfigFpga_HMCB4(unsigned short nMCB =0);
    //Í¨¹ıÓÃ»§²ÎÊıÉèÖÃÔöÒæÄ£Ê½ SETRANGE
    int SetDMCmdReg_HMCB4(unsigned short nMCB);
    //ÉèÖÃÌ½²âÆ÷¹¤×÷²ÎÊı
    int ConfigureUserParameter_HMCB4(INT8U nDbline, unsigned short nMCB/*=0*/ );
    //
    void FetchMsg_HMCB4(CRequestSocket *pRequestSocket, int nIndex, int nPort);

    // ============================================================
    // ½ÓÊÕ/½âÎö½âñî£ºÃ¿¸ö MCB Ò»Ì×½ÓÊÕ»º³å + ¶ÀÁ¢½âÎöÏß³Ì
    // £¨ÒÆÖ²×Ô V2.0.1.1 MCBCtrlC.h µÄ MCBParseContext£¬ÊÂ¼ş¸ÄÎª Linux condvar ÊµÏÖ£©
    // ============================================================
    // ÊÖ¶¯ÖØÖÃÊÂ¼ş£¨Windows CreateEvent(NULL,TRUE,FALSE,NULL) µÄµÈ¼ÛÎï£©£º
    // signal ºó±£³ÖĞÅºÅÌ¬£¬Ö±µ½½âÎöÏß³Ì×Ô¼º clear()£¬±ÜÃâ¶ªÊ§»½ĞÑ
    class ManualResetEvent {
    public:
        void signal() {
            std::lock_guard<std::mutex> lk(m_mu);
            m_signaled = true;
            m_cv.notify_all();
        }
        void clear() {
            std::lock_guard<std::mutex> lk(m_mu);
            m_signaled = false;
        }
        // ·µ»Ø true=ĞÅºÅÌ¬£¬false=³¬Ê±£¨¶ÔÓ¦ WaitForSingleObject 100ms ÂÖÑ¯£©
        bool wait(int ms) {
            std::unique_lock<std::mutex> lk(m_mu);
            return m_cv.wait_for(lk, std::chrono::milliseconds(ms),
                                  [this] { return m_signaled; });
        }
    private:
        std::mutex m_mu;
        std::condition_variable m_cv;
        bool m_signaled = false;
    };

    struct MCBParseContext
    {
        CMCBDataBuffer  shared;         // ½ÓÊÕÏß³ÌĞ´Èë/½âÎöÏß³Ì°á×ßµÄ¹²Ïí»º³å£¨csBuf ±£»¤£©
        CMCBDataBuffer  local;          // ½âÎöÏß³ÌË½ÓĞ»º³å£¨³£×¤£¬Ì¯Ïú¿½±´£©
        ManualResetEvent dataEvent;     // Í¨Öª½âÎöÏß³ÌÓĞĞÂÊı¾İ
        std::thread     parseThread;    // ½âÎöÏß³Ì¾ä±ú
        volatile MCB_LONG bStop;        // Í¨ÖªÏß³ÌÍË³ö
        std::mutex      csBuf;          // ½ÓÊÕÏß³ÌÓë½âÎöÏß³Ì·ÃÎÊ shared µÄ»¥³â
        // Ïß³ÌÍË³öÍ¬²½£¨StopParseThreads ÏŞÊ±µÈ´ı£¬Ìæ´ú Windows WaitForSingleObject£©
        std::mutex              doneMu;
        std::condition_variable doneCv;
        volatile bool           bThreadDone;
        // Í³¼Æ
        unsigned long long totalRecvBytes;
        unsigned long long totalParsedBytes;

        MCBParseContext() : bStop(0), bThreadDone(false), totalRecvBytes(0), totalParsedBytes(0) {}
        MCBParseContext(const MCBParseContext&) = delete;
        MCBParseContext& operator=(const MCBParseContext&) = delete;
    };

    static bool m_bParseCtxInited;
    static MCBParseContext g_parseCtx[MAX_MCBNUM];
    // ½âÎöÏß³ÌÈë¿Ú
    static void MCBParseThreadProc(int nMCB);
    // Æô¶¯ / Í£Ö¹½âÎöÏß³Ì
    static bool StartParseThreads();
    static void StopParseThreads();
    // ´Ó¹²Ïí»º³å½âÎö³öÍêÕûÊı¾İ°ü£¨consumed=±¾´ÎÒÑÏû·Ñ×Ö½ÚÊı£©
    static bool ParseBuffer_HMCB4(int nMCB, CMCBDataBuffer& local, ULONG nTarget_pkg_num, size_t& consumed);

    void ReadParseData_HMCB4(ULONG pkg_num);


    DWORD m_gdwThreadId[MAX_MCBNUM];
    //HANDLE m_ghThread[MAX_MCBNUM];

    //Êı¾İ½âÎöÏß³Ì
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    static DWORD WINAPI ETH_read_and_parse_dataEx_Thread_HMCB4(void * lpParameter);
#else
    static DWORD ETH_read_and_parse_dataEx_Thread_HMCB4(void * lpParameter);
#endif
    //¸üĞÂÒ»¸öÊı¾İ°ü¸øApp
    static void UpdateOnePacketDataToApp_HMCB4(OneDBLineOnePackage *pDBDataPackage);
    bool DAQFailToReset();
    //
    void CheckDVReadySts_HMCB4();
    void CheckDataEnd_HMCB4();
    void CheckStopSts_HMCB4();
    void CheckRunStatus_HMCB4();
    /////////////////////////////////////////////////////
    void CheckHMCBStatus();

    /////////////////////////////////////////////////////
    /////////////////////////////////////////////////////
    /////////////////////////////////////////////////////
    INT8U ResetCBLogicBeforeStart(unsigned short nMCB);
    //
    void Start(long lcounter);
    //????????????????
    void ProcStartDaqCmd(unsigned short nMCB);
    static void Stop(int nCmd, unsigned short nMCB = 0);//0--Cmd 1--Acq  2--Overtime
    //
    /////////////////////////////////////////////////////

    /////////////////////////////////////////////////////
    void CloseMCB();
    //ÉèÖÃ/È¡Ïû TÄ£Ê½
    void SetTMode(bool bT);
    //ÉèÖÃCB ¹¤×÷Ê±ĞòÂß¼­²ÎÊı
    UINT8 InitCBWorkPara(unsigned short nMCB);
    //ÉèÖÃÖ÷¿ØÖÆºĞ´¥·¢Ô´
    void SetTrig(int trig);
    INT8U SetTrigCtrlReg(unsigned short nMCB);

	//ÉèÖÃ´¥·¢Ô´²¢Ê¹ÄÜDAQ
	INT8U SetTrigCtrlRegAndEnableDAQ(INT32U enable, unsigned short nMCB);

    //??????????????????
    static INT8U SetMainHMCBTrigCtrlRegAsNONE();
    //
    void SetMode(unsigned short nMode);
    void SetReg(unsigned short nMCB);
    //ÉèÖÃÔöÒæ
    void SetRange(unsigned short nRange);
    void SetRange(unsigned char * pRange);
    //»ñÈ¡ÔöÒæÉèÖÃ²ÎÊı
    void GetRangeParam(std::string str);
    //»ñÈ¡¹¤×÷Ê±ĞòÂß¼­²ÎÊı
    void GetModeParam(std::string str);
    //ÏµÍ³×´Ì¬ÂÖÑ¯¼à¿Ø
    void OnTimer();
    void CheckConnect();
    void CheckDataEnd();
    void CheckStopSts();
    void CheckRunStatus();
    void CheckDVReadySts();
    //ÍøÂçÍ¨ĞÅÏà¹Ø
    //Í¨¹ısocket»ñÈ¡Êı¾İ²¢±£´æÖÁFIFO»º´æÖĞ
    //
    CRequestSocket*/* CMCBCtrlC::*/OpenSocket(unsigned short nMCB, int nPort);
    bool ConfigFpga(unsigned short nMCB);
    bool ConnectServer(CRequestSocket *pSocket, unsigned short nMCB, int nPort);
    bool ConnectOpen(unsigned short nMCB);
    //
    void FetchMsg(CRequestSocket *pRequestSocket, int nIndex, int nPort);
    //¹Ø±ÕÍøÂç¶Ë¿Ú
    void ConnectClose(unsigned short nMCB, UINT wParam = 0);

    //ÉèÖÃÖ÷¿ØºĞÄÚ´¥·¢ÖÜÆÚ¡¢»ı·ÖÊ±¼ä
    void SetPOTMT(int nTm = 0);
    void SetPOTMW(int nTm = 0);
    void SetINTTm(int nTm = 0);
    //µ÷ÓÃConfigureUserParameterÉèÖÃÌ½²âÆ÷¹¤×÷²ÎÊı
    int SetDMCmdReg(unsigned short nMCB);
    //ÉèÖÃÌ½²âÆ÷¹¤×÷²ÎÊı
    int ConfigureUserParameter(unsigned short nMCB);
    //¶àÏß³Ì½âÎöÊı¾İ
    void ReadParseData(ULONG pkg_num);
    //Êı¾İ½âÎöÏß³Ì
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    static DWORD WINAPI ETH_read_and_parse_dataEx_Thread(void * lpParameter);
#else
    static DWORD  ETH_read_and_parse_dataEx_Thread(void * lpParameter);
#endif
    //½áÊø²ÉÊı
    static void NormalEndDAQ(unsigned short nMCB);
    //¸üĞÂÒ»¸öÊı¾İ°ü¸øApp
    static void UpdateOnePacketDataToApp(OneDBLineOnePackage *pDBDataPackage);


//³£¹æ¸¨Öú¹¦ÄÜº¯Êı
public:
    //¶ÁÈ¡¼Ä´æÆ÷Öµ·¶Î§: 0--²¿·Ö¶ÁÈ¡ 1--È«²¿¶ÁÈ¡
    void DumpAllMCBAllTheECSRegisters(UINT nLevel = 0);
    int DumpAllTheECSRegistersRelease(unsigned short nMCB, UINT nLevel = 0, UINT nFLag = 0);
    void SetDA(DWORD sda, unsigned short nMCB);
    void SetNewIPAddress(unsigned char * pnNewIP, unsigned short nMCB);
    //
    void UpdateNewTime();
    //»ñÈ¡³ÌĞò°æ±¾
    std::string GetProductVersion(std::string lpszFileName = NULL);
    //°ÑÏûÏ¢Âë¼°²ÎÊı×ª»»ÏûÏ¢×Ö·û´®¼°²ÎÊı×Ö·û´®
    static void Conv_SendAppMsg_code_to_String(UINT msg_code, std::string& str);
    static void Conv_SendAppMsg_wParam_to_String(WPARAM msg_wParam, std::string& str);
    static void Conv_SendAppMsg_lParam_to_String(WPARAM msg_wParam, LPARAM msg_lparam, std::string& str);
    //
    void SaveExdata();
    //
    UINT GetMCBNum(void);
    void GetDMInfo(DMINFO *pDMInfo);
    void GetDMExInfo(DMEXINFO *pDMExInfo);
    //ÈÕÖ¾Ïà¹Ø
    static void WriteLogFile(std::string lpsz, UINT nLogLevel =0, bool bFlush = true);
    //
    static void WriteDataErrorLogFile(std::string lpsz, UINT nMCB);
public:
//Õë¶ÔÖÇÄÜĞèÇóÀ©Õ¹¹¦ÄÜ
    void GetMCBxDMyPGz(std::string str);
    //ÅĞ¶ÏÊÇ·ñ´æÔÚ D:\\XRayV3\\Config\\Setting.ini »ò D:\\XRayV4\\Config\\Setting.iniÎÄ¼ş
    //·µ»Ø3:´æÔÚD:\\XRayV3\\Config\\Setting.ini
    //·µ»Ø4:´æÔÚD:\\XRayV4\\Config\\Setting.ini
    //·µ»Ø0:²»´æÔÚXRayVxÎÄ¼ş
    DWORD IsExistXRayVxSetting();
//´ıÍêÉÆ¹¦ÄÜº¯Êı
public:
    void SetSendAllData(int nSAD);
    void SetPTFDiv(UINT nFDiv);

//ºê¶¨Òå²ÎÊı
public:

#if defined(VC6_BUILD) && (defined(UNICODE) || defined(_UNICODE))
    _nuc_util::CNUCStrConverter m_szMCBIPAddr_sttConvert;
#endif //VC6_BUILD, UNICODE,_UNICODE
//
public:
    /////////////////////////////////////////////////////
    //¸¨Öú¹¦ÄÜ
    //¿ØÖÆ°å¹Ì¼ş°æ±¾
    std::string m_strMCBCoreFirmwareVer[MAX_MCBNUM];
    unsigned int m_nMCB_Version[MAX_MCBNUM];
    //2022-06-04
    //ÓÃÓÚ¼ÆËã´ø¿íÍ³¼Æ»ñÈ¡×Ö½ÚÊıºÍÊ±¼ä
    static ULONG m_SocketRecBytes[MAX_MCBNUM];
    static DWORD m_TimeSaveSocketData[MAX_MCBNUM];
    /////////////////////////////////////////////////////
    //Êı¾İ½âÎöÒì³£´ÎÊıÀÛ¼Æ
    //static UINT m_nMissAA55Cnt;
    static std::map<int,UINT> m_nMissAA55Cnt;
    //static ULONG m_nWaitDataTrigger[MAX_MCBNUM];
    //ÊÇ·ñÎªµ÷ÊÔ°æ±¾£¬Êı¾İÒì³£Ê±±£´æÒì³£Êı¾İ
    static UINT m_bDebug;
    //Òì³£Êı¾İ´æ´¢ÎÄ¼şÏà¹Ø
    static std::string m_DataErrorfilename[MAX_MCBNUM];
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    static CStdioFile m_DataErrorfile[MAX_MCBNUM];
#else
    static std::fstream  m_DataErrorfile[MAX_MCBNUM];
#endif
    static bool m_bDataErrorCLogPath[MAX_MCBNUM];
    //10sÔÚÈÕÖ¾ÎÄ¼şÄÚ¼ÇÂ¼Ò»´ÎĞÄÌø
    WORD m_nHeartbeatSec;
    /////////////////////////////////////////////////////
    //×´Ì¬¸üĞÂtime
    DWORD m_timeInitSpan;
    DWORD m_timeuncheck;
    //startÃüÁîÆğÊ¼Ê±¼ä
    DWORD m_timeprestart;
    static DWORD m_timeprestop;
    static DWORD m_timepreend;
    DWORD m_timenow;
    //
    static ULONG m_lcounter;
	//½öÓÃÓÚµ÷ÊÔ½Úµã¼ÆÊı
    static ULONG m_lDebugCnt;
	//
    static DWORD m_timehot[MAX_MCBNUM];
    //Éè±¸Æô¶¯²É¼¯£¬xÃëÄÚÃ»ÓĞÊÕµ½Êı¾İ±¨ no data ÎŞÊı¾İ´íÎó
    static DWORD m_timenodata[MAX_MCBNUM];
    DWORD m_timeunconnect[MAX_MCBNUM];
    //data socket receive data time
    static DWORD m_timeSocketRecData[MAX_MCBNUM];
    /////////////////////////////////////////////////////
#define MAX_CHNADJNUM 4
    static int m_nAdjChn[MAX_CHNADJNUM];  //adjust mode: 0-NONE, 1-ROW, 2-COLUMN, 3-MODULE, 4-ALL_ROW
    static int m_nMCBxAdjChn[MAX_MCBNUM][MAX_CHNADJNUM];  //adjust mode: 0-NONE, 1-ROW, 2-COLUMN, 3-MODULE, 4-ALL_ROW
    static int m_nMCBxDByAdjChn[MAX_MCBNUM][MAX_DBLINENUM][MAX_CHNADJNUM];  //adjust mode: 0-NONE, 1-ROW, 2-COLUMN, 3-MODULE, 4-ALL_ROW

//¹´µ±ĞÅºÅ¼ÇÂ¼
#define GODOWNNUM (65536*8)
    static unsigned short m_godown[GODOWNNUM];
//Exdata¼ÇÂ¼
#define MAXEXDATANUM (16384)
    static UINT m_InfoExdata[5][MAXEXDATANUM];  // 0-Socket No., 1- Pulse No., 2-DI, 3-ex_data_upper, 4-ex_data_lower
    bool m_bSaveExdata;
    std::string m_filenameExd;
//Í£Ö¹²É¼¯ºó´òÓ¡Ã¿¸ö¿ØÖÆºĞÇ°128¸öÍ¨µÀÊı¾İ
#define DDataNUM (128)
    static UINT m_uDetData[MAX_MCBNUM][DDataNUM];
    static ULONG m_uDBParameter[MAX_MCBNUM][DDataNUM];
    static ULONG m_uDBPulse[MAX_MCBNUM][DDataNUM];
//Í£Ö¹²É¼¯ºó´òÓ¡Ã¿¸ö¿ØÖÆºĞ¼°¼¶ÁªµÄµÚÒ»¸öDB°å×îºóÒ»´Î·µ»ØµÄÊı¾İ°üÄÚ°üº¬µÄ×´Ì¬
    static OneDBLineOnePackageStatue m_pOneDBLineOnePackageStatue[MAX_MCBNUM];//
///////////////////////////////////////////////////////
public:
    //Ä£¿é¡¢Ì½²âÆ÷¹¤×÷Ê±Ğò¿ØÖÆ¡¢²ÎÊıÏà¹Ø
    //ÏµÍ³¼°¸÷MCB³õÊ¼»¯×´Ì¬
    //bool m_bMCBxInInitialing =false;

    // ½âñî¸ÄÔì£¨¶ÔÕÕ V2.0.1.1£©£ºÒÔÏÂ±êÖ¾¿ç½ÓÊÕ/½âÎö/Ö÷¿ØÏß³Ì·ÃÎÊ£¬
    // ÀàĞÍ´Ó bool/int ¸ÄÎª volatile MCB_LONG£¨ÆÕÍ¨¸³Öµ¼æÈİ£¬¶ÁÈ¡×ß AtomicLoadL£©
    static volatile MCB_LONG m_MCBRunSts[MAX_MCBNUM]; //bit 0-pre start, 1-pre stop cmd, 2-pre stop normal
    static volatile MCB_LONG m_MCBReadySts[MAX_MCBNUM]; //bit 0-connection, 1-MB fpga, 2- AD fpga, 3-Reg set
    bool m_bSetDMInfo[MAX_MCBNUM];  // if set DM info
    //check if slave MCB ready to start
    //´Ó¿ØÖÆºĞÊÇ·ñ×¼±¸¾ÍĞ÷
    bool m_bSlaveStart =false;
    volatile MCB_LONG m_bPreStart =0;
    //ÊÇ·ñÍ£Ö¹È¡Êı
    static volatile MCB_LONG m_bPreDataEnd ;
    static volatile MCB_LONG m_bPreStop;
    static volatile MCB_LONG m_bStart;
	//ÒÑÊÕµ½Í£Ö¹²É¼¯ÃüÁî
	static volatile MCB_LONG m_bRecStopCmd;
    //ÊÇ·ñ×Ô¶¯ÖØÁ¬
    bool m_bconnectauto;

	//Ó²¼şĞòÁĞºÅÈ¡ÖµÄ£Ê½
	//Ä¬ÈÏÎª1£¬È¡Êµ¼Ê·µ»Ø°ü¹üÊıĞòÁĞºÅ×öÓ²¼şĞòÁĞºÅ
	//¸ÄÎª0£¬È¡Èí¼ş°üĞòÁĞºÅ
	//£¨ÁÙÊ±½â¾ö´æÔÚÓ²¼şĞòÁĞºÅÓĞÖØ¸´µÄÏÖÏó£¨»»¿ØÖÆºĞÈÔ¾É´æÔÚ£©£¬µ¼ÖÂ¶à¸öºĞ¼¶ÁªÊ±´æÔÚÊı¾İ¶Ô²»ÆëµÄÎÊÌâ£©£¨Èí¼ş¶Ë°´»Øµ÷Êı¾İÀïÃæµÄÓ²¼ş´¥·¢Êı´æÍ¼£©
	static bool m_bHWSerialNumberMode;

    //Êı¾İ½âÎö°´ÖÇÄÜ£¨=0£©»¹ÊÇ¸ßÄÜ £¨=1£© Ä£Ê½½âÎö´¦Àí
    static bool m_bHEorCXMode;
    //ÊÇ·ñÒªÇó»ñÈ¡¿ªÊ¼µÄ 0xA5 Êı¾İ°ü,Ä¬ÈÏĞèÒª»ñÈ¡
    static bool m_bNeedGet0xA5;


    //ÊÇ·ñÒÑ»ñÈ¡µ½¿ªÊ¼µÄ0xA5Êı¾İ°ü
    static bool m_bGet0xA5Packet[MAX_MCBNUM];
    //»ñÈ¡µ½0xA5Êı¾İ°üÇ°ÒÑ½âÎö³öµÄÊı¾İ°ü
    static UINT m_nMiss0XA5PacketCnt[MAX_MCBNUM];
    //ÉÏÒ»´ÎÊı¾İ²É¼¯ÊÇ·ñ³É¹¦Íê³É±êÖ¾
    static volatile MCB_LONG m_bPreDAQSucessFlag[MAX_MCBNUM];
    //Ì½²âÆ÷Ä£¿éÅäÖÃÏêÏ¸²ÎÊı:MCB¸öÊı£¬Ì½²âÆ÷ÊıÄ¿£¬ĞĞÁĞÖµ£¬Í¨µÀ×ÜÊı
    static DMINFOHMCB4 m_DMInfo[MAX_MCBNUM];
    //¿ØÖÆºĞÏà¹Ø²ÎÊı
    //ÊÇ·ñÊ¹ÄÜÊ¹ÓÃHMCB*µÄ4¶Ë¿Ú
    //static bool m_bEnMCBx4;
    //¿ØÖÆºĞ¸öÊı
    static int m_nMCBNum;
    //4¶Ë¿ÚHMCBµÄDBLine¸öÊı
    //static int m_nMCBx4DBLine;
    //Ì½²âÆ÷AD×ª»»Î»Êı
    static INT m_nADBit;
    //Ì½²âÆ÷Êı¾İÉÏ´«Èí¼şÎ»Êı
    bool m_bFcto16ADBit;
    int m_nTrig; // trig source 0 -inner ,1-outer
    //CB¹¤×÷²ÎÊıµÈ
    Register m_reg;
    //Ì½²âÆ÷°å¹¤×÷¼Ä´æÆ÷ÅäÖÃ²ÎÊı
    UINT m_nADMCMDREG[MAX_MCBNUM][MAX_MCBADNUM];   //DM cmd register, bit0~2--DDC232 range
    //Ì½²âÆ÷°å¹¤×÷¼Ä´æÆ÷ÅäÖÃ²ÎÊı(Ö§³Ö¶à¿é¶à¶Ë¿Ú¼¶Áª)
    UINT m_nMCBxDByREG[MAX_MCBNUM][MAX_DBLINENUM][MAX_MCBADNUM];   //DM cmd register, bit0~2--DDC232 range


    volatile MCB_LONG m_bDVReady;  // if device ready
    //
    std::string m_szMCBIPAddr;
    //??????????????????IP????
    std::string m_szMCBSetIPAddr[MAX_MCBNUM];
    static CRequestSocket* m_pDataSocket[MAX_MCBNUM];
    //
    //Ì½²âÆ÷FPGAÅäÖÃbinÎÄ¼ş
    std::string m_szADM_FPGAFilename;
    //Ã¿¸ö¿ØÖÆ°åµÄÌ½²âÆ÷FPGAÅäÖÃbinÎÄ¼ş
    std::string m_szMCB_FPGAFilename[MAX_MCBNUM];
    //Ã¿¸ö¿ØÖÆ°åµÄÃ¿¸öDBlineµÄÌ½²âÆ÷FPGAÅäÖÃbinÎÄ¼ş
    std::string m_szMCBDBLine_FPGAFilename[MAX_MCBNUM][MAX_DBLINENUM];
    //MCB dbline bin file ÅäÖÃ±êÖ¾Î»
    INT32U m_nDBLine_FPGAFileConfigFlag[MAX_MCBNUM][MAX_DBLINENUM];
    //iniÅäÖÃ²ÎÊıÎÄ¼şÂ·¾¶
    std::string m_strMCBCtrlIniFile;
    //Ì½²âÆ÷ÔöÒæÉèÖÃsave DM range set by APP, max 4 range
    unsigned char m_cDMRange[MAX_MCBNUM][(MAX_MCBADNUM+1)*4];
    /////////////////////////////////////////////////////
    static bool m_bZeroPadding;//????????????????????????????????????
    static long m_lPrePulseCnt[MAX_MCBNUM];//????????????????
    /////////////////////////////////////////////////////
public:
    //ÊÕµ½[Start]ÃüÁî´ÎÊı
    INT32U m_nRecvStartCmdCnt;
    //Ì½²âÆ÷Êı¾İ½âÎöÏà¹Ø
    FILE* cmd_pkg_parse_log_file;
    //
    static long pnDBlineDetectorData[MAX_MCBNUM][MAX_MCBADNUM][MAX_DMCOLNUM][MAX_DMROWNUM];
    //
    static INT32U parsed_pkg_n[MAX_MCBNUM];//ÒÑ¾­½âÎöµÄÊı¾İ°ü¸öÊı
    static INT32U Update_pkg_n[MAX_MCBNUM];//ÒÑ¾­½âÎöÇÒ·¢ËÍApp¸üĞÂµÄÊı¾İ°ü¸öÊı
    static INT32U SC_1st_pkg_index[MAX_MCBNUM];//Ã¿Ò»¸öDB Line·µ»ØµÄµÚÒ»¸öÊı¾İ°ü
    //
    static INT32U SC_1st_pkg_TriggCnt[MAX_MCBNUM];//Ã¿Ò»¸öDB Line·µ»ØµÄµÚÒ»¸öÊı¾İ°üÄÚ´¥·¢¼ÆÊı
    //static INT32U SC_Current_pkg_TriggCnt[MAX_MCBNUM];//Ã¿Ò»¸öDB Line·µ»ØµÄµ±Ç°Êı¾İ°üÄÚ´¥·¢¼ÆÊı
    //
    static INT32U SC_pre_pkg_index[MAX_MCBNUM];//Ã¿Ò»¸öDB Line·µ»ØµÄÇ°Ò»¸öÊı¾İ°üË÷Òı
    static INT32U SC_current_pkg_index[MAX_MCBNUM];//Ã¿Ò»¸öDB Line·µ»ØµÄµ±Ç°Êı¾İ°üË÷Òı
    //
    //Ì½²âÆ÷Êı¾İ½âÎöÏà¹Ø
    static MCBData m_MCBData[MAX_MCBNUM + 1];
    static MCBData m_MCBDataBackup[MAX_MCBNUM];//Ç°´ÎÊı¾İ±¸·İ
    static MCBData m_MCBDataTmp[MAX_MCBNUM]; // è§£æçº¿ç¨‹ç§æœ‰ï¼ˆæ¯MCBä¸€ä¸ªï¼Œå¯¹ç…§V2.0.1.1ï¼šå¤šæ¿è§£æçº¿ç¨‹å¹¶å‘ï¼Œå•å®ä¾‹ä¼šæ’•è£‚ï¼‰ // temporary data for channel adjust
    static MCBData_H m_MCBDataHeadTmp[MAX_MCBNUM]; // temporary data for AIN/DI...adjust
    //MCBData_H m_MCBDataHTmp[MAX_MCBNUM]; // temporary data for AIN/DI...adjust
    //unsigned char m_ADOriData[MAX_MCBNUM][MAX_MCBADNUM*MAX_DMROWNUM*MAX_DMCOLNUM*sizeof(long)];
    //Ul_Uc m_ADOriDataUL[MAX_MCBNUM][MAX_MCBADNUM*MAX_DMROWNUM*MAX_DMCOLNUM];
    /////////////////////////////////////////////////
    //Èí¼ş¼ÇÂ¼µÄ»ñÈ¡µÄÊı¾İ°üÊı
    static unsigned long m_scounter[MAX_MCBNUM];
    //Ö¸¶¨MCBxÉÙ¶ªÆú°ü¹üÊı£¬Ê¹Êı¾İÓëÆäËûMCB¿ØÖÆºĞÊı¾İ¶ÔÆë£¬Ä¬ÈÏÖµÎª0¡££¨Ö÷ÒªÎªÁËÊ¹ÁÁ¶ÈĞ£ÕıÊı¾İÓë³ÉÏñÊı¾İÄÜµ÷Õû¶ÔÆë£©
    static INT32U m_nDiscardNum[MAX_MCBNUM];
    //Ö¸¶¨MCBxÊÇ·ñ½øÈëÕï¶ÏÄ£Ê½
    bool m_bMCBxDTM[MAX_MCBNUM+1];

	/////////////////////////////////////////////////////
	//2026-03-19Ôö¼Ó¶ÔCS0200MµÄÖ§³Ö
	//Ôö¼Ó [CHADJ] DEVICETYPEID ÅäÖÃÏî
	//Ä¬ÈÏÖµÎª0£¬³£¹æÉè±¸
	//1¡ª¡ªCS0200M£¬ µ÷ÕûÖ÷¿ØÖÆºĞµÄm_MCBData[nIndex].d_head.head_use.accEnergyÎª³öÊø¹â»úµÄ±àºÅ( m_MCBData[nIndex].d_head.head_use.accEnergy = (long)l_angle);
	//³öÊø¹â»úµÄ±àºÅ¹æÔò£º¹â»ú´¥·¢ĞÅºÅ½ÓDI-0£¬¹â»úÑ­»·ĞÅºÅ½ÓDI-1£¬DI-1Îª1Ê±µÄ¹â»ú±àºÅÎª0£¬DI-1Îª0ºóÃ¿Ò»´Î´¥·¢µÄ¹â»ú±àºÅ+1
	static UINT m_nDeviceTypeID;
	//ÉäÏßÔ´ĞòºÅ
	static UINT m_nXRayGenN;
	/////////////////////////////////////////////////////


public:
    //ÈÕÖ¾¼ÇÂ¼Ïà¹Ø
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    static char m_szPath[MAX_PATH];
#else
    static char m_szPath[PATH_MAX];
#endif
    bool m_bPath;
    static std::string m_filename;
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    static CStdioFile m_file;
#else
    static std::fstream m_file;
#endif
    static bool m_bCLogPath;
    //ÈÕÖ¾Ïà¹Ø
    static std::string m_strLogFilePath;
    static bool m_bSetLogFilePath;
    static UINT m_nLogLevel;
    /////////////////////////////////
    //¼ÇÂ¼data socketÔ­Ê¼Êı¾İ
    static std::string m_SocketRawDatafilename;
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    static CStdioFile m_SocketRawDatafile;
#else
    static std::fstream m_SocketRawDatafile;
#endif
    static bool m_bSocketRawDatafile;
    static ULONGLONG  m_unSocketRawWrCnt;//Ğ´Èë¼ÇÂ¼µÄ×ÜÊı¾İ×Ö½ÚÊı
    /////////////////////////////////
public:
//ÖÇÄÜÊı¾İµ÷ÕûĞèÇó
#define MAX_SECTIONADJNUM 5
    static int m_nSectionFirstADNo[MAX_SECTIONADJNUM];
    static int m_nSectionLastADNo[MAX_SECTIONADJNUM];
    //
    static int m_nMCBxSectionFirstADNo[MAX_MCBNUM][MAX_SECTIONADJNUM];
    static int m_nMCBxSectionLastADNo[MAX_MCBNUM][MAX_SECTIONADJNUM];

    //ÖÇÄÜÀ©Õ¹ĞèÇó²ÎÊı
    static int m_nAppADBit;
    //
    static UINT m_nALGValue;
    //
    static double m_fProgrammeGainAmp;
    //
    static double m_fMCBxDMyPGz[MAX_MCBNUM][MAX_MCBADNUM][MAX_DMROWNUM*MAX_DMCOLNUM/16];   //Õë¶ÔÎïÆ·»úÓ¦ÓÃ£¬16¸öÍ¨µÀÒ»×éµ¥¶À¿ÉÉèÖÃ¸¡µãÊıÔöÒæ
//    //Ö»Õë¶ÔÖÇÄÜÓÃµ¥¿é¶à¿Ú¿ØÖÆÄ£¿éÊ±£¬¶à¶Ë¿ÚÊı¾İºÏ²¢ÓĞĞ§£¬¸ßÄÜÎŞĞ§
//    static bool m_bAllMCBDataMerging;//ËùÓĞÊı¾İºÏ²¢£¬´¥·¢Ò»´Î»Øµ÷
//    UINT m_nMergingDMNum;     //ËùÓĞÊı¾İºÏ²¢ºóµÄÄ£¿éÊı
    //
public:
    //¸¨Öú¹¦ÄÜ
    //¶àÁĞºÏÒ»£¬¶àÁĞºÏ²¢ÇóËãÊõ¾ùÖµ
    static bool m_bColCombine;
    //µ÷ÕûÌ½²âÆ÷Ä£¿éÊı¾İÓë¿ØÖÆºĞAin¡¢Di Êı¾İ´íÎ»1 ¸ö´¥·¢µÄÎÊÌâ
    static bool m_bAdjAinDi;
    /////////////////////////////////////
    //·¢ËÍ£¨½ÓÊÕ£©ËùÓĞÊı¾İ
    static int m_nSendAllRecorded;//½øÈësend all Ä£Ê½ÈÕÖ¾ÊÇ·ñÒÑ¼ÇÂ¼ 0 ³õÊ¼ 1ĞèÒª¼ÇÂ¼ 2ÒÑ¼ÇÂ¼
    static LONG m_nNeedSendData[MAX_MCBNUM];
    /////////////////////////////////////
    //´ıÍêÉÆ¹¦ÄÜ
    //×Ô¶¯»ñÈ¡Ì½²âÆ÷Ä£¿éÊıÁ¿ 0-²»×Ô¶¯£¬1-×Ô¶¯
    bool m_bAutoDMNum;
    //·¢ËÍËùÓĞÊı¾İ
    bool m_bSendAllData;
    //µÈ´ı¿ØÖÆºĞ´«ÍêËùÓĞÊı¾İ
    bool m_bEnAllData[MAX_MCBNUM];

    static std::mutex m_Data_mutex[MAX_MCBNUM];
private:
    std::mutex m_Initial_mutex;

    /////////////////////////////////////////////////////
///////////////////////////////////////////////////////
};
#endif // !defined(AFX_MCBCTRLC_H__4C9078C7_F07D_4BFD_850D_39CB39720350__INCLUDED_)
