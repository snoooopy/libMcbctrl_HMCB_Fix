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
    INT8U ADFormat_Bit;//AD转换位数
    UINT nRowNum[MAX_MCBNUM];//每行/列笔数
    UINT nColNum[MAX_MCBNUM];//行列数
    UINT nDMNum[MAX_MCBNUM];//每个DBLine带的有效模块数
    bool  bDebug;
}RECVPARAM;


class /*AFX_EXT_CLASS*/ CMCBCtrlC /*: public CObject*/
{
public:
    CMCBCtrlC();
    virtual ~CMCBCtrlC();

public:
    //消息回调
    static void SendMCBMsg(UINT msg, WPARAM wParam =0, LPARAM lparam =0);
    //核心功能函数
    void ReadConfigParaFromIni();
    void InitialAllMCB();
    void InitialMCB(unsigned short nMCB);
    /////////////////////////////////////////////////////
    //HMCB-4 functions
    /////////////////////////////////////////////////////
    //是否使能使用HMCB*的4端口
    static bool m_bEnMCBx4;
	//高低能指示信号借用为钩铛信号
	static bool m_bGoudang_S4;
	//三能加速器使能(三能加速器（DI(3)为高，则为0.7MeV，能量指示值为2或3，否则为高低双能，能量指示值为0或1）)
	static bool m_bThreeEnergyAcc_En;

    //
    //是否使能使用DAQ Tirgger控制采集启停
//    static bool m_bEnCtrlDAQTrig;
//    //4端口HMCB的DBLine个数
//    static int m_nMCBx4DBLine;
    /////////////////////////////////////////////////////
    /////////////////////////////////////////////////////
    void InitialMCB_HMCB4(unsigned short nMCB);
    void Start_HMCB4(long lcounter);
    INT8U ResetCBLogicBeforeStart_HMCB4(unsigned short nMCB);
    bool ConfigFpga_HMCB4(unsigned short nMCB =0);
    //通过用户参数设置增益模式 SETRANGE
    int SetDMCmdReg_HMCB4(unsigned short nMCB);
    //设置探测器工作参数
    int ConfigureUserParameter_HMCB4(INT8U nDbline, unsigned short nMCB/*=0*/ );
    //
    void FetchMsg_HMCB4(CRequestSocket *pRequestSocket, int nIndex, int nPort);

    // ============================================================
    // 接收/解析解耦：每个 MCB 一套接收缓冲 + 独立解析线程
    // （移植自 V2.0.1.1 MCBCtrlC.h 的 MCBParseContext，事件改为 Linux condvar 实现）
    // ============================================================
    // 手动重置事件（Windows CreateEvent(NULL,TRUE,FALSE,NULL) 的等价物）：
    // signal 后保持信号态，直到解析线程自己 clear()，避免丢失唤醒
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
        // 返回 true=信号态，false=超时（对应 WaitForSingleObject 100ms 轮询）
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
        CMCBDataBuffer  shared;         // 接收线程写入/解析线程搬走的共享缓冲（csBuf 保护）
        CMCBDataBuffer  local;          // 解析线程私有缓冲（常驻，摊销拷贝）
        ManualResetEvent dataEvent;     // 通知解析线程有新数据
        std::thread     parseThread;    // 解析线程句柄
        volatile MCB_LONG bStop;        // 通知线程退出
        std::mutex      csBuf;          // 接收线程与解析线程访问 shared 的互斥
        // 线程退出同步（StopParseThreads 限时等待，替代 Windows WaitForSingleObject）
        std::mutex              doneMu;
        std::condition_variable doneCv;
        volatile bool           bThreadDone;
        // 统计
        unsigned long long totalRecvBytes;
        unsigned long long totalParsedBytes;

        MCBParseContext() : bStop(0), bThreadDone(false), totalRecvBytes(0), totalParsedBytes(0) {}
        MCBParseContext(const MCBParseContext&) = delete;
        MCBParseContext& operator=(const MCBParseContext&) = delete;
    };

    static bool m_bParseCtxInited;
    static MCBParseContext g_parseCtx[MAX_MCBNUM];
    // 解析线程入口
    static void MCBParseThreadProc(int nMCB);
    // 启动 / 停止解析线程
    static bool StartParseThreads();
    static void StopParseThreads();
    // 从共享缓冲解析出完整数据包（consumed=本次已消费字节数）
    static bool ParseBuffer_HMCB4(int nMCB, CMCBDataBuffer& local, ULONG nTarget_pkg_num, size_t& consumed);

    void ReadParseData_HMCB4(ULONG pkg_num);


    DWORD m_gdwThreadId[MAX_MCBNUM];
    //HANDLE m_ghThread[MAX_MCBNUM];

    //数据解析线程
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    static DWORD WINAPI ETH_read_and_parse_dataEx_Thread_HMCB4(void * lpParameter);
#else
    static DWORD ETH_read_and_parse_dataEx_Thread_HMCB4(void * lpParameter);
#endif
    //更新一个数据包给App
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
    //设置/取消 T模式
    void SetTMode(bool bT);
    //设置CB 工作时序逻辑参数
    UINT8 InitCBWorkPara(unsigned short nMCB);
    //设置主控制盒触发源
    void SetTrig(int trig);
    INT8U SetTrigCtrlReg(unsigned short nMCB);

	//设置触发源并使能DAQ
	INT8U SetTrigCtrlRegAndEnableDAQ(INT32U enable, unsigned short nMCB);

    //??????????????????
    static INT8U SetMainHMCBTrigCtrlRegAsNONE();
    //
    void SetMode(unsigned short nMode);
    void SetReg(unsigned short nMCB);
    //设置增益
    void SetRange(unsigned short nRange);
    void SetRange(unsigned char * pRange);
    //获取增益设置参数
    void GetRangeParam(std::string str);
    //获取工作时序逻辑参数
    void GetModeParam(std::string str);
    //系统状态轮询监控
    void OnTimer();
    void CheckConnect();
    void CheckDataEnd();
    void CheckStopSts();
    void CheckRunStatus();
    void CheckDVReadySts();
    //网络通信相关
    //通过socket获取数据并保存至FIFO缓存中
    //
    CRequestSocket*/* CMCBCtrlC::*/OpenSocket(unsigned short nMCB, int nPort);
    bool ConfigFpga(unsigned short nMCB);
    bool ConnectServer(CRequestSocket *pSocket, unsigned short nMCB, int nPort);
    bool ConnectOpen(unsigned short nMCB);
    //
    void FetchMsg(CRequestSocket *pRequestSocket, int nIndex, int nPort);
    //关闭网络端口
    void ConnectClose(unsigned short nMCB, UINT wParam = 0);

    //设置主控盒内触发周期、积分时间
    void SetPOTMT(int nTm = 0);
    void SetPOTMW(int nTm = 0);
    void SetINTTm(int nTm = 0);
    //调用ConfigureUserParameter设置探测器工作参数
    int SetDMCmdReg(unsigned short nMCB);
    //设置探测器工作参数
    int ConfigureUserParameter(unsigned short nMCB);
    //多线程解析数据
    void ReadParseData(ULONG pkg_num);
    //数据解析线程
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    static DWORD WINAPI ETH_read_and_parse_dataEx_Thread(void * lpParameter);
#else
    static DWORD  ETH_read_and_parse_dataEx_Thread(void * lpParameter);
#endif
    //结束采数
    static void NormalEndDAQ(unsigned short nMCB);
    //更新一个数据包给App
    static void UpdateOnePacketDataToApp(OneDBLineOnePackage *pDBDataPackage);


//常规辅助功能函数
public:
    //读取寄存器值范围: 0--部分读取 1--全部读取
    void DumpAllMCBAllTheECSRegisters(UINT nLevel = 0);
    int DumpAllTheECSRegistersRelease(unsigned short nMCB, UINT nLevel = 0, UINT nFLag = 0);
    void SetDA(DWORD sda, unsigned short nMCB);
    void SetNewIPAddress(unsigned char * pnNewIP, unsigned short nMCB);
    //
    void UpdateNewTime();
    //获取程序版本
    std::string GetProductVersion(std::string lpszFileName = NULL);
    //把消息码及参数转换消息字符串及参数字符串
    static void Conv_SendAppMsg_code_to_String(UINT msg_code, std::string& str);
    static void Conv_SendAppMsg_wParam_to_String(WPARAM msg_wParam, std::string& str);
    static void Conv_SendAppMsg_lParam_to_String(WPARAM msg_wParam, LPARAM msg_lparam, std::string& str);
    //
    void SaveExdata();
    //
    UINT GetMCBNum(void);
    void GetDMInfo(DMINFO *pDMInfo);
    void GetDMExInfo(DMEXINFO *pDMExInfo);
    //日志相关
    static void WriteLogFile(std::string lpsz, UINT nLogLevel =0, bool bFlush = true);
    //
    static void WriteDataErrorLogFile(std::string lpsz, UINT nMCB);
public:
//针对智能需求扩展功能
    void GetMCBxDMyPGz(std::string str);
    //判断是否存在 D:\\XRayV3\\Config\\Setting.ini 或 D:\\XRayV4\\Config\\Setting.ini文件
    //返回3:存在D:\\XRayV3\\Config\\Setting.ini
    //返回4:存在D:\\XRayV4\\Config\\Setting.ini
    //返回0:不存在XRayVx文件
    DWORD IsExistXRayVxSetting();
//待完善功能函数
public:
    void SetSendAllData(int nSAD);
    void SetPTFDiv(UINT nFDiv);

//宏定义参数
public:

#if defined(VC6_BUILD) && (defined(UNICODE) || defined(_UNICODE))
    _nuc_util::CNUCStrConverter m_szMCBIPAddr_sttConvert;
#endif //VC6_BUILD, UNICODE,_UNICODE
//
public:
    /////////////////////////////////////////////////////
    //辅助功能
    //控制板固件版本
    std::string m_strMCBCoreFirmwareVer[MAX_MCBNUM];
    unsigned int m_nMCB_Version[MAX_MCBNUM];
    //2022-06-04
    //用于计算带宽统计获取字节数和时间
    static ULONG m_SocketRecBytes[MAX_MCBNUM];
    static DWORD m_TimeSaveSocketData[MAX_MCBNUM];
    /////////////////////////////////////////////////////
    //数据解析异常次数累计
    //static UINT m_nMissAA55Cnt;
    static std::map<int,UINT> m_nMissAA55Cnt;
    //static ULONG m_nWaitDataTrigger[MAX_MCBNUM];
    //是否为调试版本，数据异常时保存异常数据
    static UINT m_bDebug;
    //异常数据存储文件相关
    static std::string m_DataErrorfilename[MAX_MCBNUM];
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    static CStdioFile m_DataErrorfile[MAX_MCBNUM];
#else
    static std::fstream  m_DataErrorfile[MAX_MCBNUM];
#endif
    static bool m_bDataErrorCLogPath[MAX_MCBNUM];
    //10s在日志文件内记录一次心跳
    WORD m_nHeartbeatSec;
    /////////////////////////////////////////////////////
    //状态更新time
    DWORD m_timeInitSpan;
    DWORD m_timeuncheck;
    //start命令起始时间
    DWORD m_timeprestart;
    static DWORD m_timeprestop;
    static DWORD m_timepreend;
    DWORD m_timenow;
    //
    static ULONG m_lcounter;
	//仅用于调试节点计数
    static ULONG m_lDebugCnt;
	//
    static DWORD m_timehot[MAX_MCBNUM];
    //设备启动采集，x秒内没有收到数据报 no data 无数据错误
    static DWORD m_timenodata[MAX_MCBNUM];
    DWORD m_timeunconnect[MAX_MCBNUM];
    //data socket receive data time
    static DWORD m_timeSocketRecData[MAX_MCBNUM];
    /////////////////////////////////////////////////////
#define MAX_CHNADJNUM 4
    static int m_nAdjChn[MAX_CHNADJNUM];  //adjust mode: 0-NONE, 1-ROW, 2-COLUMN, 3-MODULE, 4-ALL_ROW
    static int m_nMCBxAdjChn[MAX_MCBNUM][MAX_CHNADJNUM];  //adjust mode: 0-NONE, 1-ROW, 2-COLUMN, 3-MODULE, 4-ALL_ROW
    static int m_nMCBxDByAdjChn[MAX_MCBNUM][MAX_DBLINENUM][MAX_CHNADJNUM];  //adjust mode: 0-NONE, 1-ROW, 2-COLUMN, 3-MODULE, 4-ALL_ROW

//勾当信号记录
#define GODOWNNUM (65536*8)
    static unsigned short m_godown[GODOWNNUM];
//Exdata记录
#define MAXEXDATANUM (16384)
    static UINT m_InfoExdata[5][MAXEXDATANUM];  // 0-Socket No., 1- Pulse No., 2-DI, 3-ex_data_upper, 4-ex_data_lower
    bool m_bSaveExdata;
    std::string m_filenameExd;
//停止采集后打印每个控制盒前128个通道数据
#define DDataNUM (128)
    static UINT m_uDetData[MAX_MCBNUM][DDataNUM];
    static ULONG m_uDBParameter[MAX_MCBNUM][DDataNUM];
    static ULONG m_uDBPulse[MAX_MCBNUM][DDataNUM];
//停止采集后打印每个控制盒及级联的第一个DB板最后一次返回的数据包内包含的状态
    static OneDBLineOnePackageStatue m_pOneDBLineOnePackageStatue[MAX_MCBNUM];//
///////////////////////////////////////////////////////
public:
    //模块、探测器工作时序控制、参数相关
    //系统及各MCB初始化状态
    //bool m_bMCBxInInitialing =false;

    // 解耦改造（对照 V2.0.1.1）：以下标志跨接收/解析/主控线程访问，
    // 类型从 bool/int 改为 volatile MCB_LONG（普通赋值兼容，读取走 AtomicLoadL）
    static volatile MCB_LONG m_MCBRunSts[MAX_MCBNUM]; //bit 0-pre start, 1-pre stop cmd, 2-pre stop normal
    static volatile MCB_LONG m_MCBReadySts[MAX_MCBNUM]; //bit 0-connection, 1-MB fpga, 2- AD fpga, 3-Reg set
    bool m_bSetDMInfo[MAX_MCBNUM];  // if set DM info
    //check if slave MCB ready to start
    //从控制盒是否准备就绪
    bool m_bSlaveStart =false;
    volatile MCB_LONG m_bPreStart =0;
    //是否停止取数
    static volatile MCB_LONG m_bPreDataEnd ;
    static volatile MCB_LONG m_bPreStop;
    static volatile MCB_LONG m_bStart;
	//已收到停止采集命令
	static volatile MCB_LONG m_bRecStopCmd;
    //是否自动重连
    bool m_bconnectauto;

	//硬件序列号取值模式
	//默认为1，取实际返回包裹数序列号做硬件序列号
	//改为0，取软件包序列号
	//（临时解决存在硬件序列号有重复的现象（换控制盒仍旧存在），导致多个盒级联时存在数据对不齐的问题）（软件端按回调数据里面的硬件触发数存图）
	static bool m_bHWSerialNumberMode;

    //数据解析按智能（=0）还是高能 （=1） 模式解析处理
    static bool m_bHEorCXMode;
    //是否要求获取开始的 0xA5 数据包,默认需要获取
    static bool m_bNeedGet0xA5;


    //是否已获取到开始的0xA5数据包
    static bool m_bGet0xA5Packet[MAX_MCBNUM];
    //获取到0xA5数据包前已解析出的数据包
    static UINT m_nMiss0XA5PacketCnt[MAX_MCBNUM];
    //上一次数据采集是否成功完成标志
    static volatile MCB_LONG m_bPreDAQSucessFlag[MAX_MCBNUM];
    //探测器模块配置详细参数:MCB个数，探测器数目，行列值，通道总数
    static DMINFOHMCB4 m_DMInfo[MAX_MCBNUM];
    //控制盒相关参数
    //是否使能使用HMCB*的4端口
    //static bool m_bEnMCBx4;
    //控制盒个数
    static int m_nMCBNum;
    //4端口HMCB的DBLine个数
    //static int m_nMCBx4DBLine;
    //探测器AD转换位数
    static INT m_nADBit;
    //探测器数据上传软件位数
    bool m_bFcto16ADBit;
    int m_nTrig; // trig source 0 -inner ,1-outer
    //CB工作参数等
    Register m_reg;
    //探测器板工作寄存器配置参数
    UINT m_nADMCMDREG[MAX_MCBNUM][MAX_MCBADNUM];   //DM cmd register, bit0~2--DDC232 range
    //探测器板工作寄存器配置参数(支持多块多端口级联)
    UINT m_nMCBxDByREG[MAX_MCBNUM][MAX_DBLINENUM][MAX_MCBADNUM];   //DM cmd register, bit0~2--DDC232 range


    volatile MCB_LONG m_bDVReady;  // if device ready
    //
    std::string m_szMCBIPAddr;
    //??????????????????IP????
    std::string m_szMCBSetIPAddr[MAX_MCBNUM];
    static CRequestSocket* m_pDataSocket[MAX_MCBNUM];
    //
    //探测器FPGA配置bin文件
    std::string m_szADM_FPGAFilename;
    //每个控制板的探测器FPGA配置bin文件
    std::string m_szMCB_FPGAFilename[MAX_MCBNUM];
    //每个控制板的每个DBline的探测器FPGA配置bin文件
    std::string m_szMCBDBLine_FPGAFilename[MAX_MCBNUM][MAX_DBLINENUM];
    //MCB dbline bin file 配置标志位
    INT32U m_nDBLine_FPGAFileConfigFlag[MAX_MCBNUM][MAX_DBLINENUM];
    //ini配置参数文件路径
    std::string m_strMCBCtrlIniFile;
    //探测器增益设置save DM range set by APP, max 4 range
    unsigned char m_cDMRange[MAX_MCBNUM][(MAX_MCBADNUM+1)*4];
    /////////////////////////////////////////////////////
    static bool m_bZeroPadding;//????????????????????????????????????
    static long m_lPrePulseCnt[MAX_MCBNUM];//????????????????
    /////////////////////////////////////////////////////
public:
    //收到[Start]命令次数
    INT32U m_nRecvStartCmdCnt;
    //探测器数据解析相关
    FILE* cmd_pkg_parse_log_file;
    //
    static long pnDBlineDetectorData[MAX_MCBNUM][MAX_MCBADNUM][MAX_DMCOLNUM][MAX_DMROWNUM];
    //
    static INT32U parsed_pkg_n[MAX_MCBNUM];//已经解析的数据包个数
    static INT32U Update_pkg_n[MAX_MCBNUM];//已经解析且发送App更新的数据包个数
    static INT32U SC_1st_pkg_index[MAX_MCBNUM];//每一个DB Line返回的第一个数据包
    //
    static INT32U SC_1st_pkg_TriggCnt[MAX_MCBNUM];//每一个DB Line返回的第一个数据包内触发计数
    //static INT32U SC_Current_pkg_TriggCnt[MAX_MCBNUM];//每一个DB Line返回的当前数据包内触发计数
    //
    static INT32U SC_pre_pkg_index[MAX_MCBNUM];//每一个DB Line返回的前一个数据包索引
    static INT32U SC_current_pkg_index[MAX_MCBNUM];//每一个DB Line返回的当前数据包索引
    //
    //探测器数据解析相关
    static MCBData m_MCBData[MAX_MCBNUM + 1];
    static MCBData m_MCBDataBackup[MAX_MCBNUM];//前次数据备份
    static MCBData m_MCBDataTmp; // temporary data for channel adjust
    static MCBData_H m_MCBDataHeadTmp[MAX_MCBNUM]; // temporary data for AIN/DI...adjust
    //MCBData_H m_MCBDataHTmp[MAX_MCBNUM]; // temporary data for AIN/DI...adjust
    //unsigned char m_ADOriData[MAX_MCBNUM][MAX_MCBADNUM*MAX_DMROWNUM*MAX_DMCOLNUM*sizeof(long)];
    //Ul_Uc m_ADOriDataUL[MAX_MCBNUM][MAX_MCBADNUM*MAX_DMROWNUM*MAX_DMCOLNUM];
    /////////////////////////////////////////////////
    //软件记录的获取的数据包数
    static unsigned long m_scounter[MAX_MCBNUM];
    //指定MCBx少丢弃包裹数，使数据与其他MCB控制盒数据对齐，默认值为0。（主要为了使亮度校正数据与成像数据能调整对齐）
    static INT32U m_nDiscardNum[MAX_MCBNUM];
    //指定MCBx是否进入诊断模式
    bool m_bMCBxDTM[MAX_MCBNUM+1];

	/////////////////////////////////////////////////////
	//2026-03-19增加对CS0200M的支持
	//增加 [CHADJ] DEVICETYPEID 配置项
	//默认值为0，常规设备
	//1——CS0200M， 调整主控制盒的m_MCBData[nIndex].d_head.head_use.accEnergy为出束光机的编号( m_MCBData[nIndex].d_head.head_use.accEnergy = (long)l_angle);
	//出束光机的编号规则：光机触发信号接DI-0，光机循环信号接DI-1，DI-1为1时的光机编号为0，DI-1为0后每一次触发的光机编号+1
	static UINT m_nDeviceTypeID;
	//射线源序号
	static UINT m_nXRayGenN;
	/////////////////////////////////////////////////////


public:
    //日志记录相关
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
    //日志相关
    static std::string m_strLogFilePath;
    static bool m_bSetLogFilePath;
    static UINT m_nLogLevel;
    /////////////////////////////////
    //记录data socket原始数据
    static std::string m_SocketRawDatafilename;
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    static CStdioFile m_SocketRawDatafile;
#else
    static std::fstream m_SocketRawDatafile;
#endif
    static bool m_bSocketRawDatafile;
    static ULONGLONG  m_unSocketRawWrCnt;//写入记录的总数据字节数
    /////////////////////////////////
public:
//智能数据调整需求
#define MAX_SECTIONADJNUM 5
    static int m_nSectionFirstADNo[MAX_SECTIONADJNUM];
    static int m_nSectionLastADNo[MAX_SECTIONADJNUM];
    //
    static int m_nMCBxSectionFirstADNo[MAX_MCBNUM][MAX_SECTIONADJNUM];
    static int m_nMCBxSectionLastADNo[MAX_MCBNUM][MAX_SECTIONADJNUM];

    //智能扩展需求参数
    static int m_nAppADBit;
    //
    static UINT m_nALGValue;
    //
    static double m_fProgrammeGainAmp;
    //
    static double m_fMCBxDMyPGz[MAX_MCBNUM][MAX_MCBADNUM][MAX_DMROWNUM*MAX_DMCOLNUM/16];   //针对物品机应用，16个通道一组单独可设置浮点数增益
//    //只针对智能用单块多口控制模块时，多端口数据合并有效，高能无效
//    static bool m_bAllMCBDataMerging;//所有数据合并，触发一次回调
//    UINT m_nMergingDMNum;     //所有数据合并后的模块数
    //
public:
    //辅助功能
    //多列合一，多列合并求算术均值
    static bool m_bColCombine;
    //调整探测器模块数据与控制盒Ain、Di 数据错位1 个触发的问题
    static bool m_bAdjAinDi;
    /////////////////////////////////////
    //发送（接收）所有数据
    static int m_nSendAllRecorded;//进入send all 模式日志是否已记录 0 初始 1需要记录 2已记录
    static LONG m_nNeedSendData[MAX_MCBNUM];
    /////////////////////////////////////
    //待完善功能
    //自动获取探测器模块数量 0-不自动，1-自动
    bool m_bAutoDMNum;
    //发送所有数据
    bool m_bSendAllData;
    //等待控制盒传完所有数据
    bool m_bEnAllData[MAX_MCBNUM];

    static std::mutex m_Data_mutex[MAX_MCBNUM];
private:
    std::mutex m_Initial_mutex;

    /////////////////////////////////////////////////////
///////////////////////////////////////////////////////
};
#endif // !defined(AFX_MCBCTRLC_H__4C9078C7_F07D_4BFD_850D_39CB39720350__INCLUDED_)
