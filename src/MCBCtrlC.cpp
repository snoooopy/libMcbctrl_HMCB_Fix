// MCBCtrlC.cpp: implementation of the CMCBCtrlC class.
//
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#pragma comment(lib,"Version.lib")
#include "stdafx.h"
#include "Shlwapi.h"
#include<windows.h>
#else
#include "replenish_type.hpp"
#include <thread>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/timeb.h>
#if __cplusplus >= 201703L
#include <filesystem>
#else
#include <sys/stat.h>
#endif
#include "IniFile.h"
#include <dlfcn.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <mutex>
#include "nuctool.h"
#include "logAdapter.h"
#endif

//////////////////////////////////////////////////////////////////////


#include "MCBCtrl.h"
#include "MCBCtrlC.h"
//#include "IniFile.h"


#include <iostream>

//#include "ETH_func.h"
//#include "ECS_func.h"
//#include "FPGA_cfg.h"
#include "including.h"
#include "math.h"
#include <locale>
using namespace std;

void addrDL(){}

#include "MCBThread.h"
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define gstrHMCBDllVersion (("1.0.2"))
#else
std::string gstrHMCBDllVersion = "1.3.3";
#endif
#define gCmdReptN	(1)
bool m_bMCBxInInitialing = false;
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define DEBUG_NEW new
#endif

#ifdef VC6_BUILD		 // Build project with VC6
#define strcpy_s strcpy  // use strcpy when use VC6
#endif
#define SOCKETBUFSIZE (1440*4)
//MCB 线程
extern CMCBThread* gpMCBThread;
//日志记录水平
//补充记录一些函数操作及返回值日志记录
#define gLogLevel1	1
//记录一些数据解析部分的日志
#define gLogLevel2	2
//留存待用
#define gLogLevel3	3
//留存待用
#define gLogLevel4	4
//记录socket接收的数据且另存为txt文件
#define gLogLevel5	5
//打印部分程序调用流转日志记录
#define gLogLevel6	6

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
//////////////////////////////////////////////////////////////
//定义多线程同步临界区
CRITICAL_SECTION ics;
#endif
//////////////////////////////////////////////////////////////////////
//数据通信socket，每个控制盒一个
CRequestSocket* CMCBCtrlC::m_pDataSocket[MAX_MCBNUM];
std::string g_LogFilePath = "/home/nuctech/";
//2022-06-04
//用于socket 数据传输带宽统计
ULONG CMCBCtrlC::m_SocketRecBytes[MAX_MCBNUM];
DWORD CMCBCtrlC::m_TimeSaveSocketData[MAX_MCBNUM];
//////////////////////////////////////////////////////////////////////
//日志相关
char CMCBCtrlC::m_szPath[PATH_MAX];
std::string CMCBCtrlC::m_filename;
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
CStdioFile CMCBCtrlC::m_file;
#else
std::fstream CMCBCtrlC::m_file;
#endif
bool CMCBCtrlC::m_bCLogPath;
/////////////////////////////////
//记录data socket原始数据
std::string CMCBCtrlC::m_SocketRawDatafilename;
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
CStdioFile CMCBCtrlC::m_SocketRawDatafile;
#else
std::fstream CMCBCtrlC::m_SocketRawDatafile;
#endif
bool  CMCBCtrlC::m_bSocketRawDatafile;
ULONGLONG  CMCBCtrlC::m_unSocketRawWrCnt;//写入记录的总数据字节数
/////////////////////////////////
DWORD CMCBCtrlC::m_timehot[MAX_MCBNUM];
//设备启动采集，x秒内没有收到数据报 no data 无数据错误
DWORD CMCBCtrlC::m_timenodata[MAX_MCBNUM];
DWORD  CMCBCtrlC::m_timeprestop;
DWORD CMCBCtrlC::m_timepreend;
DWORD CMCBCtrlC::m_timeSocketRecData[MAX_MCBNUM];
bool CMCBCtrlC::m_bZeroPadding;
long CMCBCtrlC::m_lPrePulseCnt[MAX_MCBNUM];
//
std::string CMCBCtrlC::m_DataErrorfilename[MAX_MCBNUM];
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
CStdioFile CMCBCtrlC::m_DataErrorfile[MAX_MCBNUM];
#else
std::fstream CMCBCtrlC::m_DataErrorfile[MAX_MCBNUM];
#endif
bool CMCBCtrlC::m_bDataErrorCLogPath[MAX_MCBNUM];
//
UINT CMCBCtrlC::m_bDebug;//是否为调试版本
//
INT32U CMCBCtrlC::parsed_pkg_n[MAX_MCBNUM];//已经解析的数据包个数
INT32U CMCBCtrlC::Update_pkg_n[MAX_MCBNUM];//已经解析且发送App更新的数据包个数
INT32U CMCBCtrlC::SC_1st_pkg_index[MAX_MCBNUM];//每一个DB Line返回的第一个数据包
INT32U CMCBCtrlC::SC_1st_pkg_TriggCnt[MAX_MCBNUM];//每一个DB Line返回的第一个数据包内触发计数
//INT32U CMCBCtrlC::SC_Current_pkg_TriggCnt[MAX_MCBNUM];//每一个DB Line返回的当前数据包内触发计数
//
INT32U CMCBCtrlC::SC_pre_pkg_index[MAX_MCBNUM];//每一个DB Line返回的前一个数据包索引
INT32U CMCBCtrlC::SC_current_pkg_index[MAX_MCBNUM];//每一个DB Line返回的当前数据包索引
//
//是否使能使用DAQ Tirgger控制采集启停
//bool CMCBCtrlC::m_bEnCtrlDAQTrig;
//是否使能使用HMCB*的4端口
bool CMCBCtrlC::m_bEnMCBx4;
//高低能指示信号借用为钩铛信号
bool CMCBCtrlC::m_bGoudang_S4;
//三能加速器使能(三能加速器（DI(3)为高，则为0.7MeV，能量指示值为2或3，否则为高低双能，能量指示值为0或1）)
bool CMCBCtrlC::m_bThreeEnergyAcc_En;
//
int CMCBCtrlC::m_nMCBNum;

//
int CMCBCtrlC::m_MCBReadySts[MAX_MCBNUM];
int CMCBCtrlC::m_MCBRunSts[MAX_MCBNUM]; //bit 0-pre start, 1-pre stop cmd, 2-pre stop normal

//硬件序列号取值模式
//默认为1，取实际返回包裹数序列号做硬件序列号
//改为0，取软件包序列号
//（临时解决存在硬件序列号有重复的现象（换控制盒仍旧存在），导致多个盒级联时存在数据对不齐的问题）（软件端按回调数据里面的硬件触发数存图）
bool CMCBCtrlC::m_bHWSerialNumberMode;
//数据解析按智能（=0）还是高能（=1）模式解析处理
bool  CMCBCtrlC::m_bHEorCXMode;
//是否要求获取开始的0xA5数据包,默认为需要获取
bool CMCBCtrlC::m_bNeedGet0xA5;

//是否已获取到开始的0xA5数据包
bool CMCBCtrlC::m_bGet0xA5Packet[MAX_MCBNUM];
//获取到0xA5数据包前已解析出的数据包
UINT CMCBCtrlC::m_nMiss0XA5PacketCnt[MAX_MCBNUM];
//上一次数据采集是否成功完成标志
bool CMCBCtrlC::m_bPreDAQSucessFlag[MAX_MCBNUM];

bool CMCBCtrlC::m_bPreDataEnd =false;
bool CMCBCtrlC::m_bPreStop =false;
bool CMCBCtrlC::m_bStart=false;
//已收到停止采集命令
bool CMCBCtrlC::m_bRecStopCmd = false;	
	
//UINT  CMCBCtrlC::m_nMissAA55Cnt;
std::map<int,UINT> CMCBCtrlC::m_nMissAA55Cnt;
//ULONG  CMCBCtrlC::m_nWaitDataTrigger[MAX_MCBNUM];
unsigned long CMCBCtrlC::m_scounter[MAX_MCBNUM];
//指定MCBx少丢弃包裹数，使数据与其他MCB控制盒数据对齐，默认值为0。（主要为了使亮度校正数据与成像数据能调整对齐）
INT32U CMCBCtrlC::m_nDiscardNum[MAX_MCBNUM];
//
DMINFOHMCB4 CMCBCtrlC::m_DMInfo[MAX_MCBNUM];
MCBData CMCBCtrlC::m_MCBData[MAX_MCBNUM + 1];
//
MCBData CMCBCtrlC::m_MCBDataBackup[MAX_MCBNUM];//前次数据备份
//
MCBData CMCBCtrlC::m_MCBDataTmp;
MCBData_H CMCBCtrlC::m_MCBDataHeadTmp[MAX_MCBNUM]; // temporary data for AIN/DI...adjust

long CMCBCtrlC::pnDBlineDetectorData[MAX_MCBNUM][MAX_MCBADNUM][MAX_DMCOLNUM][MAX_DMROWNUM];
//
double  CMCBCtrlC::m_fMCBxDMyPGz[MAX_MCBNUM][MAX_MCBADNUM][MAX_DMROWNUM*MAX_DMCOLNUM/16];   //针对物品机应用，16个通道一组单独可设置浮点数增益

//bool CMCBCtrlC::m_bAllMCBDataMerging;//所有数据合并，触发一次回调
int CMCBCtrlC::m_nAdjChn[MAX_CHNADJNUM];
int CMCBCtrlC::m_nMCBxAdjChn[MAX_MCBNUM][MAX_CHNADJNUM];
int CMCBCtrlC::m_nMCBxDByAdjChn[MAX_MCBNUM][MAX_DBLINENUM][MAX_CHNADJNUM];  //adjust mode: 0-NONE, 1-ROW, 2-COLUMN, 3-MODULE, 4-ALL_ROW
//
int  CMCBCtrlC::m_nSectionFirstADNo[MAX_SECTIONADJNUM];
int  CMCBCtrlC::m_nSectionLastADNo[MAX_SECTIONADJNUM];
//
int  CMCBCtrlC::m_nMCBxSectionFirstADNo[MAX_MCBNUM][MAX_SECTIONADJNUM];
int  CMCBCtrlC::m_nMCBxSectionLastADNo[MAX_MCBNUM][MAX_SECTIONADJNUM];
//
unsigned short CMCBCtrlC::m_godown[GODOWNNUM];
UINT CMCBCtrlC::m_uDetData[MAX_MCBNUM][DDataNUM];
ULONG CMCBCtrlC::m_uDBParameter[MAX_MCBNUM][DDataNUM];
ULONG CMCBCtrlC::m_uDBPulse[MAX_MCBNUM][DDataNUM];
UINT CMCBCtrlC::m_InfoExdata[5][MAXEXDATANUM];  // 0-Socket No., 1- Pulse No., 2-DI, 3-ex_data_upper, 4-ex_data_lower
//
//停止采集后打印每个控制盒及级联的第一个DB板状态
OneDBLineOnePackageStatue CMCBCtrlC::m_pOneDBLineOnePackageStatue[MAX_MCBNUM];//
//////////////////////////////////////////////////////////////////////
std::string CMCBCtrlC::m_strLogFilePath;
bool CMCBCtrlC::m_bSetLogFilePath;
UINT CMCBCtrlC::m_nLogLevel;
//
int CMCBCtrlC::m_nADBit;
int CMCBCtrlC::m_nAppADBit;
//
UINT CMCBCtrlC::m_nALGValue;
//
double CMCBCtrlC::m_fProgrammeGainAmp;

/////////////////////////////////////////////////////
//2026-03-19增加对CS0200M的支持
//增加 [CHADJ] DEVICETYPEID 配置项
//默认值为0，常规设备
//1——CS0200M， 调整主控制盒的m_MCBData[nIndex].d_head.head_use.accEnergy为出束光机的编号( m_MCBData[nIndex].d_head.head_use.accEnergy = (long)l_angle);
//出束光机的编号规则：光机触发信号接DI-0，光机循环信号接DI-1，DI-1为1时的光机编号为0，DI-1为0后每一次触发的光机编号+1
UINT CMCBCtrlC::m_nDeviceTypeID;
//射线源序号
UINT CMCBCtrlC::m_nXRayGenN;
//////////////////////////////////////////////////////////////////////
//辅助功能
//多列合一，多列合并求算术均值
bool CMCBCtrlC::m_bColCombine;
//调整探测器模块数据与控制盒Ain、Di 数据错位1 个触发的问题
bool CMCBCtrlC::m_bAdjAinDi;
ULONG CMCBCtrlC::m_lcounter =0;

//仅用于调试节点计
ULONG  CMCBCtrlC::m_lDebugCnt;
/////////////////////////////////////
//发送（接收）所有数据
int CMCBCtrlC::m_nSendAllRecorded;//进入send all 模式日志是否已记录 0 初始 1需要记录 2已记录
LONG CMCBCtrlC::m_nNeedSendData[MAX_MCBNUM];
std::mutex CMCBCtrlC::m_Data_mutex[MAX_MCBNUM];
//////////////////////////////////////////////////////////////////////
//#define GLB_DATA_BUF_SIZE	(100000)	//不能超过32位数的表示范围，必须大于1个SC数据包长度
//#define GLB_DATA_BUF_SIZE	(64*1024*1024)	//不能超过32位数的表示范围，必须大于1个SC数据包长度

#define LOGFILE_SIZE	(50*1024*1024)	//50M
//extern INT8U glb_data_buf[MAX_MCBNUM][GLB_DATA_BUF_SIZE];
extern vector<unsigned char> glb_data_buf_vetor[MAX_MCBNUM];
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
extern HANDLE hMutex;
#else
//extern boost::shared_mutex mutex1;
extern mutex _mutex;
#endif

extern ULONGLONG glb_data_AccWrBytes[MAX_MCBNUM];//记录总接收数据字节数
extern ULONGLONG glb_data_AccRdBytes[MAX_MCBNUM];//记录总读出处理字节数

//extern INT32U gbl_data_buf_wraddr[MAX_MCBNUM];			//记录数据缓存区下一个数据的写入位置
//extern INT32U gbl_data_buf_rdaddr[MAX_MCBNUM];			//记录下一个数据的读取地址
extern INT32S gbl_sc_pkg_index[MAX_MCBNUM];			//记录各个DB Line上一个sc pkg的包编号

//一个DBLine的size= 10字节逻辑 + DB数*一个DB包size + 1个CB包字节
//MAX_MCBNUM个MCB，MAX_DBLINENUM个DBline，每个DBline需要接收的字节数
extern INT32U gbl_DBLine_Size[MAX_MCBNUM][MAX_DBLINENUM];
//
//INT32U xB_pkg_index_Temp1[MAX_MCBNUM] = { 0 };
//INT32U xB_pkg_index_Temp2[MAX_MCBNUM] = { 0 };
//INT32U xB_pkg_index_Temp3[MAX_MCBNUM] = { 0 };
//--------------------------------------------------------------
//      全局数据读取缓存区
//--------------------------------------------------------------
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
std::string str_EDAQ_reg_name_list[EDAQ_REG_NUM] =
    {
        ("EDAQ_FIRMWARE_VERSION_REG"),//Reg 0
        ("EDAQ_GENERAL_CTRL_REG"),	//1
        ("EDAQ_GENERAL_STATUS_REG"),  //2
        ("EDAQ_ECS_TEST_REG"),		//3
        ("EDAQ_EEPROM_CTRL_REG"),      //4
        ("EDAQ_EEPROM_RDDATA_REG"),    //5
        ("EDAQ_DB_COMM_CTRL_REG"),    //6
        ("EDAQ_DB_COMM_STATUS_REG"),  //7
        ("EDAQ_DB_NUM_SET_REG"),		//8
        ("EDAQ_DB_NUM_STATUS_REG"),	//9

        ("EDAQ_DB_DAQ_CMD_EXCT_TIME_SET_REG"),	//10
        ("EDAQ_DB_NON_DAQ_CMD_EXCT_TIME_SET_REG"),//11
        ("EDAQ_DB_INTEGRATION_TIME_SET_REG"),		//12
        ("EDAQ_INT_TRIG_PERIOD_SET_REG"),			//13
        ("EDAQ_INT_TRIG_PULSE_WIDTH_SET_REG"),	//14

        ("EDAQ_RB_ACCESS_CTRL_REG"),			//15
        ("EDAQ_DB_FPGA_CFG_DATA_WR_REG"),		//16
        ("EDAQ_DDI_LATCH_LATENCY_SET_REG"),	//17

        ("CB_FIFO_ERR_LOG_REG"),	//18
        ("CB_ERR_LOG_REG1"),		//19
        ("CB_ERR_LOG_REG2"),		//20
        ("DB_FPGA_CFG_DATA_BYTE_NUM_REG"),	//21
        (""),(""),
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// IMPLEMENT_SERIAL(CMCBCtrlC, CObject, 0)

CMCBCtrlC::CMCBCtrlC()
{
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    int i = 0;
    int j = 0;
    int k = 0;
    CString sz_szInfo = ("");
    ////////////////////////////////////////////////////////////////
    //获取程序所在文件路径
    if( !GetModuleFileName( ghInstance, m_szPath, MAX_PATH ) )
    {
        std::cout << "GetModuleFileName failed! \n";
        m_bPath = false;
    }
    else
    {
        PathRemoveFileSpec(m_szPath);
        m_bPath =true;
    }
    ////////////////////////////////////////////////////////////////
    //系统时间
    CTime m_t;
    std::string m_str = "";
    ////////////////////////////////////////////////////////////////
    std::string filename = ("");
    std::string SysInifilename = ("");
    std::string strDeviceType = ("");
    DWORD ReNChars = 0;
    //判断是否存在 D:\\XRayV3\\Config\\Setting.ini 或 D:\\XRayV4\\Config\\Setting.ini文件且设置了("MainConfig"), ("DeviceType"),参数
    DWORD nXRayV = IsExistXRayVxSetting();
    //
    if(3 == nXRayV)
    {
        SysInifilename = ("D:\\XRayV3\\Config\\Setting.ini");
        ReNChars = GetPrivateProfileString(("MainConfig"), ("DeviceType"), strDeviceType, strDeviceType.GetBuffer(MAX_PATH), MAX_PATH, SysInifilename);
        filename.Format(("D:\\XRayV3\\Config\\MCBCtrl_%s.ini"), strDeviceType);
    }
    else if(4 == nXRayV)
    {
        SysInifilename = ("D:\\XRayV4\\Config\\Setting.ini");
        ReNChars = GetPrivateProfileString(("MainConfig"), ("DeviceType"), strDeviceType, strDeviceType.GetBuffer(MAX_PATH), MAX_PATH, SysInifilename);
        filename.Format(("D:\\XRayV4\\Config\\MCBCtrl_%s.ini"), strDeviceType);
    }
    else
    {
        if(m_bPath)
        {
            filename.Format(("%s\\MCBCtrl.ini"), m_szPath);
        }
        else
        {
            filename = ("MCBCtrl.ini");
        }
    }
    m_strMCBCtrlIniFile = filename;
    //
    //收到[Start]命令次数
    m_nRecvStartCmdCnt = 0;
    ///////////////////////////////////////////////////////
    //socket 初始化
    for (i=0; i< MAX_MCBNUM; i++)
    {
        m_pDataSocket[i] = NULL;
    }
    ///////////////////////////////////////////////////////
    // read ini file
    CIniFile IniFile;
    IniFile.SetFileName(m_strMCBCtrlIniFile);
    /////////////////////////////////////////////////////////////////////////
    //获取设置的日志路径
    m_strLogFilePath = IniFile.GetString(("LOG"), ("LOGFILEPATH"), (""));
    int nRevFindEndCh = m_strLogFilePath.ReverseFind(('\\'));
    int nLogFilePathLength = m_strLogFilePath.GetLength();
    //若最后一个字符为盘符'\' 则去掉
    if((nRevFindEndCh + 1) == nLogFilePathLength)
    {
        m_strLogFilePath = m_strLogFilePath.Left(nRevFindEndCh);
    }
    /////////////////////
    if(m_strLogFilePath.GetLength() <= 0)
    {
        m_bSetLogFilePath = FALSE;
    }
    else
    {
        m_bSetLogFilePath = true;
    }
    m_nLogLevel = IniFile.GetInt(("LOG"), ("LOGLEVEL"), 0);
    ///////////////////////////////////////////////////////
    m_bSocketRawDatafile = FALSE;
    m_unSocketRawWrCnt = 0;//写入记录的总数据字节数
    ///////////////////////////////////////////////////////
    CString szfilenamebak = ("");
    CString szfilenamebakold = ("");
    //
    if (m_bPath)
    {
        ///////////////////
        for (i=0; i< MAX_MCBNUM; i++)
        {
            m_bDataErrorCLogPath[i] = FALSE;
        }
        //
        m_t = time(NULL);
        m_bCLogPath =true;
        //若设置了日志路径,则取设置值
        if(m_bSetLogFilePath)
        {
            m_filename.Format(("%s\\HMCBCtrl.%s.log"), m_strLogFilePath, m_t.Format(("%Y-%m-%d")));
            szfilenamebak.Format(("%s\\HMCBCtrlBak.%s.log"), m_strLogFilePath, m_t.Format(("%Y-%m-%d")));
            szfilenamebakold.Format(("%s\\HMCBCtrlBakOld.%s.log"), m_strLogFilePath, m_t.Format(("%Y-%m-%d")));
            ///////////////////
            for (i=0; i<MAX_MCBNUM; i++)
            {
                m_DataErrorfilename[i].Format(("%s\\HMCB%dCtrlDataError.%s.log"), m_strLogFilePath, i, m_t.Format(("%Y-%m-%d")));
            }
            ///////////////////
        }
        //若没有设置日志路径,则取当前路径
        else
        {
            m_filename.Format(("%s"), m_szPath);
            szfilenamebak.Format(("%s"), m_szPath);
            szfilenamebakold.Format(("%s"), m_szPath);
            m_filename +=(("\\HMCBCtrl.log"));
            szfilenamebak +=(("\\HMCBCtrlBak.log"));
            szfilenamebakold +=(("\\HMCBCtrlBakOld.log"));
            //
            ///////////////////
            for (i=0; i<MAX_MCBNUM; i++)
            {
                m_DataErrorfilename[i].Format(("%s\\HMCB%dCtrlDataError.log"), m_szPath, i);
            }
            ///////////////////
        }
        //
        if (m_file.Open(m_filename, CFile::modeWrite |CFile::modeCreate |CFile::modeNoTruncate
                                        |CFile::typeText |CFile::shareDenyWrite ) ==0) //
        {
            std::cout <<(("open LOG file error!\n"));
            m_bCLogPath =FALSE;
        }
        else
        {
            //if (m_file.GetLength() > 20*1024*1024)
			if (m_file.GetLength() > LOGFILE_SIZE)
            {
                CopyFile(szfilenamebak, szfilenamebakold, FALSE);
                CopyFile(m_filename, szfilenamebak, FALSE);
                m_file.SetLength(0);
            }
            else
            {
                m_file.SeekToEnd();
            }
            m_t =time(NULL);
            m_bCLogPath =true;
            m_str =__DATE__;  //build data and time
            m_str +=(" ");
            m_str +=__TIME__;
#ifdef _DEBUG
            m_str += (", Debug");
#else
            m_str += (", Release");
#endif
#ifdef VC6_BUILD		 // Build project with VC6
            m_str += ("(VC6)");
#else
#ifdef VS2008_BUILD
            m_str += ("(VS2008)"); // Build project with VS2010
#else
#ifdef VS2010_BUILD
            m_str += ("(VS2010)"); // Build project with VS2010
#endif  //VS2010_BUILD
#endif  //VS2010_BUILD
#endif  //VC6_BUILD

#ifdef MCBx64
            sz_szInfo = m_t.Format(("\n%Y-%m-%d %H:%M:%S.000  ")) +("HMCBCtrl64(Ver-") + GetProductVersion() +(") ") + m_str +(") begin...\n");
#else
            sz_szInfo =m_t.Format(("\n%Y-%m-%d %H:%M:%S.000  ")) +("HMCBCtrl(Ver-") + GetProductVersion() +(") ") + m_str +(") begin...\n");
#endif
            std::cout <<(sz_szInfo);
            m_file.WriteString(sz_szInfo);
            //////////////////////////////////////////
        }
    }
#else
    int i = 0;
    int j = 0;
    int k = 0;
    CIniFile inif;
    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    ////////////////////////////////////////////////////////////////
    //获取程序所在文件路径

   if(g_strIniFilePath=="")
   {
        Dl_info dlinfo;
        int ret = dladdr((void*)addrDL,&dlinfo);

        if( ret==0 )
        {
            std::cout << "GetModuleFileName failed! \n";
            m_bPath = false;
        }
        else
        {
            std::string dlnm = dlinfo.dli_fname;
            std::cout << " dlnm : " << dlnm << std::endl;
            if(dlnm == ""|| dlnm.find("/") == std::string::npos)
            {
                std::cout << " @@@@@@@@@@@@@@  " <<std::endl;
                char * pdllpth = getenv("HMCBCTRL_SO_PATH");
                if(pdllpth == NULL)
                {
                    sprintf(m_szPath,"/home/nuctech/Nuctech_Services/OSS/Release/mcb_reader");
                }
                else
                {
                    printf(" pdllpth : %s \n",pdllpth);
                    strcpy(m_szPath,pdllpth);
                    dlnm = m_szPath;
                    std::cout << " dlnm : " << dlnm << std::endl;
                }
            }
            else
            {
                std::cout << " ####################  " <<std::endl;
                std::string dlpath(dlnm.begin(),dlnm.begin() + dlnm.rfind("/"));
                strcpy(m_szPath,dlpath.c_str()/*dlinfo.dli_fname*/);
            }
            m_bPath =true;
        }
    }
   else
   {
        strcpy(m_szPath,g_strIniFilePath.c_str());
        m_bPath =true;
   }
    ////////////////////////////////////////////////////////////////
    //系统时间

    std::string m_str = "";
    ////////////////////////////////////////////////////////////////
    std::string filename = ("");
    std::string SysInifilename = ("");
    std::string strDeviceType = ("");
    DWORD ReNChars = 0;
    //判断是否存在 D:\\XRayV3\\Config\\Setting.ini 或 D:\\XRayV4\\Config\\Setting.ini文件且设置了("MainConfig"), ("DeviceType"),参数
    DWORD nXRayV = IsExistXRayVxSetting();
    //
    if(3 == nXRayV)
    {
        SysInifilename = ("/home/nuctech/OSS/XRayV3/Config/Setting.ini");
        inif.SetFileName(SysInifilename);
        inif.GetString("MainConfig","DeviceType",strDeviceType);
        memset(tmp,0,512);
        sprintf(tmp,"/home/nuctech/OSS/XRayV3/Config/MCBCtrl_%s.ini", strDeviceType.c_str());
        filename = tmp;
    }
    else if(4 == nXRayV)
    {
        SysInifilename = ("/home/nuctech/OSS/XRayV4/Config/Setting.ini");
        inif.SetFileName(SysInifilename);
        inif.GetString("MainConfig","DeviceType",strDeviceType);
        memset(tmp,0,512);
        sprintf(tmp,"/home/nuctech/OSS/XRayV4/Config/MCBCtrl_%s.ini", strDeviceType.c_str());
        filename = tmp;
    }
    else
    {
        if(m_bPath)
        {
            memset(tmp,0,512);
            sprintf(tmp,"%s/MCBCtrl.ini", m_szPath);
            filename = tmp;
        }
        else
        {
            filename = ("MCBCtrl.ini");
        }
    }
    m_strMCBCtrlIniFile = filename;
    //
    //收到[Start]命令次数
    m_nRecvStartCmdCnt = 0;
    ///////////////////////////////////////////////////////
    //socket 初始化
    for (i=0; i< MAX_MCBNUM; i++)
    {
        m_pDataSocket[i] = NULL;
    }
    ///////////////////////////////////////////////////////
    // read ini file
    CIniFile IniFile;

    IniFile.SetFileName(m_strMCBCtrlIniFile);
    /////////////////////////////////////////////////////////////////////////
    //获取设置的日志路径
    m_strLogFilePath = IniFile.GetString(("LOG"), ("LOGFILEPATH"), (""));
    g_LogFilePath = m_strLogFilePath;
    int nRevFindEndCh = m_strLogFilePath.rfind(('/'));
    int nLogFilePathLength = m_strLogFilePath.length();
    // std::cout << " ##############    "<< m_strLogFilePath << std::endl;
    //若最后一个字符为盘符'\' 则去掉
    if((nRevFindEndCh + 1) == nLogFilePathLength)
    {
        IniFile.Left(m_strLogFilePath,nRevFindEndCh);
    }
    /////////////////////
    if(m_strLogFilePath.length() <= 0)
    {
        m_bSetLogFilePath = false;
    }
    else
    {
        m_bSetLogFilePath = true;
    }
    m_nLogLevel = IniFile.GetInt(("LOG"), ("LOGLEVEL"), 0);
    ///////////////////////////////////////////////////////
    m_bSocketRawDatafile = false;
    m_unSocketRawWrCnt = 0;//写入记录的总数据字节数
    ///////////////////////////////////////////////////////
    std::string szfilenamebak = ("");
    std::string szfilenamebakold = ("");
    //
    if (m_bPath)
    {
        ///////////////////
        for (i=0; i< MAX_MCBNUM; i++)
        {
            m_bDataErrorCLogPath[i] = false;
        }
        //
        m_bCLogPath =true;
        //若设置了日志路径,则取设置值
        if(m_bSetLogFilePath)
        {
            time_t t;
            time(&t);
            struct tm * timeinfo = localtime(&t)/*gmtime(&t)*/;
            char szUCT[50] = { 0 };
            strftime(szUCT, 50, "%Y-%m-%d", timeinfo);

            m_filename = m_strLogFilePath + "/HMCBCtrl."+ std::string(szUCT)+ ".log";
            szfilenamebak = m_strLogFilePath + "/HMCBCtrlBak."+ szUCT+ ".log";
            szfilenamebakold = m_strLogFilePath + "/HMCBCtrlBakOld."+ szUCT +".log";
            ///////////////////
            for (i=0; i<MAX_MCBNUM; i++)
            {
                m_DataErrorfilename[i] = m_strLogFilePath + "/HMCB" + std::to_string(i)+ "CtrlDataError."+ szUCT +".log";
            }
            ///////////////////
        }
        //若没有设置日志路径,则取当前路径
        else
        {

            m_filename=m_szPath;
            szfilenamebak=m_szPath;
            szfilenamebakold=m_szPath;
            m_filename +=(("/HMCBCtrl.log"));
            szfilenamebak +=(("/HMCBCtrlBak.log"));
            szfilenamebakold +=(("/HMCBCtrlBakOld.log"));
            //
            ///////////////////
            for (i=0; i<MAX_MCBNUM; i++)
            {
                m_DataErrorfilename[i] = std::string(m_szPath) + "/HMCB"+ std::to_string(i) + "CtrlDataError.log";
            }
            ///////////////////
        }
        //
        m_file.open(m_filename,std::ios::out|std::ios::app);

        if (!m_file.is_open()) //
        {
            std::cout <<(("open LOG file error! 3 \n"));
            std::cout << m_filename << std::endl;
            m_bCLogPath = false;
        }
        else
        {
#if __cplusplus >= 201703L
            if(!std::filesystem::exists(m_filename))
            {
                std::cout << " file exit error " << std::endl;
                m_file.open(m_filename,std::ios::out|std::ios::app);
            }
            //if ( std::filesystem::file_size(m_filename) > 20*1024*1024)
			if ( std::filesystem::file_size(m_filename) > LOGFILE_SIZE)
            {
                std::filesystem::remove(szfilenamebakold);
                std::filesystem::copy_file(szfilenamebak, szfilenamebakold);
                std::filesystem::remove(szfilenamebak);
                std::filesystem::copy_file(m_filename, szfilenamebak);
#else
            struct stat statbuf;
            stat(m_filename.c_str(), &statbuf);
            // 获取文件大小
            size_t filesize = statbuf.st_size;
           // if(filesize > 20*1024*1024)
			 if(filesize > LOGFILE_SIZE)
            {
                std::string strCpyCmd1 = "cp " + szfilenamebak + " " + szfilenamebakold;
                popen(strCpyCmd1.c_str(),"r");
                std::string strCpyCmd2 = "cp " + m_filename + " " + szfilenamebak;
                popen(strCpyCmd2.c_str(),"r");
#endif
                m_file.close();
                m_file.open(m_filename, std::ios::in|std::ios::trunc);
            }
            else
            {
                m_file.seekg(0,std::ios_base::end);
            }
            m_bCLogPath =true;
            m_str =__DATE__;  //build data and time
            m_str +=(" ");
            m_str +=__TIME__;
#ifdef _DEBUG
            m_str += (", Debug");
#else
            m_str += (", Release");
#endif

            m_str +="(g++)";

            time_t t;
            time(&t);
            struct tm * timeinfo = localtime(&t)/*gmtime(&t)*/;
            char szUCT[50] = { 0 };
            strftime(szUCT, 50, "\n%Y-%m-%d %H-%M-%S.000  ", timeinfo);

            std::string strUct = szUCT;
            // GetProductVersion();
            sz_szInfo = strUct + ("HMCBCtrl(Ver-") + "1.3.3" +(") ") + m_str +(") begin...\n");

            std::cout <<(sz_szInfo);
            m_file<<(sz_szInfo);
            //////////////////////////////////////////
        }
        ////////////////////////////////////////////////////////////////
        //从ini文件中读取参数
        ReadConfigParaFromIni();
    }
#endif
}


void CMCBCtrlC::ReadConfigParaFromIni()
{
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    int i = 0;
    int j = 0;
    int k = 0;
    CString str = ("");
    CString sz_szInfo = ("");
    //////////////////////////////////////////////////////////////////////////////////////////
    sz_szInfo.Format(("Read configure parameters from %s.\n"), m_strMCBCtrlIniFile);
    std::cout <<(sz_szInfo);
    WriteLogFile(sz_szInfo);
    ///////////////////////////////////////////////////////
    // read ini file
    CIniFile IniFile;
    IniFile.SetFileName(m_strMCBCtrlIniFile);
    ///////////////////////////////////////////////////////
    m_nMCBNum = (unsigned short) IniFile.GetInt(("MCB"), ("MCBNUM"), 1);
    if (m_nMCBNum > MAX_MCBNUM)
    {
        m_nMCBNum = MAX_MCBNUM;
    }
    else if (m_nMCBNum <1)
    {
        m_nMCBNum =1;
    }
    ///////////////////////////////////////////////////////
    //是否使能使用DAQ Tirgger控制采集启停
    m_bEnCtrlDAQTrig = false;//初始值
    //
    m_bEnMCBx4 = (IniFile.GetInt(("MCB"), ("ENMCBX4"), 0) ==1);
    //
    m_nMCBx4DBLine = (unsigned short) IniFile.GetInt(("MCB"), ("MCBNUM"), 1);
    if (m_nMCBx4DBLine > MAX_DBLINENUM)
    {
        m_nMCBx4DBLine = MAX_DBLINENUM;
    }
    else if (m_nMCBx4DBLine <1)
    {
        m_nMCBx4DBLine = 1;
    }
    ///////////////////////////////////////////////////////
    m_szMCBIPAddr = IniFile.GetString(("CONNECTION"), ("IPADDR"), ("192.111.111.211"));

    m_bconnectauto = (IniFile.GetInt(("CONNECTION"), ("AUTOCONNECT"), 1) ==1);
    //
    str.Format(("HMCB IP Address:%s, CTRL port:%d, Data port:%d, autoconnect=%d.\n"), IniFile.GetString(("CONNECTION"), ("IPADDR"), ("192.111.111.211")), ETH_CTRL_PORT, ETH_DATA_PORT, m_bconnectauto);
    std::cout <<(str);
    WriteLogFile(str);
    ////////////////////////////////////////////////////////
    m_bFcto16ADBit = false;
    //
    m_nADBit = IniFile.GetInt(("ADM"), ("ADBIT"), 20);
    if (m_nADBit >= 20)  //AD Resolution range 16 or 20
    {
        m_nADBit = 20;
    }
    else
    {
        m_nADBit = 16;
    }
    ////////////////////////////////////////////////////////
    m_nAppADBit = IniFile.GetInt(("ADM"), ("APPADBIT"), m_nADBit);
    if (m_nAppADBit > 20)  //AD Resolution range from 8 to 20
    {
        m_nAppADBit = 20;
    }
    else if (m_nAppADBit < 16)
    {
        m_nAppADBit = 16;
    }
    ////////////////////////////////////////////////////////
    //m_nALGValue:0--Org data; 1--Add 2000; 2--Add 2000 and average ; default 0--Org data
    m_nALGValue = IniFile.GetInt(("ADM"), ("ALG"), 0);
    //////////////////////////////////////////////////////////////////////
    m_fProgrammeGainAmp = IniFile.GetDouble(("ADM"), ("PGAMP"), 1.0);
    if(m_fProgrammeGainAmp <= 0.0)
    {
        m_fProgrammeGainAmp = 1.0;
    }
    /////////////////////////////////////////////////////////////////////////
    m_szADM_FPGAFilename = IniFile.GetString(("ADM"), ("BINFILE"), ("ddc232.bin"));
    ////////////////////////////////////////////////////////
    for (i=0; i< m_nMCBNum; i++)
    {
        sz_szInfo.Format(("MCB%dBINFILE"), i);
        m_szDBLine_FPGAFilename[i] = IniFile.GetString(("ADM"), sz_szInfo, m_szADM_FPGAFilename);
        //
        m_nDBLine_FPGAFileConfigFlag[i] = (1<<i);
    }
    //////////////////////////////////
    //配置时的配置参数
    for (i = 0; i < m_nMCBNum; i++)
    {
        for (j = (i+1); j < m_nMCBNum; j++)
        {
            if(0 == (m_szDBLine_FPGAFilename[i].Compare(m_szDBLine_FPGAFilename[j])))
            {
                m_nDBLine_FPGAFileConfigFlag[i] += m_nDBLine_FPGAFileConfigFlag[j];
                m_nDBLine_FPGAFileConfigFlag[j] = 0x00;
            }
        }
    }
    //////////////////////////////////
    //////////////////////////////////
    m_bAutoDMNum = (IniFile.GetInt(("MCB"), ("AUTODMNUM"), 0) ==1);
    //
    UINT nAADDB =0;
    nAADDB = (IniFile.GetInt(("ADM"), ("AADDB"), 1) ==0) ? 0:0x40;
    //
    UINT nTestMode =0;
    nTestMode = (IniFile.GetInt(("ADM"), ("TESTMODE"), 0) ==0) ? 0:0x80;
    //
    m_bAutoDMNum = (IniFile.GetInt(("MCB"), ("AUTODMNUM"), 0) ==1);
    //////////////////////////////////////////////////////////////
    m_nMergingDMNum = 0;
    //所有数据是否合并，触发一次回调
    m_bAllMCBDataMerging = (IniFile.GetInt(("CHADJ"), ("MCBDATAMERGING"), 0) == 1);
    //
    for (i = 0; i < MAX_MCBNUM; i++)
    {
        sz_szInfo.Format(("MCB%dADNUM"),i);
        m_DMInfo[i].nMCB =i;
        m_DMInfo[i].nDMNum = IniFile.GetInt(("MCB"), sz_szInfo, 8);
        //
        if (m_DMInfo[i].nDMNum > MAX_MCBADNUM)
        {
            m_DMInfo[i].nDMNum =MAX_MCBADNUM;
        }
        else if (m_DMInfo[i].nDMNum <1)
        {
            m_DMInfo[i].nDMNum =1;  // min ADM number =1
        }
        //////////////////////////////////////////
        if(m_nMCBNum > i)
        {
            m_nMergingDMNum += m_DMInfo[i].nDMNum;
        }
        //////////////////////////////////////////
        m_DMInfo[i].nRowNum = IniFile.GetInt(("ADM"), ("ROW"), 32);
        if (m_DMInfo[i].nRowNum > MAX_DMROWNUM)
        {
            m_DMInfo[i].nRowNum =MAX_DMROWNUM;
        }
        else if (m_DMInfo[i].nRowNum <8)
        {
            m_DMInfo[i].nRowNum =8;  //min Row =8
        }
        m_DMInfo[i].nColNum = IniFile.GetInt(("ADM"), ("COLUMN"), 4);
        if (m_DMInfo[i].nColNum > MAX_DMCOLNUM)
        {
            m_DMInfo[i].nColNum =MAX_DMCOLNUM;
        }
        else if (m_DMInfo[i].nColNum <1)
        {
            m_DMInfo[i].nColNum =1; //min Col =1
        }
        m_DMInfo[i].nAllChnNum = m_DMInfo[i].nDMNum*m_DMInfo[i].nRowNum*m_DMInfo[i].nColNum;
        //////////////////////////////////////////////////////////
        sz_szInfo.Format(("MCB%dDISCARDNUM"),i);
        m_nDiscardNum[i] = IniFile.GetInt(("CHADJ"), sz_szInfo, 0);
        m_nDiscardNum[i]  = m_nDiscardNum[i] > 0 ? m_nDiscardNum[i] : 0;
        m_nDiscardNum[i]  = m_nDiscardNum[i] < INVALID_DATA_PACKET_NUM ? m_nDiscardNum[i] : INVALID_DATA_PACKET_NUM;
        //////////////////////////////////////////////////////////
        for (j=0; j<MAX_MCBADNUM; j++)
        {
            m_nADMCMDREG[i][j] = 0x07;  //DDC default max range
            m_nADMCMDREG[i][j] += nAADDB;
            m_nADMCMDREG[i][j] += nTestMode;//探测器模块测试模式
            //
            if (m_nADBit == 20)  //set 16bit/20bit control bit
            {
                m_nADMCMDREG[i][j] |= 0x10;//置1
            }
            else
            {
                m_nADMCMDREG[i][j] &= 0xffffffef;//置0
            }
        }
        //
        for (j=0; j<MAX_MCBADNUM; j++)
        {
            for(k=0; k <(MAX_DMROWNUM*MAX_DMCOLNUM/16); k++)
            {
                m_fMCBxDMyPGz[i][j][k] = 1.0;//针对物品机应用，16个通道一组单独可设置浮点数增益
            }
        }
        //////////////////////////////////////////////////////////////////
        sz_szInfo.Format(("MCB%dDTM"), i);
        //指定MCBx是否进入诊断模式
        m_bMCBxDTM[i] = (IniFile.GetInt(("ADM"), sz_szInfo, 0) > 0);
    }
    //////////////////////////////////////////////////////////////////
    //指定MCBx是否进入诊断模式
    m_bMCBxDTM[MAX_MCBNUM] = (IniFile.GetInt(("ADM"), ("DTM"), 0) > 0);
    //如果配置了DTM，则所有控制盒模块都进入诊断模式
    if(m_bMCBxDTM[MAX_MCBNUM])
    {
        for (i = 0; i < MAX_MCBNUM; i++)
        {
            m_bMCBxDTM[i] = true;
        }
    }
    //日志记录
    bool bDTM = false;
    sz_szInfo = ("Configure HMCB Diagnostic Test Mode: ");
    for (i = 0; i < m_nMCBNum; i++)
    {
        if(m_bMCBxDTM[i])
        {
            bDTM = true;
            str.Format(("HMCB%d DTM=true; "), i);
        }
        else
        {
            str.Format(("HMCB%d DTM=FALSE; "), i);
        }
        sz_szInfo += str;
    }
    sz_szInfo += ("\n");
    if(bDTM)
    {
        std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    //////////////////////////////////////////////////////////////////////
    //上一次数据采集是否成功完成标志
    for (i=0; i<MAX_MCBNUM; i++)
    {
        m_bPreDAQSucessFlag[i] = false;
    }
    //////////////////////////////////////////////////////////////////////
    m_bColCombine = (m_DMInfo[0].nColNum >1)&&(IniFile.GetInt(("ADM"), ("COL_COMBINE"), 0) ==1);

    m_bAdjAinDi = (IniFile.GetInt(("ADM"), ("ADJ_AINDI"), 0) ==1);
    /////////////////////////////////////////////////////////////////////////////
    str.Format(("HMCB Number=%d, Enable HMCB-4=%d, Auto get DM number=%d, AD bit=%d, DM ROW*COLUMN=%d*%d, Column-Combine=%d, ADJ_AinDi=%d, APPADBIT=%d, ALG=%d, PGAMP=%.2f.\n"),
               m_nMCBNum, m_bEnMCBx4, m_bAutoDMNum, m_nADBit, m_DMInfo[0].nRowNum, m_DMInfo[0].nColNum, m_bColCombine, m_bAdjAinDi, m_nAppADBit, m_nALGValue, m_fProgrammeGainAmp);
    std::cout <<(str);
    WriteLogFile(str);
    /////////////////////////////////////////////////////////////////////////////
    if(m_bEnMCBx4)
    {
        for (i = 0; i < m_nMCBx4DBLine; i++)
        {
            str.Format(("HMCB4-DB%d DM number=%d.\n"), i, m_DMInfo[i].nDMNum);
            std::cout <<(str);
            WriteLogFile(str);
        }
    }
    else
    {
        for (i = 0; i < m_nMCBNum; i++)
        {
            str.Format(("HMCB No.%d DM number=%d.\n"), i, m_DMInfo[i].nDMNum);
            std::cout <<(str);
            WriteLogFile(str);
        }
    }
    /////////////////////////////////////////////////////////////////////////////
    if(m_bAllMCBDataMerging)
    {
        std::cout <<(("Set up HMCB4 all dbline data merges.\n"));
        WriteLogFile(("Set up HMCB4 all dbline data merges.\n"));
    }
    //////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////
	bool bAdj = true;
	CString strtmp = ("");
	memset(m_nAdjChn, 0, sizeof(m_nAdjChn));  //	int m_nAdjChn[MAX_CHNADJNUM];  //adjust mode: 0-NONE, 1-ROW, 2-COLUMN, 3-MODULE, 4-ALL_ROW
	for (i = 0; i < m_nMCBNum; i++)
	{
		strtmp.Format(("STEP%d"),i+1);
		str =IniFile.GetString(("CHADJ"), strtmp, ("NONE"));
		//
		bAdj = true;
		if (str ==("ROW"))
		{
			m_nAdjChn[i] =1;
		}
		else if (str ==("COLUMN"))
		{
			m_nAdjChn[i] =2;
		}
		else if (str ==("MODULE"))
		{
			m_nAdjChn[i] =3;
		}
		else if (str ==("ALL_ROW"))
		{
			m_nAdjChn[i] =4;
		}
		else
		{
			bAdj = false;
			break;
		}
		if(bAdj)
		{
			std::cout <<(("Channel order adjust, ") +strtmp +("= ") + str+(".\n"));
			WriteLogFile(("Channel order adjust, ") +strtmp +("= ") + str+(".\n"));
		}
	}
    //////////////////////////////////////////////////////////////////////
    memset(m_nMCBxAdjChn, 0, sizeof(m_nMCBxAdjChn));  //	int m_nMCBxAdjChn[MAX_MCBNUM][MAX_CHNADJNUM];  //adjust mode: 0-NONE, 1-ROW, 2-COLUMN, 3-MODULE, 4-ALL_ROW
    for(j = 0; j < m_nMCBNum; j++)
    {
        for (i=0; i<MAX_CHNADJNUM; i++)
        {
			bAdj = true;
            strtmp.Format(("MCB%dSTEP%d"),j, i+1);
            str =IniFile.GetString(("CHADJ"), strtmp, ("NONE"));
            if (str ==("ROW"))
            {
                m_nMCBxAdjChn[j][i] =1;
            }
            else if (str ==("COLUMN"))
            {
                m_nMCBxAdjChn[j][i] =2;
            }
            else if (str ==("MODULE"))
            {
                m_nMCBxAdjChn[j][i] =3;
            }
            else if (str ==("ALL_ROW"))
            {
                m_nMCBxAdjChn[j][i] =4;
            }
            else
            {
				bAdj = false;
                break;
            }
			if(bAdj)
			{
				std::cout <<(("Channel order adjust, ") +strtmp +("= ") + str+(".\n"));
				WriteLogFile(("Channel order adjust, ") +strtmp +("= ") + str+(".\n"));
			}
        }
    }
    //////////////////////////////////////////////////////////////////////
    memset(m_nSectionFirstADNo, 0, sizeof(m_nSectionFirstADNo));
    memset(m_nSectionLastADNo,  0, sizeof(m_nSectionLastADNo));
    for (i=0; i<MAX_SECTIONADJNUM; i++)
    {
        CString strtmp1 = ("");
        CString strtmp2 = ("");
        int xFirstADNo = 0;
        int xLastADNo = 0;
        //
        strtmp1.Format(("SECTION%dFIRSTADNO"),i+1);
        strtmp2.Format(("SECTION%dLASTADNO"),i+1);
        //
        xFirstADNo = IniFile.GetInt(("SECTIONADJ"), strtmp1, 0);
        xLastADNo = IniFile.GetInt(("SECTIONADJ"), strtmp2, 0);
        //
        m_nSectionFirstADNo[i] = xFirstADNo;
        m_nSectionLastADNo[i] = xLastADNo;
        //
        if((xFirstADNo > 0) && (xLastADNo > 0))
        {
            str.Format(("Detector Section order adjust, SECTION%dFIRSTADNO = %d,  SECTION%dLASTADNO = %d \n"), i+1, xFirstADNo, i+1, xLastADNo);
            std::cout <<(str);
            WriteLogFile(str);
        }
    }
    //////////////////////////////////////////////////////////////////////
    memset(m_nMCBxSectionFirstADNo, 0, sizeof(m_nMCBxSectionFirstADNo));
    memset(m_nMCBxSectionLastADNo,  0, sizeof(m_nMCBxSectionLastADNo));
    for(j = 0; j < MAX_MCBNUM; j++)
    {
        for (i=0; i<MAX_SECTIONADJNUM; i++)
        {
            CString strtmp1 = ("");
            CString strtmp2 = ("");
            int xFirstADNo = 0;
            int xLastADNo = 0;
            //
            strtmp1.Format(("MCB%dSECTION%dFIRSTADNO"), j, i+1);
            strtmp2.Format(("MCB%dSECTION%dLASTADNO"), j, i+1);
            //
            xFirstADNo = IniFile.GetInt(("SECTIONADJ"), strtmp1, 0);
            xLastADNo = IniFile.GetInt(("SECTIONADJ"), strtmp2, 0);
            //
            m_nMCBxSectionFirstADNo[j][i] = xFirstADNo;
            m_nMCBxSectionFirstADNo[j][i] = xLastADNo;
            //
            if((xFirstADNo > 0) && (xLastADNo > 0))
            {
                str.Format(("MCB%d Detector Section order adjust, SECTION%dFIRSTADNO = %d,  SECTION%dLASTADNO = %d \n"), j, i+1, xFirstADNo, i+1, xLastADNo);
                std::cout <<(str);
                WriteLogFile(str);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    sz_szInfo.Format(("HMCB Discard number: "));
    for (i=0; i< m_nMCBNum; i++)
    {
        str.Format(("MCB%dDISCARDNUM = %d, "), i, m_nDiscardNum[i]);
        sz_szInfo += str;
    }
    sz_szInfo +=("\n");
    std::cout <<(sz_szInfo);
    WriteLogFile(sz_szInfo);
    //////////////////////////////////////////////////////////////////////
    m_nTrig =0; // default outer trig
    ////////////////////////////////////////////////////////////////
    //获取CB工作时序逻辑参数
    GetModeParam(("MODE0"));
    //////////////////////////////////////////////////////////////////////
    //
    GetRangeParam(("RANGE0"));
    //////////////////////////////////////////////////////////////////////
    //
    GetMCBxDMyPGz(("ADM"));
    //////////////////////////////////////////////////////////////////////
    //str.Format(("HMCB Registers:CTRLA=0x%02x, POTMT=%d, POTMW=%d, INTT=%d, DataRDYT =%d, Trigger=%s.\n"),
    //	m_reg.regCtrlA, m_reg.regPOTMT, m_reg.regPOTMW, m_reg.regINTT, m_reg.regDataRdyT, m_nTrig==0?("0-inner"):("1-outer"));
    str.Format(("Master HMCB some Registers initial value: POTMT=%dus, POTMW=%dus, INTT=%dus, DataRDYT=%dus, DDI-LatchT=%dus, Trigger=%s.\n"),
               m_reg.regPOTMT, m_reg.regPOTMW, m_reg.regINTT, m_reg.regDataRdyT, m_reg.regDDILatchT, m_nTrig==0?("0-inner"):("1-outer"));
    std::cout <<(str);
    WriteLogFile(str);
    //////////////////////////////////////////////////////////////////////
    m_bSaveExdata = (IniFile.GetInt(("EXDATA"), ("SAVEDATA"), 0) ==1);
    if (m_bSaveExdata)
    {
        std::cout <<(("Set as save exdata mode.\n"));
        WriteLogFile(("Set as save exdata mode.\n"));
    }
    ///////////////////////////////////////////////////////////////
    //备用
    //是否为调试版本
    m_bDebug = (IniFile.GetInt(("ADM"), ("DEBUG"), 0) ==1);
    //////////////////////////////////////////////////////////////
    if(m_bDebug)
    {
        cmd_pkg_parse_log_file = cmd_pkg_parse_log_file_create();
    }
    else
    {
        cmd_pkg_parse_log_file = NULL;
    }
#else
    int i = 0;
    int j = 0;
    int k = 0;
    std::string str = ("");
    std::string sz_szInfo = ("");
    //////////////////////////////////////////////////////////////////////////////////////////
    char tmp[512] = {0};
    memset(tmp,0,512);
    sprintf(tmp,"Read configure parameters from %s.\n",m_strMCBCtrlIniFile.c_str());
    sz_szInfo = tmp;
    //std::cout <<(sz_szInfo);
    WriteLogFile(sz_szInfo);
    ///////////////////////////////////////////////////////
    // read ini file
    CIniFile IniFile;
    IniFile.SetFileName(m_strMCBCtrlIniFile);
    ///////////////////////////////////////////////////////
    m_nMCBNum = (unsigned short) IniFile.GetInt(("MCB"), ("MCBNUM"), 1);
    if (m_nMCBNum > MAX_MCBNUM)
    {
        m_nMCBNum = MAX_MCBNUM;
    }
    else if (m_nMCBNum <1)
    {
        m_nMCBNum =1;
    }
    ///////////////////////////////////////////////////////
    //是否使能使用DAQ Tirgger控制采集启停
    //m_bEnCtrlDAQTrig = false;//初始值


    ///////////////////////////////////////////////////////
    m_szMCBIPAddr = IniFile.GetString(("CONNECTION"), ("IPADDR"), ("192.111.111.211"));

    m_bconnectauto = (IniFile.GetInt(("CONNECTION"), ("AUTOCONNECT"), 1) ==1);
    //
    memset(tmp,0,512);
    sprintf(tmp,"HMCB IP Address:%s, CTRL port:%d, Data port:%d, autoconnect=%d.\n",IniFile.GetString(("CONNECTION"), ("IPADDR"), ("192.111.111.211")).c_str(), ETH_CTRL_PORT, ETH_DATA_PORT, m_bconnectauto);
    str = tmp;
    //std::cout <<(str);
    WriteLogFile(str);
    ////////////////////////////////////////////////////////
    m_bFcto16ADBit = false;
    //
    m_nADBit = IniFile.GetInt(("ADM"), ("ADBIT"), 20);
    if (m_nADBit >= 20)  //AD Resolution range 16 or 20
    {
        m_nADBit = 20;
    }
    else
    {
        m_nADBit = 16;
    }
    ////////////////////////////////////////////////////////
    m_nAppADBit = IniFile.GetInt(("ADM"), ("APPADBIT"), m_nADBit);
    if (m_nAppADBit > 20)  //AD Resolution range from 8 to 20
    {
        m_nAppADBit = 20;
    }
    else if (m_nAppADBit < 16)
    {
        m_nAppADBit = 16;
    }
    ////////////////////////////////////////////////////////
    //m_nALGValue:0--Org data; 1--Add 2000; 2--Add 2000 and average ; default 0--Org data
    m_nALGValue = IniFile.GetInt(("ADM"), ("ALG"), 0);
    //////////////////////////////////////////////////////////////////////
    m_fProgrammeGainAmp = IniFile.GetDouble(("ADM"), ("PGAMP"), 1.0);
    if(m_fProgrammeGainAmp <= 0.0)
    {
        m_fProgrammeGainAmp = 1.0;
    }
    /////////////////////////////////////////////////////////////////////////
    m_szADM_FPGAFilename = IniFile.GetString(("ADM"), ("BINFILE"), ("ddc232.bin"));
    ////////////////////////////////////////////////////////
    for (i=0; i< MAX_MCBNUM; i++)
    {
        for(j = 0; j < MAX_DBLINENUM; j++)
        {
            m_szMCBDBLine_FPGAFilename[i][j] = ("");
            m_nDBLine_FPGAFileConfigFlag[i][j] = 0x00;
            m_DMInfo[i].nDBDMNUM[j] = 0;
        }
        memset(&m_DMInfo[i], 0x00, sizeof(m_DMInfo[i]));
    }
    //////////////////////////////////
    for (i = 0; i < m_nMCBNum; i++)
    {
        //每个控制板的探测器FPGA配置bin文件
        memset(tmp,0,512);
        sprintf(tmp,"MCB%dBINFILE", i);
        sz_szInfo = tmp;
        m_szMCB_FPGAFilename[i] = IniFile.GetString(("ADM"), sz_szInfo, m_szADM_FPGAFilename);
        //
        for(j = 0; j < MAX_DBLINENUM; j++)
        {
            //每个控制板的每个DBline的探测器FPGA配置bin文件
            memset(tmp,0,512);
            sprintf(tmp,"MCB%dDB%dBINFILE", i, j);
            sz_szInfo = tmp;
            m_szMCBDBLine_FPGAFilename[i][j] = IniFile.GetString(("ADM"), sz_szInfo, m_szMCB_FPGAFilename[i]);
            //
            m_nDBLine_FPGAFileConfigFlag[i][j] = 0x01<<j;
        }
    }
    //////////////////////////////////
    //配置时的配置参数
    for (i = 0; i < m_nMCBNum; i++)
    {
        for(j = 0; j < MAX_DBLINENUM; j++)
        {
            for (k = (j+1); k < MAX_DBLINENUM; k++)
            {
                if(0 == (m_szMCBDBLine_FPGAFilename[i][j].compare(m_szMCBDBLine_FPGAFilename[i][k])))
                {
                    m_nDBLine_FPGAFileConfigFlag[i][j] += m_nDBLine_FPGAFileConfigFlag[i][k];
                    m_nDBLine_FPGAFileConfigFlag[i][k] = 0x00;
                }
            }
        }
    }
    //////////////////////////////////
    //////////////////////////////////
    m_bAutoDMNum = (IniFile.GetInt(("MCB"), ("AUTODMNUM"), 0) ==1);
    //
    UINT nAADDB =0;
    nAADDB = (IniFile.GetInt(("ADM"), ("AADDB"), 1) ==0) ? 0:0x40;
    //
    UINT nTestMode =0;
    nTestMode = (IniFile.GetInt(("ADM"), ("TESTMODE"), 0) ==0) ? 0:0x80;
    //
    m_bAutoDMNum = (IniFile.GetInt(("MCB"), ("AUTODMNUM"), 0) ==1);
    //////////////////////////////////////////////////////////////
    //m_nMergingDMNum = 0;
    //所有数据是否合并，触发一次回调
    //m_bAllMCBDataMerging = (IniFile.GetInt(("CHADJ"), ("MCBDATAMERGING"), 0) == 1);
    //////////////////////////////////////////////////////////////
    //m_bEnMCBx4 = (IniFile.GetInt(("MCB"), ("MCBENDB4"), 0) ==1);
	//2026.02.01 把MCBENDB4默认值改为1
	m_bEnMCBx4 = (IniFile.GetInt(("MCB"), ("MCBENDB4"), 1) ==1);
    //
    for (i = 0; i < MAX_MCBNUM; i++)
    {
        memset(tmp,0,512);
        sprintf(tmp,"MCB%dADNUM",i);
        sz_szInfo = tmp;
        m_DMInfo[i].nMCB =i;
        m_DMInfo[i].nDMNum = IniFile.GetInt(("MCB"), sz_szInfo, 0);
        //
        for (j = 0; j < MAX_DBLINENUM; j++)
        {
            memset(tmp,0,512);
            sprintf(tmp,"MCB%dDB%dADNUM",i, j);
            sz_szInfo = tmp;
            m_DMInfo[i].nDBDMNUM[j] = IniFile.GetInt(("MCB"), sz_szInfo, (0 == j) ? m_DMInfo[i].nDMNum : 0);//默认DBLine0的数量为整个控制板的模块数量
            //
            if(m_DMInfo[i].nDBDMNUM[j] > 0)
            {
                m_DMInfo[i].nDBLineEnNum += 1;
            }
        }


        if (m_DMInfo[i].nDMNum > MAX_MCBADNUM)
        {
            m_DMInfo[i].nDMNum =MAX_MCBADNUM;
        }
        else if (m_DMInfo[i].nDMNum <1)
        {
            m_DMInfo[i].nDMNum =1;  // min ADM number =1
        }
        //////////////////////////////////////////
        //        if(m_nMCBNum > i)
        //        {
        //            m_nMergingDMNum += m_DMInfo[i].nDMNum;
        //        }
        //////////////////////////////////////////
        m_DMInfo[i].nRowNum = IniFile.GetInt(("ADM"), ("ROW"), 32);
        if (m_DMInfo[i].nRowNum > MAX_DMROWNUM)
        {
            m_DMInfo[i].nRowNum =MAX_DMROWNUM;
        }
        else if (m_DMInfo[i].nRowNum <8)
        {
            m_DMInfo[i].nRowNum =8;  //min Row =8
        }
        m_DMInfo[i].nColNum = IniFile.GetInt(("ADM"), ("COLUMN"), 4);
        if (m_DMInfo[i].nColNum > MAX_DMCOLNUM)
        {
            m_DMInfo[i].nColNum =MAX_DMCOLNUM;
        }
        else if (m_DMInfo[i].nColNum <1)
        {
            m_DMInfo[i].nColNum =1; //min Col =1
        }
        m_DMInfo[i].nAllChnNum = m_DMInfo[i].nDMNum*m_DMInfo[i].nRowNum*m_DMInfo[i].nColNum;
        //////////////////////////////////////////////////////////
        memset(tmp,0,512);
        sprintf(tmp,"MCB%dDISCARDNUM",i);
        sz_szInfo = tmp;
        m_nDiscardNum[i] = IniFile.GetInt(("CHADJ"), sz_szInfo, 0);
        m_nDiscardNum[i]  = m_nDiscardNum[i] > 0 ? m_nDiscardNum[i] : 0;
        m_nDiscardNum[i]  = m_nDiscardNum[i] < INVALID_DATA_PACKET_NUM ? m_nDiscardNum[i] : INVALID_DATA_PACKET_NUM;
        //////////////////////////////////////////////////////////
        for (j=0; j<MAX_MCBADNUM; j++)
        {
            m_nADMCMDREG[i][j] = 0x07;  //DDC default max range
            m_nADMCMDREG[i][j] += nAADDB;
            m_nADMCMDREG[i][j] += nTestMode;//探测器模块测试模式
            //
            if (m_nADBit == 20)  //set 16bit/20bit control bit
            {
                m_nADMCMDREG[i][j] |= 0x10;//置1
            }
            else
            {
                m_nADMCMDREG[i][j] &= 0xffffffef;//置0
            }
        }
        /////////////////////////////////////////////////
        //探测器板工作寄存器配置参数(支持多块多端口级联)
        for (j = 0; j < MAX_DBLINENUM; j++)
        {
            for(k = 0; k < MAX_MCBADNUM; k++)
            {
                m_nMCBxDByREG[i][j][k] = 0x07;  //DDC default max range
                m_nMCBxDByREG[i][j][k] += nAADDB;
                m_nMCBxDByREG[i][j][k] += nTestMode;//探测器模块测试模式
                //
                if (m_nADBit == 20)  //set 16bit/20bit control bit
                {
                    m_nMCBxDByREG[i][j][k] |= 0x10;//置1
                }
                else
                {
                    m_nMCBxDByREG[i][j][k] &= 0xffffffef;//置0
                }
            }
        }
        /////////////////////////////////////////////////
        for (j=0; j<MAX_MCBADNUM; j++)
        {
            for(k=0; k <(MAX_DMROWNUM*MAX_DMCOLNUM/16); k++)
            {
                m_fMCBxDMyPGz[i][j][k] = 1.0;//针对物品机应用，16个通道一组单独可设置浮点数增益
            }
        }
        //////////////////////////////////////////////////////////////////
        memset(tmp,0,512);
        sprintf(tmp,"MCB%dDTM", i);
        sz_szInfo = tmp;
        //指定MCBx是否进入诊断模式
        m_bMCBxDTM[i] = (IniFile.GetInt(("ADM"), sz_szInfo, 0) > 0);
    }
    //////////////////////////////////////////////////////////////////
    //指定MCBx是否进入诊断模式
    m_bMCBxDTM[MAX_MCBNUM] = (IniFile.GetInt(("ADM"), ("DTM"), 0) > 0);
    //如果配置了DTM，则所有控制盒模块都进入诊断模式
    if(m_bMCBxDTM[MAX_MCBNUM])
    {
        for (i = 0; i < MAX_MCBNUM; i++)
        {
            m_bMCBxDTM[i] = true;
        }
    }
    //日志记录
    bool bDTM = false;
    sz_szInfo = ("Configure HMCB Diagnostic Test Mode: ");
    for (i = 0; i < m_nMCBNum; i++)
    {
        if(m_bMCBxDTM[i])
        {
            bDTM = true;
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d DTM=true; ", i);
            str = tmp;
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d DTM=FALSE; ", i);
            str = tmp;
        }
        sz_szInfo += str;
    }
    sz_szInfo += ("\n");
    if(bDTM)
    {
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    //////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	//硬件序列号取值模式
	//默认为1，取实际返回包裹数序列号做硬件序列号
	//改为0，取软件包序列号
	//（临时解决存在硬件序列号有重复的现象（换控制盒仍旧存在），导致多个盒级联时存在数据对不齐的问题）（软件端按回调数据里面的硬件触发数存图）
	m_bHWSerialNumberMode = (IniFile.GetInt(("ADM"), ("HWSN"), 1) ==1);
    //数据解析按智能（=0）还是高能（=1）模式解析处理
    //默认为高能模式
    m_bHEorCXMode = (IniFile.GetInt(("ADM"), ("CXMODE"), 1) ==1);
	//针对HMCB/S2-4 把高低能指示信号借用为钩铛信号
    m_bGoudang_S4 = (IniFile.GetInt(("ADM"), ("GOUDANG_S4"), 0) ==1);
	//三能加速器使能(三能加速器（DI(3)为高，则为0.7MeV，能量指示值为2或3，否则为高低双能，能量指示值为0或1）)
	m_bThreeEnergyAcc_En = (IniFile.GetInt(("ADM"), ("THREEENERGYACC"), 0) ==1);
    //////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////
	//2026-03-19增加对CS0200M的支持
	//增加 [CHADJ] DEVICETYPEID 配置项
	//默认值为0，常规设备
	//1——CS0200M， 调整主控制盒的m_MCBData[nIndex].d_head.head_use.accEnergy为出束光机的编号( m_MCBData[nIndex].d_head.head_use.accEnergy = (long)l_angle);
	//出束光机的编号规则：光机触发信号接DI-0，光机循环信号接DI-1，DI-1为1时的光机编号为0，DI-1为0后每一次触发的光机编号+1
	str =IniFile.GetString(("CHADJ"), ("DEVICETYPE"), (""));
	if (str ==("CS0200M"))// 0-normal, 1-CS0200M
	{
		m_nDeviceTypeID = 1;
	}
	else
	{
		m_nDeviceTypeID =0;
	}
	//射线源序号
	m_nXRayGenN = 0;
	//
	if(0 == m_nDeviceTypeID)
	{
		//str.Format(_T("DEVICETYPE = %s, DEVICETYPEID = %d, this is a standard device.\n"), strDVType, m_nDeviceTypeID);
		//WriteLogFile(str);
        memset(tmp,0,512);
        sprintf(tmp,"DEVICETYPE = %s, DEVICETYPEID = %d, this is a standard device.\n", str, m_nDeviceTypeID);
        sz_szInfo = tmp;
        WriteLogFile(sz_szInfo);
	}
	else if(1 == m_nDeviceTypeID)
	{
		//str.Format(_T("DEVICETYPE = %s, DEVICETYPEID = %d, this is a CS0200M device.\n"), strDVType, m_nDeviceTypeID);
		//WriteLogFile(str);
        memset(tmp,0,512);
        sprintf(tmp,"DEVICETYPE = %s, DEVICETYPEID = %d, this is a CS0200M device.\n", str, m_nDeviceTypeID);
        sz_szInfo = tmp;
        WriteLogFile(sz_szInfo);
	}
	else
	{
		//str.Format(_T("DEVICETYPEID = %d, this is an unknown device device.\n"));
		//WriteLogFile(str);
        memset(tmp,0,512);
        sprintf(tmp,"DEVICETYPE = %s, DEVICETYPEID = %d, this is an unknown device device.\n", str, m_nDeviceTypeID);
        sz_szInfo = tmp;
        WriteLogFile(sz_szInfo);
	}
    //////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////
    //是否要求获取开始的0xA5数据包,默认为不需要获取
    //int nT = IniFile.GetInt(("ADM"), ("NEEDA5"), 0);
    //m_bNeedGet0xA5 = (nT == 1);
	m_bNeedGet0xA5 = (IniFile.GetInt(("ADM"), ("NEEDA5"), 1) ==1);
	if(m_bNeedGet0xA5)
    {
        str="HMCB need 0xA5 data packet first when start DAQ.\n";
    }
    else
    {
        str="HMCB don't need 0xA5 data packet first when start DAQ.\n";
    }
    WriteLogFile(str);
    /////////////////////////////////////////////////////////////////////////////
    //上一次数据采集是否成功完成标志
    for (i=0; i<MAX_MCBNUM; i++)
    {
        m_bPreDAQSucessFlag[i] = false;
    }
    //////////////////////////////////////////////////////////////////////
    m_bColCombine = (m_DMInfo[0].nColNum >1)&&(IniFile.GetInt(("ADM"), ("COL_COMBINE"), 0) ==1);

    m_bAdjAinDi = (IniFile.GetInt(("ADM"), ("ADJ_AINDI"), 0) ==1);
    /////////////////////////////////////////////////////////////////////////////
    memset(tmp,0,512);
    sprintf(tmp,"HMCB Number=%d, Enable HMCB-4=%d, Auto get DM number=%d, AD bit=%d, DM ROW*COLUMN=%d*%d, Column-Combine=%d, ADJ_AinDi=%d, APPADBIT=%d, ALG=%d, PGAMP=%.2f.\n",
            m_nMCBNum, m_bEnMCBx4, m_bAutoDMNum, m_nADBit, m_DMInfo[0].nRowNum, m_DMInfo[0].nColNum, m_bColCombine, m_bAdjAinDi, m_nAppADBit, m_nALGValue, m_fProgrammeGainAmp);
    str = tmp;
    //std::cout <<(str);
    WriteLogFile(str);
    /////////////////////////////////////////////////////////////////////////////
    str = m_bHEorCXMode == 0 ? "CX Mode" : "HE Mode" ;

    if(m_bEnMCBx4)
    {
        memset(tmp,0,512);
        sprintf(tmp,"HMCB-4 set as enable and CXMode=%d(%s).\n", m_bHEorCXMode, str.c_str());
        sz_szInfo = tmp;
        WriteLogFile(sz_szInfo);
        for (i = 0; i < m_nMCBNum; i++)
        {
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d: DM number=%d, of which DB0 DM number=%d,  DB1 DM number=%d, DB2 DM number=%d, DB3 DM number=%d.\n", i, m_DMInfo[i].nDMNum,  m_DMInfo[i].nDBDMNUM[0],  m_DMInfo[i].nDBDMNUM[1],  m_DMInfo[i].nDBDMNUM[2],  m_DMInfo[i].nDBDMNUM[3]);
            str = tmp;
            //std::cout <<(str);
            WriteLogFile(str);
        }
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"HMCB-4 set as disable and CXMode=%d(%s).\n", m_bHEorCXMode, str.c_str());
        sz_szInfo = tmp;
        WriteLogFile(sz_szInfo);
        for (i = 0; i < m_nMCBNum; i++)
        {
            memset(tmp,0,512);
            sprintf(tmp,"HMCB No.%d DM number=%d.\n", i, m_DMInfo[i].nDMNum);
            str = tmp;
            //std::cout <<(str);
            WriteLogFile(str);
        }
    }
    /////////////////////////////////////////////////////////////////////////////
//    if(m_bAllMCBDataMerging)
//    {
//        //std::cout <<(("Set up HMCB4 all dbline data merges.\n"));
//        WriteLogFile(("Set up HMCB4 all dbline data merges.\n"));
//    }
    //////////////////////////////////////////////////////////////////////
    m_bZeroPadding = (IniFile.GetInt(("CHADJ"), ("ZEROPADDING"), 1) == 1);//�Զ�ʧ�Ĵ��������Ƿ��㣨Ĭ�ϲ��㣩
    memset(tmp,0,512);
    sprintf(tmp,"HMCB set [CHADJ] ZEROPADDING = %d.\n", m_bZeroPadding);
    sz_szInfo=tmp;
    WriteLogFile(sz_szInfo);
    //////////////////////////////////////////////////////////////
 	bool bAdj = true;
    std::string  strtmp = ("");
    memset(m_nAdjChn, 0, sizeof(m_nAdjChn));  //	int m_nAdjChn[MAX_CHNADJNUM];  //adjust mode: 0-NONE, 1-ROW, 2-COLUMN, 3-MODULE, 4-ALL_ROW
    for (i = 0; i < m_nMCBNum; i++)
    {
		bAdj = true;
        memset(tmp,0,512);
        sprintf(tmp,"STEP%d",i+1);
        strtmp = tmp;
        str =IniFile.GetString(("CHADJ"), strtmp, ("NONE"));
        if (str ==("ROW"))
        {
            m_nAdjChn[i] =1;
        }
        else if (str ==("COLUMN"))
        {
            m_nAdjChn[i] =2;
        }
        else if (str ==("MODULE"))
        {
            m_nAdjChn[i] =3;
        }
        else if (str ==("ALL_ROW"))
        {
            m_nAdjChn[i] =4;
        }
        else
        {
			bAdj = false;
            break;
        }
		if(bAdj)
		{
			//std::cout <<(("Channel order adjust, ") +strtmp +("= ") + str+(".\n"));
			WriteLogFile(("Channel order adjust, ") +strtmp +("= ") + str+(".\n"));
		}
    }
    //////////////////////////////////////////////////////////////////////
    memset(m_nMCBxAdjChn, 0, sizeof(m_nMCBxAdjChn));  //	int m_nMCBxAdjChn[MAX_MCBNUM][MAX_CHNADJNUM];  //adjust mode: 0-NONE, 1-ROW, 2-COLUMN, 3-MODULE, 4-ALL_ROW
    for(j = 0; j < m_nMCBNum; j++)
    {
        for (i=0; i<MAX_CHNADJNUM; i++)
        {
			bAdj = true;
            memset(tmp,0,512);
            sprintf(tmp,"MCB%dSTEP%d",j, i+1);
            strtmp = tmp;
            str =IniFile.GetString(("CHADJ"), strtmp, ("NONE"));
            if (str ==("ROW"))
            {
                m_nMCBxAdjChn[j][i] =1;
            }
            else if (str ==("COLUMN"))
            {
                m_nMCBxAdjChn[j][i] =2;
            }
            else if (str ==("MODULE"))
            {
                m_nMCBxAdjChn[j][i] =3;
            }
            else if (str ==("ALL_ROW"))
            {
                m_nMCBxAdjChn[j][i] =4;
            }
            else
            {
				bAdj = false;
                break;
            }
			if(bAdj)
			{
				//std::cout <<(("Channel order adjust, ") +strtmp +("= ") + str+(".\n"));
				WriteLogFile(("Channel order adjust, ") +strtmp +("= ") + str+(".\n"));
			}
        }
    }
    //////////////////////////////////////////////////////////////////////
    memset(m_nMCBxDByAdjChn, 0, sizeof(m_nMCBxDByAdjChn));  //	int m_nMCBxAdjChn[MAX_MCBNUM][MAX_CHNADJNUM];  //adjust mode: 0-NONE, 1-ROW, 2-COLUMN, 3-MODULE, 4-ALL_ROW
    for(j = 0; j < m_nMCBNum; j++)
    {
        for (i = 0; i< MAX_DBLINENUM; i++)
        {
            for(k = 0; k < MAX_CHNADJNUM; k++)
            {
				bAdj = true;
				//
                memset(tmp,0,512);
                sprintf(tmp,"MCB%dDB%dSTEP%d",j, i, k+1);
                strtmp = tmp;
                str =IniFile.GetString("CHADJ", strtmp, "NONE");
                if (str == ("ROW"))
                {
                    m_nMCBxDByAdjChn[j][i][k] =1;
                }
                else if (str ==("COLUMN"))
                {
                    m_nMCBxDByAdjChn[j][i][k] =2;
                }
                else if (str ==("MODULE"))
                {
                    m_nMCBxDByAdjChn[j][i][k] =3;
                }
                else if (str ==("ALL_ROW"))
                {
                    m_nMCBxDByAdjChn[j][i][k] =4;
                }
                else
                {
					bAdj = false;
                    break;
                }
				if(bAdj)
				{
					memset(tmp,0,512);
					sprintf(tmp,"HMCB%d-DB%d channel order adjust, %s= %s.\n", j, i, strtmp, str);
					sz_szInfo = tmp;
					WriteLogFile(sz_szInfo);
				}
            }
        }
    }
    //////////////////////////////////////////////////////////////////////
    memset(m_nSectionFirstADNo, 0, sizeof(m_nSectionFirstADNo));
    memset(m_nSectionLastADNo,  0, sizeof(m_nSectionLastADNo));
    for (i=0; i<MAX_SECTIONADJNUM; i++)
    {
        std::string strtmp1 = ("");
        std::string strtmp2 = ("");
        int xFirstADNo = 0;
        int xLastADNo = 0;
        //
        memset(tmp,0,512);
        sprintf(tmp,"SECTION%dFIRSTADNO",i+1);
        strtmp1 = tmp;

        memset(tmp,0,512);
        sprintf(tmp,"SECTION%dLASTADNO",i+1);
        strtmp2 = tmp;
        //
        xFirstADNo = IniFile.GetInt(("SECTIONADJ"), strtmp1, 0);
        xLastADNo = IniFile.GetInt(("SECTIONADJ"), strtmp2, 0);
        //
        m_nSectionFirstADNo[i] = xFirstADNo;
        m_nSectionLastADNo[i] = xLastADNo;
        //
        if((xFirstADNo > 0) && (xLastADNo > 0))
        {
            memset(tmp,0,512);
            sprintf(tmp,"Detector Section order adjust, SECTION%dFIRSTADNO = %d,  SECTION%dLASTADNO = %d \n", i+1, xFirstADNo, i+1, xLastADNo);
            str = tmp;
            //std::cout <<(str);
            WriteLogFile(str);
        }
    }
    //////////////////////////////////////////////////////////////////////
    memset(m_nMCBxSectionFirstADNo, 0, sizeof(m_nMCBxSectionFirstADNo));
    memset(m_nMCBxSectionLastADNo,  0, sizeof(m_nMCBxSectionLastADNo));
    for(j = 0; j < MAX_MCBNUM; j++)
    {
        for (i=0; i<MAX_SECTIONADJNUM; i++)
        {
            std::string strtmp1 = ("");
            std::string strtmp2 = ("");
            int xFirstADNo = 0;
            int xLastADNo = 0;
            //

            memset(tmp,0,512);
            sprintf(tmp,"MCB%dSECTION%dFIRSTADNO", j, i+1);
            strtmp1 = tmp;
            memset(tmp,0,512);
            sprintf(tmp,"MCB%dSECTION%dLASTADNO", j, i+1);
            strtmp2 = tmp;
            //
            xFirstADNo = IniFile.GetInt(("SECTIONADJ"), strtmp1, 0);
            xLastADNo = IniFile.GetInt(("SECTIONADJ"), strtmp2, 0);
            //
            m_nMCBxSectionFirstADNo[j][i] = xFirstADNo;
            m_nMCBxSectionFirstADNo[j][i] = xLastADNo;
            //
            if((xFirstADNo > 0) && (xLastADNo > 0))
            {
                memset(tmp,0,512);
                sprintf(tmp,"MCB%d Detector Section order adjust, SECTION%dFIRSTADNO = %d,  SECTION%dLASTADNO = %d \n", j, i+1, xFirstADNo, i+1, xLastADNo);
                str = tmp;
                //std::cout <<(str);
                WriteLogFile(str);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////

    sz_szInfo= "HMCB Discard number: ";
    for (i=0; i< m_nMCBNum; i++)
    {
        memset(tmp,0,512);
        sprintf(tmp,"MCB%dDISCARDNUM = %d, ", i, m_nDiscardNum[i]);
        str = tmp;
        sz_szInfo += str;
    }
    sz_szInfo +=("\n");
    //std::cout <<(sz_szInfo);
    WriteLogFile(sz_szInfo);
    //////////////////////////////////////////////////////////////////////
    m_nTrig =0; // default outer trig
    ////////////////////////////////////////////////////////////////
    //获取CB工作时序逻辑参数
    GetModeParam(("MODE0"));
    //////////////////////////////////////////////////////////////////////
    //获取增益设置参数
    GetRangeParam(("RANGE0"));
    //////////////////////////////////////////////////////////////////////
    //
    GetMCBxDMyPGz(("ADM"));
    //////////////////////////////////////////////////////////////////////
    //str.Format(("HMCB Registers:CTRLA=0x%02x, POTMT=%d, POTMW=%d, INTT=%d, DataRDYT =%d, Trigger=%s.\n"),
    //	m_reg.regCtrlA, m_reg.regPOTMT, m_reg.regPOTMW, m_reg.regINTT, m_reg.regDataRdyT, m_nTrig==0?("0-inner"):("1-outer"));
    memset(tmp,0,512);
    sprintf(tmp,"Master HMCB some Registers initial value: POTMT=%dus, POTMW=%dus, INTT=%dus, DataRDYT=%dus, DDI-LatchT=%dus, Trigger=%s.\n",
            m_reg.regPOTMT, m_reg.regPOTMW, m_reg.regINTT, m_reg.regDataRdyT, m_reg.regDDILatchT, m_nTrig==0?("0-inner"):("1-outer"));
    str = tmp;
    //std::cout <<(str);
    WriteLogFile(str);
    //////////////////////////////////////////////////////////////////////
    m_bSaveExdata = (IniFile.GetInt(("EXDATA"), ("SAVEDATA"), 0) ==1);
    if (m_bSaveExdata)
    {
        std::cout <<(("Set as save exdata mode.\n"));
        WriteLogFile(("Set as save exdata mode.\n"));
    }
    ///////////////////////////////////////////////////////////////
    //备用
    //是否为调试版本
    m_bDebug = (IniFile.GetInt(("ADM"), ("DEBUG"), 0) ==1);
    //////////////////////////////////////////////////////////////
    if(m_bDebug)
    {
        cmd_pkg_parse_log_file = cmd_pkg_parse_log_file_create();
    }
    else
    {
        cmd_pkg_parse_log_file = NULL;
    }
#endif
}

std::string CMCBCtrlC::GetProductVersion(std::string lpszFileName)
{
    std::string strVersion = ("");
    strVersion = gstrHMCBDllVersion;
    //TCHAR szSrcfn[MAX_PATH]; // 要获取信息的文件
    //if(NULL == lpszFileName)
    //    ::GetModuleFileName(NULL, szSrcfn, sizeof(szSrcfn));
    //else
    //    lstrcpy(szSrcfn, lpszFileName);

    //// 读文件信息
    //DWORD dwVerHnd = 0;
    //DWORD dwVerInfoSize = ::GetFileVersionInfoSize(szSrcfn, &dwVerHnd);
    //if(dwVerInfoSize)
    //{
    //    HANDLE hMem;
    //    void * lpvMem;
    //    unsigned int uInfoSize = 0;
    //    VS_FIXEDFILEINFO * pFileInfo;

    //    hMem = ::GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize);
    //    lpvMem = ::GlobalLock(hMem);
    //    ::GetFileVersionInfo(szSrcfn, dwVerHnd, dwVerInfoSize, lpvMem);
    //    ::VerQueryValue(lpvMem, (LPTSTR)("\\"), (void**)&pFileInfo, &uInfoSize);

    //    WORD nVer[4];
    //    nVer[0] = HIWORD(pFileInfo->dwProductVersionMS);
    //    nVer[1] = LOWORD(pFileInfo->dwProductVersionMS);
    //    nVer[2] = HIWORD(pFileInfo->dwProductVersionLS);
    //    nVer[3] = LOWORD(pFileInfo->dwProductVersionLS);
    //    strVersion.Format(("%d.%d.%d.%d "), nVer[0], nVer[1],nVer[2],nVer[3]);

    //    ::GlobalUnlock(hMem);
    //    ::GlobalFree(hMem);
    //}

    return strVersion;
}

CMCBCtrlC::~CMCBCtrlC()
{
    int i = 0;
    //
    if (m_bCLogPath)
    {
        m_file.close();
    }
    /////////////////////////////////
    for(i=0; i< MAX_MCBNUM; i++)
    {
        if(m_bDataErrorCLogPath[i])
        {
            m_DataErrorfile[i].close();
        }
        m_bDataErrorCLogPath[i] = false;
    }
}

//判断是否存在 D:\\XRayV3\\Config\\Setting.ini 或 D:\\XRayV4\\Config\\Setting.ini文件
//返回3:存在D:\\XRayV3\\Config\\Setting.ini
//返回4:存在D:\\XRayV4\\Config\\Setting.ini
//返回0:不存在XRayVx文件
DWORD CMCBCtrlC::IsExistXRayVxSetting()
{
    return 0;
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    //
    CString strDeviceType = ("");
    ////////////////////////////////////////////////////////////////
    CString strXRayV3SysInifilename = ("D:\\XRayV3\\Config\\Setting.ini");
    CString strXRayV4SysInifilename = ("D:\\XRayV4\\Config\\Setting.ini");
    //
    CFileStatus status;
    DWORD ReNCharsV3 = 0;
    DWORD ReNCharsV4 = 0;
    //读取文件状态
    if(CFile::GetStatus(strXRayV3SysInifilename, status ))
    {
        ReNCharsV3 = GetPrivateProfileString(("MainConfig"), ("DeviceType"), strDeviceType, strDeviceType.GetBuffer(MAX_PATH), MAX_PATH, strXRayV3SysInifilename);
    }
    //读取文件状态
    if(CFile::GetStatus(strXRayV4SysInifilename, status ))
    {
        ReNCharsV4 = GetPrivateProfileString(("MainConfig"), ("DeviceType"), strDeviceType, strDeviceType.GetBuffer(MAX_PATH), MAX_PATH, strXRayV4SysInifilename);
    }
    ////////////////////////////////////////////////////////////////
    if(ReNCharsV3 > 0)
    {
        return 3;
    }
    if(ReNCharsV4 > 0)
    {
        return 4;
    }
    //
    return 0;
#endif
}

void CMCBCtrlC::InitialAllMCB()
{
    int i = 0;
    int j = 0;
    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    //通过MCB_CMD_INITIAL消息初始化
    ////////////////////////////////////////////////////////////////
    memset(tmp,0,512);
    sprintf(tmp,"HMCB.dll configures LOGLEVEL = %d\n", m_nLogLevel);
    sz_szInfo = tmp;
    //std::cout << sz_szInfo;
    WriteLogFile(sz_szInfo);
    ////////////////////////////////////////////////////////////////
    sz_szInfo = ("Initial All HMCB...\n");
    //std::cout <<(sz_szInfo);
    WriteLogFile(sz_szInfo);
    ////////////////////////////////////////////////////////////////
    //gCtrl_Reg_WriteV变量初始化为0
    EDAQ_Ctrl_Reg_WriteValueInit();
    ////////////////////////////////////////////////////////////////
    //这一部分讲放到单个HMCB初始化里面

    ////////////////////////////////////////////////////////////////
    for (i=0; i< MAX_MCBNUM; i++)
    {
        m_MCBReadySts[i] = 0; //bit 0-connection, 1-MB fpga, 2- AD fpga, 3-Reg set
        m_bSetDMInfo[i] = false;
        //
        m_strMCBCoreFirmwareVer[i] = ("");
        //
        m_bMCBxInInitialing = false;
        //
        m_bEnAllData[i] = false;
    }
    ////////////////////////////////////////////////////////////////
    m_bDVReady = false;
    m_bStart = false; 
     //
    m_bRecStopCmd = false;
    //
    m_timeuncheck = NucTool::GetTickCount();
    m_timeInitSpan = NucTool::GetTickCount();//端口初始化失败间隔
    //
    m_nHeartbeatSec = 0;
    m_bSendAllData = false;

    ////////////////////////////////////////////////////////////////
    if(0 > ETH_WSAStartup())
    {
        std::string sz_szInfo = ("Error when calling WSAStartup()!\n");
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        return;
    }
    for(int j=0;j<m_nMCBNum;j++)
    {
        if(m_bEnMCBx4)
        {
            InitialMCB_HMCB4(j);
        }
        else
        {

            InitialMCB(j);
        }
    }
    /////////////////////////////////////////
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    InitializeCriticalSection(&ics);
#else
#endif

}

INT8U CMCBCtrlC::ResetCBLogicBeforeStart(unsigned short nMCB)
{
    INT8U ret = 0;
    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    int i=0;
    /////////////////////////////////////////////////////////////
    sprintf(tmp,"HMCB%d resets CB logic before starting a new acquisition.\n", nMCB);
    sz_szInfo = tmp;
    //std::cout <<(sz_szInfo);
    WriteLogFile(sz_szInfo);
    ////////////////////////////////////////////////////////////////
    //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
    ETH_CTRL_read_buffer_residual_data(nMCB);
    ////////////////////////////////////////////////////////////////
    //关闭控制和数据链接socket
    ETH_CTRL_close(nMCB);
    //
    if (m_pDataSocket[nMCB])
    {
        m_pDataSocket[nMCB]->Close();
        delete m_pDataSocket[nMCB];
        m_pDataSocket[nMCB] = NULL;
    }
    ///////////////////////////////////////////////////////////////////////////////
    //重新建立控制和数据链接socket，成功返回true
    bool bFuncRet = ConnectOpen (nMCB);// Reset CB Logic Before Start
    if(!bFuncRet)
    {
        return -1;
    }
    else
    {
        //time(&m_timeuncheck);
        m_timeuncheck = NucTool::GetTickCount();
        //
        ///////////////////////////////////////////////////////////////
        //恢复默认设置，禁止所有数据源
        ret = EDAQ_daq_mode_enable(0, nMCB);//退出DAQ模式
        if(ret > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_daq_mode_enable(0) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
            return ret;
        }
        ///////////////////////////////////////////////////////////////
        ret = EDAQ_TEST_DATA_enable(0, nMCB);	//使能(置一)或禁用（清零）FPGA内部计数型测试数据产生模块
        if(ret > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_TEST_DATA_enable(0) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo= tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////
        ret = EDAQ_SC_data_pkg_monitor_enable(0, nMCB);	//使能(置一)或禁用（清零）FPGA内嵌的SC数据包检查模块
        if(ret > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_SC_data_pkg_monitor_enable(0) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////
        //逻辑复位FPGA逻辑//The parsed result of DB num of DB-Line[0..3]对应寄存器值清0
        ret = EDAQ_CB_logic_reset(nMCB);
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        Sleep(5);
#else \
    //usleep(5*1000);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
#endif
        if(ret > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_CB_logic_reset() by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////
        //持续从ETH(_ETH_Data_socket)读取数据，直到清空缓存中的数据。
        ETH_CTRL_read_buffer_residual_data(nMCB);
        ///////////////////////////////////////////////////////////////
        //功能：设置MCB工作模式
        //参数:
        // 1: Work as Master HMCB
        // 0: Work as Cascaded HMCB
        if(0 == nMCB)
        {
            //设置HMCB级联工作模块
            ret = EDAQ_HMCB_work_mode_set(1, nMCB);	//1：主模式；0：从模式
            if(ret > 0)
            {
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling EDAQ_HMCB_work_mode_set(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                //std::cout <<(sz_szInfo);
                WriteLogFile(sz_szInfo);
            }
        }
        else
        {
            //设置HMCB级联工作模块
            ret = EDAQ_HMCB_work_mode_set(0, nMCB);	//1：主模式；0：从模式
            if(ret > 0)
            {
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling EDAQ_HMCB_work_mode_set(0) by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo= tmp;
                //std::cout <<(sz_szInfo);
                WriteLogFile(sz_szInfo);
            }
        }
        ///////////////////////////////////////////////////////////////
        //功能：设置DB Line数目//每个HMCB只开启一个DB Line
        ret = EDAQ_DB_line_num_set(1, nMCB);	//设置DB Line数目。
        if(ret > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_DB_line_num_set(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo= tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////
        //功能：设置DB板子数目
        ret = EDAQ_DB_num_set(0, 0, 0, m_DMInfo[nMCB].nDMNum, nMCB);//设置4个DB Line各自连接DB板数目。第一个参数，对应DB Line 3的DB num。
        if(ret > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_DB_num_set(0, 0, 0, %d) by HMCB%d. The return value is %d.\n", m_DMInfo[nMCB].nDMNum, nMCB, ret);
            sz_szInfo= tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        /////////////////////////////////////////////////////////////////
        if(m_nLogLevel >= 3)
        {
            INT8U nMaxDBLine = EDAQ_read_DB_line_max_num(nMCB);
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d is ECM1GA-%d and support the maximum value of DB line number is : %d.\n", nMCB, nMaxDBLine, nMaxDBLine);
            sz_szInfo = tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
            ///////////////////////////////////////////////////////////////
            //HMCB FPGA固件核心版本号和外围版本号生成时间
            EDAQ_FIRMWARE_VERSION_REG reg1;
            ret = ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR, &reg1.All, nMCB);
            if(ret > 0)
            {
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR,...) by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                //std::cout <<(sz_szInfo);
                WriteLogFile(sz_szInfo);
            }
            ///////////////////////////////////////////////////////////////
            //用户可自定义的状态寄存器(REV_STATUS_REG1,地址为32*4)用于表示HMCB FPGA固件外围版本号
            EDAQ_FIRMWARE_VERSION_REG reg2;
            ret = ECS_A32D32_read(CB_PERI_CIRCUITS_VERSION_REG_ADDR, &reg2.All, nMCB);
            if(ret > 0)
            {
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling ECS_A32D32_read(CB_PERI_CIRCUITS_VERSION_REG_ADDR,...) by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo= tmp;
                //std::cout <<(sz_szInfo);
                WriteLogFile(sz_szInfo);
            }
            ///////////////////////////////////////////////////////////////
            //EEPROM ID
            //INT64U rd_ID;
            //rd_ID = EDAQ_EEPROM_ID_read(nMCB);
            INT16U Int16PIDdata[64] = {0};
            ret = EDAQ_EEPROM_ID_read(nMCB,Int16PIDdata);
            //EEPROM中存储的板子生产时间
            std::string strProductionTime = ("");
            char time_str[30] = { 0x00 };
            std::string str_time;
            struct tm * info;
            time_t t;
            t = (time_t) EDAQ_production_time_read(nMCB);
            info = localtime(&t);
            if(NULL == info)
            {
                WriteLogFile("Failed to get the board production time.\n");
                str_time = "Failed to get";
            }
            else
            {
                memset(tmp,0,512);
                sprintf(tmp,"%04d/%02d/%02d %02d:%02d:%02d", 1900+info->tm_year, info->tm_mon, info->tm_mday, info->tm_hour, info->tm_min, info->tm_sec);
                str_time = tmp;
            }
            strProductionTime += str_time;
            /////////////////////////////////////////////////////////////////////
            unsigned short int nMCB_Version[MAX_MCBNUM] = { 0x00 };
            nMCB_Version[nMCB]  = reg1.Sepe.build_num;
            nMCB_Version[nMCB] += (reg1.Sepe.revision&0x0F)<<4;
            nMCB_Version[nMCB] += (reg1.Sepe.minor_ver&0x0F)<<8;
            nMCB_Version[nMCB] += (reg1.Sepe.major_ver&0x0F)<<12;
            /////////////////////////////////////////////////////////////////////
            memset(tmp,0,512);
            sprintf(tmp,"%d.%d.%d", reg1.Sepe.major_ver,reg1.Sepe.minor_ver,reg1.Sepe.revision);
            m_strMCBCoreFirmwareVer[nMCB] = tmp;
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d core firmware version: %s; HMCB other firmware version: %d.%d.%d.\n",nMCB, m_strMCBCoreFirmwareVer[nMCB].c_str(), reg2.Sepe.major_ver,reg2.Sepe.minor_ver,reg2.Sepe.revision);
            sz_szInfo = tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
            //
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Board ID: 0x%04X%04X%04X%04X; Board production time(Y/m/d H:M:S): %s.\n", nMCB, Int16PIDdata[3], Int16PIDdata[2], Int16PIDdata[1], Int16PIDdata[0], strProductionTime.c_str());
            sz_szInfo = tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
            ///////////////////////////////////////////////////////////////
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Re-Configure FPAG bin file.\n", nMCB);
            sz_szInfo = tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
            if (!ConfigFpga(nMCB))
            {
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling ConfigFpga() by HMCB%d. FPAG configuration failed.\n", nMCB);
                sz_szInfo = tmp;
                //std::cout <<(sz_szInfo);
                WriteLogFile(sz_szInfo);
                SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_FPGAFILE_ERR);
                ConnectClose(nMCB, 0);
            }
            else
            {
                //time(&m_timeuncheck);
                m_timeuncheck = NucTool::GetTickCount();
                //SetReg(nMCB);
            }
            ///////////////////////////////////////////////////////////////
            //下载FPGA后重新读取固件核心版本号等信息
            ///////////////////////////////////////////////////////////////
            //HMCB FPGA固件核心版本号和外围版本号生成时间
            EDAQ_FIRMWARE_VERSION_REG reg3;
            ret = ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR, &reg3.All, nMCB);
            if(ret > 0)
            {
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR,...) by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo =tmp;
                //std::cout <<(sz_szInfo);
                WriteLogFile(sz_szInfo);
            }
            ///////////////////////////////////////////////////////////////
            //用户可自定义的状态寄存器(REV_STATUS_REG1,地址为32*4)用于表示HMCB FPGA固件外围版本号
            EDAQ_FIRMWARE_VERSION_REG reg4;
            ret = ECS_A32D32_read(CB_PERI_CIRCUITS_VERSION_REG_ADDR, &reg4.All, nMCB);
            if(ret > 0)
            {
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling ECS_A32D32_read(CB_PERI_CIRCUITS_VERSION_REG_ADDR,...) by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                //std::cout <<(sz_szInfo);
                WriteLogFile(sz_szInfo);
            }
            ///////////////////////////////////////////////////////////////
            //EEPROM ID
            //INT64U rd_ID;
            // rd_ID = EDAQ_EEPROM_ID_read(nMCB);
            //EEPROM中存储的板子生产时间
            //CString strProductionTime = ("");
            strProductionTime = ("");
            //char time_str[30] = { 0x00 };
            //time_t t;
            //t = (time_t) EDAQ_production_time_read(nMCB);
            ret = EDAQ_EEPROM_ID_read(nMCB, Int16PIDdata);

            /////////////////////////////////////////////////////////////////////
            nMCB_Version[nMCB]  = reg3.Sepe.build_num;
            nMCB_Version[nMCB] += (reg3.Sepe.revision&0x0F)<<4;
            nMCB_Version[nMCB] += (reg3.Sepe.minor_ver&0x0F)<<8;
            nMCB_Version[nMCB] += (reg3.Sepe.major_ver&0x0F)<<12;
            /////////////////////////////////////////////////////////////////////
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d core firmware version: %d.%d.%d; HMCB other firmware version: %d.%d.%d.\n",nMCB, reg3.Sepe.major_ver,reg3.Sepe.minor_ver,reg3.Sepe.revision, reg4.Sepe.major_ver,reg4.Sepe.minor_ver,reg4.Sepe.revision);
            sz_szInfo = tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////////////
        ////设置CB工作参数
        //SetReg(nMCB);
        ///////////////////////////////////////////////////////////////////////
        ////正常启动后这一步可以重新设置，也可以不重新设置
        ////重新设置参数耗时会长，为了减少start时间，一般不再重新设置，后续记录 The parsed result of DB num of DB-Line[0..3]: 显示0为正常
        //if(m_nLogLevel >= 2)
        //{
        //	SetDMCmdReg(nMCB);
        //}
    }
    /////////////////////////////////////////////////////////////
    //DumpAllMCBAllTheECSRegisters();
    /////////////////////////////////////////////////////////////

    memset(tmp,0,512);
    sprintf(tmp,"Exit HMCB%d resets CB logic flow before starting a new acquisition.\n", nMCB);
    sz_szInfo = tmp;
    //std::cout <<(sz_szInfo);
    WriteLogFile(sz_szInfo);
    /////////////////////////////////////////////////////////////
    return ret;
}

void CMCBCtrlC::InitialMCB(unsigned short nMCB)
{
    unique_lock<mutex> lock(m_Initial_mutex);
    loger::getInstance()->writeLog(Info," InitialMCB 测试日志是否生成 ");
    m_bMCBxInInitialing = true;
    //
    int i=0;
    std::string sz_szInfo = ("");
    char tmp[512] = {0};

    i = nMCB;
    {
        m_MCBReadySts[i] = 0; //bit 0-connection, 1-MB fpga, 2- AD fpga, 3-Reg set
        m_bSetDMInfo[i] = false;
        //
        m_strMCBCoreFirmwareVer[i] = ("");
        //
        m_bEnAllData[i] = false;
    }

    if (nMCB > m_nMCBNum-1)
    {
        nMCB = m_nMCBNum-1;
    }

    m_bStart = false;
    m_bDVReady = false;
    m_bSetDMInfo[nMCB] = false;

    memset(tmp,0,512);
    sprintf(tmp,"Initiall HMCB No.%d ...\n", nMCB);
    sz_szInfo = tmp;
    //std::cout <<(sz_szInfo);
    WriteLogFile(sz_szInfo);
    //
    m_timeInitSpan = NucTool::GetTickCount();//端口初始化失败
    ///////////////////////////////////////////////////////////////
    INT8U ret = 0;
    INT8U chRetFlag = 0;
    ///////////////////////////////////////////////////////////////
    m_MCBReadySts[nMCB] = 0;
    /////////////////////////////////////////////////////////////
    // ConnectClose(nMCB, MCB_CMD_INITIAL);
    //
    bool bFuncRet = ConnectOpen (nMCB);// Initial MCBx
    if(!bFuncRet)
    {
        m_MCBReadySts[nMCB] |=0x10;//CTRL socket failed
        //
        m_bMCBxInInitialing = false;
        m_timehot[i] = NucTool::GetTickCount();
        lock.unlock();
        return;
    }
    if (bFuncRet)//open connection, configure fpga
    {
        chRetFlag = 0;
        //time(&m_timeuncheck);
        m_timeuncheck = NucTool::GetTickCount();
        //
        ///////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
        ETH_CTRL_read_buffer_residual_data(nMCB);
        ///////////////////////////////////////////////////////////////
        //恢复默认设置，禁止所有数据源
        ret = EDAQ_daq_mode_enable(0, nMCB);	//退出DAQ模式
        if(ret > 0)
        {
            chRetFlag += (0x01<<0);
            //
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_daq_mode_enable(0) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo =tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
            lock.unlock();
            return;
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"No error when calling EDAQ_daq_mode_enable(0) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout <<  sz_szInfo;
            WriteLogFile(sz_szInfo, gLogLevel1);
        }
        ///////////////////////////////////////////////////////////////
        ret = EDAQ_TEST_DATA_enable(0, nMCB);	//使能(置一)或禁用（清零）FPGA内部计数型测试数据产生模块
        if(ret > 0)
        {
            chRetFlag += (0x01<<1);
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_TEST_DATA_enable(0) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"No error when calling EDAQ_TEST_DATA_enable(0) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo, gLogLevel1);
        }
        ///////////////////////////////////////////////////////////////
        ret = EDAQ_SC_data_pkg_monitor_enable(0, nMCB);	//使能(置一)或禁用（清零）FPGA内嵌的SC数据包检查模块
        if(ret > 0)
        {
            chRetFlag += (0x01<<2);
            //
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_SC_data_pkg_monitor_enable(0) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo =tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"No error when calling EDAQ_SC_data_pkg_monitor_enable(0) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo, gLogLevel1);
        }
        ///////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
        ETH_CTRL_read_buffer_residual_data(nMCB);
        ///////////////////////////////////////////////////////////////
        //逻辑复位FPGA逻辑
        ret = EDAQ_CB_logic_reset(nMCB);
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        Sleep(5);
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
#endif
        if(ret > 0)
        {
            chRetFlag += (0x01<<3);
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_CB_logic_reset(2) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo= tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"No error when calling EDAQ_CB_logic_reset(2) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo, gLogLevel1);
        }
        ///////////////////////////////////////////////////////////////
        //通信错误判断
        if(chRetFlag)
        {
            m_MCBReadySts[nMCB] |=0x20;//Init communication failed
            //
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d initial communication failed. The error value is 0x%02X.\n", nMCB, chRetFlag);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
            //
            m_bMCBxInInitialing = false;
            m_timehot[i] = NucTool::GetTickCount();
            lock.unlock();
            return;
        }
        ///////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
        ETH_CTRL_read_buffer_residual_data(nMCB);
        ///
        ///
        //功能：设置MCB工作模式
        //参数:
        // 1: Work as Master HMCB
        // 0: Work as Cascaded HMCB
        if(0 == nMCB)
        {
            //设置HMCB级联工作模块
            ret = EDAQ_HMCB_work_mode_set(1, nMCB);	//1：主模式；0：从模式
            if(ret > 0)
            {
                chRetFlag += (0x01<<4);
                memset(tmp,0,512);
                sprintf(tmp, "Error when calling EDAQ_HMCB_work_mode_set(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo =tmp;
                //std::cout <<(sz_szInfo);
                WriteLogFile(sz_szInfo);
            }
            else
            {
                memset(tmp,0,512);
                sprintf(tmp, "No error when calling EDAQ_HMCB_work_mode_set(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo =tmp;
                //std::cout <<(sz_szInfo);
                WriteLogFile(sz_szInfo);
            }

        }
        else
        {
            //设置HMCB级联工作模块
            ret = EDAQ_HMCB_work_mode_set(0, nMCB);	//1：主模式；0：从模式
            if(ret > 0)
            {
                chRetFlag += (0x01<<4);
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling EDAQ_HMCB_work_mode_set(0) by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo=tmp;
                //std::cout <<(sz_szInfo);
                WriteLogFile(sz_szInfo);
            }
            else
            {
                memset(tmp,0,512);
                sprintf(tmp, "No error when calling EDAQ_HMCB_work_mode_set(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo =tmp;
                //std::cout <<(sz_szInfo);
                WriteLogFile(sz_szInfo);
            }
        }
        ///////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据
        ETH_CTRL_read_buffer_residual_data(nMCB);
            ///////////////////////////////////////////////////////////////
        //功能：设置DB Line数目//每个HMCB只开启一个DB Line
        ret = EDAQ_DB_line_num_set(1, nMCB);	//设置DB Line数目。
        if(ret > 0)
        {
            chRetFlag += (0x01<<5);
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_DB_line_num_set(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo=tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"No error when calling EDAQ_DB_line_num_set(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo, gLogLevel1);
        }
        ///////////////////////////////////////////////////////////////
        //功能：设置DB板子数目
        ret = EDAQ_DB_num_set(0, 0, 0, m_DMInfo[nMCB].nDMNum, nMCB);//设置4个DB Line各自连接DB板数目。第一个参数，对应DB Line 3的DB num。
        if(ret > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_DB_num_set(0, 0, 0, %d) by HMCB%d. The return value is %d.\n", m_DMInfo[nMCB].nDMNum, nMCB, ret);
            sz_szInfo=tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
        ETH_CTRL_read_buffer_residual_data(nMCB);
        ///////////////////////////////////////////////////////////////
        INT8U nMaxDBLine = EDAQ_read_DB_line_max_num(nMCB);
        memset(tmp,0,512);
        sprintf(tmp,"HMCB%d is ECM1GA-%d and support the maximum value of DB line number is : %d.\n", nMCB, nMaxDBLine, nMaxDBLine);
        sz_szInfo=tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        ///////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
        ETH_CTRL_read_buffer_residual_data(nMCB);
        ///////////////////////////////////////////////////////////////
        //HMCB FPGA固件核心版本号和外围版本号生成时间
        EDAQ_FIRMWARE_VERSION_REG reg1;
        ret = ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR, &reg1.All, nMCB);
        if(ret > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR,...) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo=tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////
        //用户可自定义的状态寄存器(REV_STATUS_REG1,地址为32*4)用于表示HMCB FPGA固件外围版本号
        EDAQ_FIRMWARE_VERSION_REG reg2;
        ret = ECS_A32D32_read(CB_PERI_CIRCUITS_VERSION_REG_ADDR, &reg2.All, nMCB);
        if(ret > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling ECS_A32D32_read(CB_PERI_CIRCUITS_VERSION_REG_ADDR,...) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////
        //EEPROM ID
        //		INT64U rd_ID;
        //		rd_ID = EDAQ_EEPROM_ID_read(nMCB);
        INT16U Int16PIDdata[64] = { 0 };
        ret = EDAQ_EEPROM_ID_read(nMCB, Int16PIDdata);
        //EEPROM中存储的板子生产时间
        std::string strProductionTime = ("");
        char time_str[30] = { 0x00 };
        struct tm *info;
        std::string str_time = "";
        time_t t;
        t = (time_t) EDAQ_production_time_read(nMCB);
        info = localtime(&t);
        if(NULL == info)
        {
            WriteLogFile("Failed to get the board production time.\n");
            str_time = "Failed to get";
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"%04d/%02d/%02d %02d:%02d:%02d", 1900+info->tm_year, info->tm_mon, info->tm_mday, info->tm_hour, info->tm_min, info->tm_sec);
            str_time = tmp;
        }
        strProductionTime += str_time;
        /////////////////////////////////////////////////////////////////////
        unsigned short int nMCB_Version[MAX_MCBNUM] = { 0x00 };
        nMCB_Version[nMCB]  = reg1.Sepe.build_num;
        nMCB_Version[nMCB] += (reg1.Sepe.revision&0x0F)<<4;
        nMCB_Version[nMCB] += (reg1.Sepe.minor_ver&0x0F)<<8;
        nMCB_Version[nMCB] += (reg1.Sepe.major_ver&0x0F)<<12;
        //
        m_nMCB_Version[nMCB] = nMCB_Version[nMCB];
        /////////////////////////////////////////////////////////////////////
        memset(tmp,0,512);
        sprintf(tmp,"%d.%d.%d", reg1.Sepe.major_ver,reg1.Sepe.minor_ver,reg1.Sepe.revision);
        m_strMCBCoreFirmwareVer[nMCB]=tmp;
        memset(tmp,0,512);
        sprintf(tmp,"HMCB%d core firmware version: %s; HMCB other firmware version: %d.%d.%d.\n",nMCB, m_strMCBCoreFirmwareVer[nMCB].c_str(), reg2.Sepe.major_ver,reg2.Sepe.minor_ver,reg2.Sepe.revision);
        sz_szInfo=tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);

        //
        memset(tmp,0,512);
        sprintf(tmp,"HMCB%d Board ID: 0x%04X%04X%04X%04X; Board production time(Y/m/d H:M:S): %s.\n", nMCB, Int16PIDdata[3], Int16PIDdata[2], Int16PIDdata[1], Int16PIDdata[0], strProductionTime.c_str());
        sz_szInfo=tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        m_timeInitSpan = NucTool::GetTickCount();//端口初始化失败间隔
        ///////////////////////////////////////////////////////////////
        if (!ConfigFpga(nMCB))
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling ConfigFpga() by HMCB%d. FPAG configuration failed.\n", nMCB);
            sz_szInfo=tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
            SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_FPGAFILE_ERR);
            ConnectClose(nMCB, 0);
            //
            m_MCBReadySts[nMCB] |=0x30;//communicate and send command to HMCB failed
            //
            m_bMCBxInInitialing = false;
            m_timehot[i] = NucTool::GetTickCount();
            lock.unlock();
            return;
        }
        else
        {
            //time(&m_timeuncheck);
            m_timeuncheck = NucTool::GetTickCount();
            //SetReg(nMCB);
        }
        m_timeInitSpan = NucTool::GetTickCount();//端口初始化失败间隔
        ///////////////////////////////////////////////////////////////
        //下载FPGA后重新读取固件核心版本号等信息
        ///////////////////////////////////////////////////////////////
        //HMCB FPGA固件核心版本号和外围版本号生成时间
        EDAQ_FIRMWARE_VERSION_REG reg3;
        ret = ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR, &reg3.All, nMCB);
        if(ret > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR,...) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////
        //用户可自定义的状态寄存器(REV_STATUS_REG1,地址为32*4)用于表示HMCB FPGA固件外围版本号
        EDAQ_FIRMWARE_VERSION_REG reg4;
        ret = ECS_A32D32_read(CB_PERI_CIRCUITS_VERSION_REG_ADDR, &reg4.All, nMCB);
        if(ret > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling ECS_A32D32_read(CB_PERI_CIRCUITS_VERSION_REG_ADDR,...) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////
        //EEPROM ID
        //INT64U rd_ID;
        //rd_ID = EDAQ_EEPROM_ID_read(nMCB);
        ret = EDAQ_EEPROM_ID_read(nMCB, Int16PIDdata);

        /////////////////////////////////////////////////////////////////////
        nMCB_Version[nMCB]  = reg3.Sepe.build_num;
        nMCB_Version[nMCB] += (reg3.Sepe.revision&0x0F)<<4;
        nMCB_Version[nMCB] += (reg3.Sepe.minor_ver&0x0F)<<8;
        nMCB_Version[nMCB] += (reg3.Sepe.major_ver&0x0F)<<12;
        /////////////////////////////////////////////////////////////////////
        memset(tmp,0,512);
        sprintf(tmp,"HMCB%d core firmware version: %d.%d.%d; HMCB other firmware version: %d.%d.%d.\n",nMCB, reg3.Sepe.major_ver,reg3.Sepe.minor_ver,reg3.Sepe.revision, reg4.Sepe.major_ver,reg4.Sepe.minor_ver,reg4.Sepe.revision);
        sz_szInfo = tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        /////////////////////////////////////////////////////////////////////
        //设置CB工作参数
        SetReg(nMCB);
        /////////////////////////////////////////////////////////////////////
        int nRet = 0;
        // 设置各DBline的DM配置参数
        nRet = SetDMCmdReg(nMCB);
        //
        if(0 == nRet)
        {
            //sz_szInfo.Format(_T("HMCB(4)-DB%d Auto DM number:%d.\n"), nDbline, 0);
            //TRACE(sz_szInfo);
            //WriteLogFile(sz_szInfo);
            //
            m_MCBReadySts[nMCB] |=0x08;
            SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_REGSET_OK);
        }
        //else
        if(0 != nRet)
        {
            //////////////////////////////////////////////////////////////////////////////////
            memset(tmp,0,512);
            sprintf(tmp,"HMCB(4)%d failed to self check the number of detector modules. The device will be reconnected..\n", nMCB);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
            //sz_szInfo.Format(_T("The setting value of HMCB(4)%d DB num of DB-Line0 is not equal to the parsed result of DB num of DB-Line0.\n"), nMCB);
            //TRACE(sz_szInfo);
            //WriteLogFile(sz_szInfo);
            m_MCBReadySts[nMCB] |=0x10;//自检失败
            //兼容老版本报 Detector module overflow
            SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_DMOVERFLOW);
            //新版本报 Detector module number error.
            SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_DMNUM_ERROR);
        }
    }
    m_timeInitSpan =NucTool::GetTickCount();//端口初始化失败间隔
    //time(&m_timeuncheck);
    m_timeuncheck = NucTool::GetTickCount();
    m_bMCBxInInitialing = false;
    m_timehot[i] =NucTool::GetTickCount();
}


//通过socket获取数据并保存至FIFO缓存中
void CMCBCtrlC::FetchMsg(CRequestSocket *pRequestSocket, int nIndex, int nPort)
{
    if(m_bEnMCBx4)
    {
        FetchMsg_HMCB4(pRequestSocket, nIndex, nPort);
        return;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    EnterCriticalSection(&ics);
    //
    WaitForSingleObject(hMutex, INFINITE);
#else
    //boost::shared_lock<boost::shared_mutex> read_lock(mutex1);
     unique_lock<mutex> lock(m_Data_mutex[nIndex]);
#endif
    //
    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //一次数据读取所读取的数据字节数
    INT32S Rec_Bytelen = 0;
    //MCB序号
    int nMCB = nIndex;
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //数据采集未启动之前的数据，不处理
    if(!m_bStart)
    {
        if(pRequestSocket->Available()>0)
        {
            //Rec_Bytelen = pRequestSocket->Receive(&glb_data_buf[nMCB][0], GLB_DATA_BUF_SIZE);
            unsigned char * chRecv = new unsigned char [SOCKETBUFSIZE];
            Rec_Bytelen = pRequestSocket->Receive(chRecv, SOCKETBUFSIZE);
            //std::vector<unsigned char> TempVector(chRecv, chRecv+Rec_Bytelen);
            //glb_data_buf_vetor[nMCB].insert(glb_data_buf_vetor[nMCB].end(), TempVector.begin(), TempVector.end());
            delete [] chRecv;
            //
            glb_data_buf_vetor[nMCB].clear();
            m_timeSocketRecData[nMCB] = NucTool::GetTickCount();
            //
            glb_data_AccWrBytes[nMCB] = 0;
            m_SocketRecBytes[nMCB] = 0;
            m_TimeSaveSocketData[nMCB] = NucTool::GetTickCount();

            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Socket Receives data before [Start], with a return data length and total receive data length are : %d / %I64u \n", nMCB, Rec_Bytelen, glb_data_AccWrBytes[nMCB] );
            sz_szInfo = tmp;
            //WriteLogFile(sz_szInfo, gLogLevel2);
            WriteLogFile(sz_szInfo);
        }
        lock.unlock();
        return;
    }
    //glb_data_buf_vetor[nMCB].clear();
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //数据缓存区可以写的字节长度
    INT32U buf_WR_EmptyLength = 0;
    INT32U buf_WR_Pt = 0;
    //
    ULONGLONG buffDataLength = 0;
    //来自数据端口的数据
    if(nPort == ETH_DATA_PORT)
    {

        //从socket接收数据放到缓存区

        if(pRequestSocket->Available()>0)
        {
            //Rec_Bytelen = pRequestSocket->Receive(&glb_data_buf[nMCB][buf_WR_Pt], buf_WR_EmptyLength);
            //从socket接收数据放到缓存区
            unsigned char * chRecv = new unsigned char [SOCKETBUFSIZE];
            Rec_Bytelen = pRequestSocket->Receive(chRecv, SOCKETBUFSIZE);

            if(Rec_Bytelen > 0)
            {
                std::vector<unsigned char> TempVector(chRecv, chRecv+Rec_Bytelen);
                glb_data_buf_vetor[nMCB].insert(glb_data_buf_vetor[nMCB].end(), TempVector.begin(), TempVector.end());
                m_timeSocketRecData[nMCB] = NucTool::GetTickCount();

                //
                buffDataLength = glb_data_buf_vetor[nMCB].size();
                //缓冲区内有大于等于2次触发的未处理数据
                if(buffDataLength >= 2*gbl_DBLine_Size[nMCB][0])
                {
                    //2024-03-19新增
                    RECVPARAM *pRecvParam0 = new RECVPARAM;
                    pRecvParam0->nMCBIndex = 0;
                    pRecvParam0->pkg_num = m_lcounter;
                    ETH_read_and_parse_dataEx_Thread((void*) pRecvParam0);
                }
            }
            delete [] chRecv;
        }
        if(Rec_Bytelen <=0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling socket Receive() by HMCB%d. The return value is : %d \n", nMCB, Rec_Bytelen);
            sz_szInfo = tmp;
           // WriteLogFile(sz_szInfo);
            return;
        }
        else if(Rec_Bytelen > 0)
        {

            if(gLogLevel2 <= m_nLogLevel)
            {
                memset(tmp,0,512);
                sprintf(tmp,"HMCB%d socket Received data successfully. The lengths of returned data and total received data are : %d / %I64u \n", nMCB, Rec_Bytelen, glb_data_AccWrBytes[nMCB] );
                sz_szInfo = tmp;
                WriteLogFile(sz_szInfo, gLogLevel2);
            }
            //
            INT32S nWr = 0;
            std::string strTemp = ("");
            sz_szInfo = ("");
            if((0 == nMCB) && (m_bSocketRawDatafile) && (m_nLogLevel >= gLogLevel5))
            {
                //保存原始数据到
                for(nWr = 0; nWr < Rec_Bytelen; nWr++)
                {
                    memset(tmp,0,512);
                    sprintf(tmp,"%02X ", glb_data_buf_vetor[nMCB].at(glb_data_buf_vetor[nMCB].back()-Rec_Bytelen+nWr));
                    strTemp = tmp;
                    //
                    m_unSocketRawWrCnt++;
                    if(0 == (m_unSocketRawWrCnt % 64))
                    {
                        strTemp += ("\n");
                    }
                    sz_szInfo += strTemp;
                }
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
                m_SocketRawDatafile.WriteString(sz_szInfo);
                m_SocketRawDatafile.Flush();
#else
                m_SocketRawDatafile << sz_szInfo;
                m_SocketRawDatafile.flush();
#endif
            }
        }
        /////////////////////////////////////////////////////
        if(m_nLogLevel > gLogLevel1)
        {
            m_SocketRecBytes[nMCB] += Rec_Bytelen;
            //
            DWORD dwTemp_Timenow = NucTool::GetTickCount();
            DWORD dwLapTick = dwTemp_Timenow - m_TimeSaveSocketData[nMCB];
            if(dwLapTick > 1000)
            {
                memset(tmp,0,512);
                sprintf(tmp,"HMCB%d the lengths of returned data and band width: %I64u bytes / %d Bytes/s\n", nMCB, glb_data_AccWrBytes[nMCB], (int)(m_SocketRecBytes[nMCB]*1000.0/dwLapTick));
                sz_szInfo = tmp;
                WriteLogFile(sz_szInfo, gLogLevel2);
                //
                m_SocketRecBytes[nMCB] = 0;
                //
                m_TimeSaveSocketData[nMCB] = dwTemp_Timenow;
            }
        }
    }
    //////////////////////////////////////////////////////////////////
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    ReleaseMutex(hMutex);
    //
    LeaveCriticalSection(&ics);
#else
     lock.unlock();
#endif
}

void CMCBCtrlC::NormalEndDAQ(unsigned short nMCB)
{
    std::string sz_szInfo = ("");
    std::string str = ("");
    char tmp[512] = {0};
    //持续从ETH读取数据，直到清空缓存中的数据。
    //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
    ETH_CTRL_read_buffer_residual_data(nMCB);
    /////////////////////////////////////////////////////////////////
    EDAQ_daq_mode_enable(0, nMCB);
    EDAQ_daq_mode_enable(0, nMCB);
    //持续从ETH读取数据，直到清空缓存中的数据。
    //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
    ETH_CTRL_read_buffer_residual_data(nMCB);
    /////////////////////////////////////////////////////////////////
    //ETH_read_buffer_residual_data();
    //EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_ETH_DATA);
    ////ETH_data_read_parse(pkg_num, DB_DAQ_PKG);
    ////ETH_data_read_parse(10000,DB_DAQ_PKG);
    ////EDAQ_daq_mode_enable(0);
    //持续从ETH读取数据，直到清空缓存中的数据。
    //ETH_read_buffer_residual_data();
    ///////////////////////////////////////////////////////////
    memset(tmp,0,512);
    sprintf(tmp,"HMCB%d gather packet number:%d, send to DLL:%d.\n",
            nMCB, m_MCBData[nMCB].d_head.head_use.pcounter.l, m_MCBData[nMCB].d_head.head_use.scounter);
    sz_szInfo = tmp;
    //std::cout <<(sz_szInfo);
    WriteLogFile(sz_szInfo);

    ///////////////////////////////////////////////////////////
    //异常情况处理
    //控制板本次采集没有采集到数据(为0),发送MCB_STSCD_SCNTPCNTDIFF消息，以便让节点重启
    if((m_MCBData[nMCB].d_head.head_use.pcounter.l < 1) || (m_MCBData[nMCB].d_head.head_use.scounter < 1))
    {
        ///////////////////////////////////////////////////////////
        memset(tmp,0,512);
        sprintf(tmp,"HMCB%d gather packet number is less than 1, then send an MCB_STSCD_SCNTPCNTDIFF message to App for node restart.\n", nMCB);
        sz_szInfo = tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);

        //////////////////////////////////////////////////////////
        if(m_pDataSocket[nMCB]->CheckSocketStatus()<0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Data Socket abnormal  !!!!!!!.\n", nMCB);
            sz_szInfo = tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        /// //////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_SCNTPCNTDIFF);
    }
    ///////////////////////////////////////////////////////////

    if(m_bPreStop)
    {
        memset(tmp,0,512);
        sprintf(tmp,"HMCB%d stop receive data by command successful! Send packet to APP:%d.\n", nMCB, Update_pkg_n[nMCB]);
        sz_szInfo = tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //
        //memset(tmp,0,512);
        //sprintf(tmp,"HMCB%d Send Message to APP: msg=0x%08x(MCB_MSG_STATUS), wParam=0x%08x(MCB_STOPOK), lParam=0x%08x.\n", nMCB, MCB_MSG_STATUS, MCB_STOPOK, 0);
        //sz_szInfo= tmp;
        ////std::cout <<(sz_szInfo);
        //WriteLogFile(sz_szInfo);
		//if(m_bRecStopCmd)
		//{
	 //       //gCallBack(MCB_MSG_STATUS, MCB_STOPOK, ((long(nMCB))<<16) + 0);//normal stop 0 -by cmd, 1 - normal
		//	SendMCBMsg(MCB_MSG_STATUS, MCB_STOPOK, 0);//normal stop 0 -by cmd, 1 - normal
		//}
		//else
		//{
	 //       //gCallBack(MCB_MSG_STATUS, MCB_STOPOK, ((long(nMCB))<<16) + 1);//normal stop 0 -by cmd, 1 - normal
		//	SendMCBMsg(MCB_MSG_STATUS, MCB_STOPOK, 1);//normal stop 0 -by cmd, 1 - normal
		//}
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"HMCB%d normal stop receive data successful! Send packet to APP:%d\n", nMCB, Update_pkg_n[nMCB]);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //
        //memset(tmp,0,512);
        //sprintf(tmp,"MCB No.%d Send Message to APP: msg=0x%08x(MCB_MSG_STATUS), wParam=0x%08x(MCB_STOPOK), lParam=0x%08x.\n", nMCB, MCB_MSG_STATUS, MCB_STOPOK, 1);
        //sz_szInfo= tmp;
        ////std::cout <<(sz_szInfo);
        //WriteLogFile(sz_szInfo);
		//
		//if(m_bRecStopCmd)
		//{
	 //       //gCallBack(MCB_MSG_STATUS, MCB_STOPOK, ((long(nMCB))<<16) + 0);//normal stop 0 -by cmd, 1 - normal
		//	SendMCBMsg(MCB_MSG_STATUS, MCB_STOPOK, 0);//normal stop 0 -by cmd, 1 - normal
		//}
		//else
		//{
	 //       //gCallBack(MCB_MSG_STATUS, MCB_STOPOK, ((long(nMCB))<<16) + 1);//normal stop 0 -by cmd, 1 - normal
		//	SendMCBMsg(MCB_MSG_STATUS, MCB_STOPOK, 1);//normal stop 0 -by cmd, 1 - normal
		//}
    }
    ////////////////////////////////////
    m_bPreDataEnd = true;
    //m_MCBRunSts[nMCB] |= 0x04;
    //time(&m_timepreend);
    m_timepreend = NucTool::GetTickCount();
    ////////////////////////////////////
    m_bStart = false;
}

void CMCBCtrlC::ConnectClose(unsigned short nMCB, UINT wParam /*= 0*/)
{

    ///////////////////////////////////////////////////////////////////////////////
    std::string sz_szInfo = ("");
    char tmp[512] = {0};

    memset(tmp,0,512);
    sprintf(tmp,"HMCB%d first calls ConnectClose().\n", nMCB);
    sz_szInfo= tmp;
    //std::cout <<(sz_szInfo);
    WriteLogFile(sz_szInfo, gLogLevel1);
    ///////////////////////////////////////////////////////////////////////////////
    //相关状态变量设置
    m_bDVReady = false;
    //
    m_bStart = false;
    //
    m_MCBReadySts[nMCB] = 0;
    //
    m_bSetDMInfo[nMCB] = false;
    ///////////////////////////////////////////////////////////////////////////////
    if(ETH_CTRL_IsValid(nMCB))
    {
        ETH_CTRL_close(nMCB);
        //
        //if(0 == wParam)
        {
            memset(tmp,0,512);
            sprintf(tmp,"HMCB No.%d CTRL connection status: closed!\n", nMCB);
            sz_szInfo = tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
            SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_CONNECTCLOSE);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    if (m_pDataSocket[nMCB])
    {
        m_pDataSocket[nMCB]->Close();
        delete m_pDataSocket[nMCB];
        m_pDataSocket[nMCB] = NULL;
        //
        memset(tmp,0,512);
        sprintf(tmp,"HMCB No.%d DATA connection status: closed!\n", nMCB);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_CONNECTCLOSE);
        //Sleep(200);  //wait for MCB close connection
    }
    ///////////////////////////////////////////////////////////////////////////////
    m_timeunconnect[nMCB] = NucTool::GetTickCount();
}

bool CMCBCtrlC::ConnectOpen(unsigned short nMCB)
{
    std::string str = ("");
    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    //socket端口建立状态 0初始没有建立 1: ctrl socket 建立;  2: Data socket建立 3:ctrl and Data socket 建立
    int nDataAndCtrlSocketSt = 0;
    //控制端口、数据端口
    if(m_pDataSocket[nMCB]) //connection has been established
    {
        return true;
    }
    else
    {
        //MCB Control port connection established
        //m_MCBReadySts[nMCB][bit0]: 为1：data and ctrl socket正常建立
        m_MCBReadySts[nMCB] &=0xFE;//bit0清零
        ///////////////////////////////////////////////////

        in_addr netAddr;
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#if defined(VC6_BUILD) && (defined(UNICODE) || defined(_UNICODE))
        netAddr.S_un.S_addr =inet_addr(m_szMCBIPAddr_sttConvert.WChar2AChar(m_szMCBIPAddr)) + (nMCB<<24);
#else
        netAddr.S_un.S_addr =inet_addr(((LPCSTR)m_szMCBIPAddr)) + (nMCB<<24);
#endif //VC6_BUILD, UNICODE,_UNICODE
#else
        netAddr.s_addr =inet_addr(m_szMCBIPAddr.c_str()) + (nMCB<<24);
#endif
        sz_szInfo =inet_ntoa(netAddr);
        memset(tmp,0,512);
        sprintf(tmp,"HMCB%d socket connection status: connecting %s...\n", nMCB, sz_szInfo.c_str());
        str = tmp;
        //std::cout <<(str);
        WriteLogFile(str);
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_CONNECTING);
        /////////////////////////////////////////////////////////////////////////////////////
        //CTRL Socket
        INT32S Ret = 0;
        Ret = ETH_init(netAddr.s_addr, nMCB);
        //
        if (Ret < 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d CTRL socket connection status: socket failed!\n", nMCB);
            sz_szInfo= tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
            SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_SOCKETFAIL);
            //
            m_timeunconnect[nMCB] = NucTool::GetTickCount();
            //
            nDataAndCtrlSocketSt &= 0xFE;
            m_MCBReadySts[nMCB] |=0x10;//CTRL socket failed
            return false;
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"HMCB No.%d CTRL socket connection status: successful!\n", nMCB);
            sz_szInfo= tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
            m_timehot[nMCB] = NucTool::GetTickCount();
            /////////////////////////////////////////////////////////////////////////////////////
            //MCB Control port connection established
            nDataAndCtrlSocketSt |= 0x01;
            //
            SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_CTRLCONNECTOK);
            // 不能返回,还要接着建立data socket
            //return true;
        }
        /////////////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////////////
        //数据 data socket
        //boost::unique_lock<boost::mutex> lock(mutex1);
        if (!(m_pDataSocket[nMCB] = OpenSocket(nMCB, ETH_DATA_PORT)))
        {
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d DATA socket connection status: socket failed!\n", nMCB);
            sz_szInfo= tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
            SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_SOCKETFAIL);
            //
            m_timeunconnect[nMCB] = NucTool::GetTickCount();
            //
            nDataAndCtrlSocketSt &= 0xFD;
            m_MCBReadySts[nMCB] |=0x10;//data socket failed
            return false;
        }
        else if (!ConnectServer(m_pDataSocket[nMCB], nMCB, ETH_DATA_PORT))
        {
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d DATA socket connection status: failed!\n", nMCB);
            sz_szInfo= tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
            delete m_pDataSocket[nMCB];
            m_pDataSocket[nMCB] = NULL;
            //
            m_timeunconnect[nMCB] = NucTool::GetTickCount();
            SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_CONNECTFAIL);
            //
            nDataAndCtrlSocketSt &= 0xFD;
            m_MCBReadySts[nMCB] |=0x10;//data socket failed
            return false;
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d DATA socket connection status: successful!\n", nMCB);
            sz_szInfo= tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
            //
            m_timehot[nMCB] = NucTool::GetTickCount();
            /////////////////////////////////////////////////////////////////////////////////////
            //MCB Data port connection established
            nDataAndCtrlSocketSt |= 0x02;
            //
            SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_DATACONNECTOK);
            //return true;
        }
        /////////////////////////////////////////////////////////////////////////////////////
        // data and ctrl Sockets are ready
        if(0x03 == nDataAndCtrlSocketSt)
        {
            m_MCBReadySts[nMCB] |=0x01;//bit0置1
        }
        else
        {
            m_MCBReadySts[nMCB] &=0xFE;//bit0清零
        }
        /////////////////////////////////////////////////////////////////////////////////////
        return true;
    }
}


CRequestSocket* CMCBCtrlC::OpenSocket(unsigned short nMCB, int nPort)
{
    CRequestSocket* pSocket =new CRequestSocket(this, nMCB, nPort);

//	if (!(pSocket->Create()))
//	{
//		delete pSocket;
//		return NULL;
//	}
#define SEND_BUFSIZE (1440)
    int nSndBuf = SEND_BUFSIZE*2;
    pSocket->SetSockOpt(SO_SNDBUF, (char*)&nSndBuf, sizeof(int));

    //设置接收缓存为64M
    int nRcvBuf = 64*1024*1024;
    pSocket->SetSockOpt(SO_RCVBUF, (char*)&nRcvBuf, sizeof(int));
    nRcvBuf =0;
    int nlen= sizeof(int);
    pSocket->GetSockOpt(SO_RCVBUF, (char*)&nRcvBuf, &nlen);
    return pSocket;
}

bool CMCBCtrlC::ConnectServer(CRequestSocket *pSocket, unsigned short nMCB, int nPort)
{
    std::string sz_szInfo;

    in_addr netAddr;
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#if defined(VC6_BUILD) && (defined(UNICODE) || defined(_UNICODE))
    netAddr.S_un.S_addr =inet_addr(m_szMCBIPAddr_sttConvert.WChar2AChar(m_szMCBIPAddr)) + (nMCB<<24);
#else
    netAddr.S_un.S_addr =inet_addr(((LPCSTR)m_szMCBIPAddr)) + (nMCB<<24);
#endif //VC6_BUILD, UNICODE,_UNICODE
#else
    netAddr.s_addr =inet_addr(m_szMCBIPAddr.c_str()) + (nMCB<<24);
#endif
    sz_szInfo =inet_ntoa(netAddr);

    if(pSocket == NULL)
    {
        return false;
    }

    if(!pSocket->Connect(sz_szInfo, nPort))
    {
        return false;
    }
    return true;

}

void CMCBCtrlC::CloseMCB()
{
    std::string sz_szInfo = ("");
    sz_szInfo = "HMCB.dll calls CloseMCB().\n";
    //std::cout <<(sz_szInfo);
    WriteLogFile(sz_szInfo);
    /////////////////////////////////////////////////////////////////////////
    m_bconnectauto = false;
    for(unsigned short i =0; i< m_nMCBNum; i++)
    {
        ConnectClose(i, 0);
    }
    //
    ETH_WSACleanup();
}

bool CMCBCtrlC::ConfigFpga(unsigned short nMCB)
{
    /////////////////////////////////////////////////////////////////////////
    int i = nMCB;
    int j = 0;
    /////////////////////////////////////////////////////////////////////////
    std::string sz_szInfo = ("");
    std::string filename = ("");
    char tmp[512] = {0};
    ////////////////////////////////////////////////////////////////
    std::string strBinfilePath = ("");
    //判断是否存在 D:\\XRayV3\\Config\\Setting.ini 或 D:\\XRayV4\\Config\\Setting.ini文件且设置了("MainConfig"), ("DeviceType"),参数
    DWORD nXRayV = IsExistXRayVxSetting();
    //
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    if(3 == nXRayV)
    {
        strBinfilePath = ("D:\\XRayV3\\Config\\Binfile");
    }
    else if(4 == nXRayV)
    {
        strBinfilePath = ("D:\\XRayV4\\Config\\Binfile");
    }
#else
    if(3 == nXRayV)
    {
        strBinfilePath = ("home/nuctech/OSS/XRayV3/Config\\Binfile");
    }
    else if(4 == nXRayV)
    {
        strBinfilePath = ("home/nuctech/OSS/XRayV4/Config/Binfile");
    }
#endif
    else
    {
        if(m_bPath)
        {
            strBinfilePath = m_szPath;
        }
        else
        {
            strBinfilePath = ("");
        }
    }
    /////////////////////////////////////////////////////////////
    filename = strBinfilePath + ("/") + m_szMCB_FPGAFilename[i];
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int strLength = filename.size()+1;
    char *pFPGAConfFile = new char[strLength];
    memset(pFPGAConfFile, 0, strLength);
    memcpy(pFPGAConfFile,filename.c_str(),strLength);
    //    for(j = 0; j < strLength; j++)
    //	{
    //        pFPGAConfFile[j] = (char)(filename.at(j));
    //	}
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    memset(tmp,0,512);
    sprintf(tmp,"HMCB%d FPGA configuration file: %s. 1\n", i, filename.c_str()) ;
    sz_szInfo = tmp;
    //std::cout <<(sz_szInfo);
    WriteLogFile(sz_szInfo);
    /////////////////////////////////////////////////////////////////////////
    INT8U ret = 0;
    INT8U nLastError = 0;
    /////////////////////////////////////////////////////////////////////////
    ret = EDAQ_CB_logic_reset(nMCB);				//CB逻辑复位
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    Sleep(5);
#else
    //usleep(5*1000);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
#endif
    if(ret > 0)
    {
        nLastError++;
        memset(tmp,0,512);
        sprintf(tmp,"Error when calling EDAQ_CB_logic_reset(3) by HMCB%d. The return value is %d.\n", nMCB, ret);
        sz_szInfo =tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"No error when calling EDAQ_CB_logic_reset(3) by HMCB%d. The return value is %d.\n", nMCB, ret);
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo, gLogLevel1);
    }
    /////////////////////////////////////////////////////////////////////////
    ret = EDAQ_RB_working_mode_set(0, nMCB);		//设置RB缓存工作模式. 1: CMD mode. 0: Auto mode.
    if(ret > 0)
    {
        nLastError++;
        memset(tmp,0,512);
        sprintf(tmp,"Error when calling EDAQ_RB_working_mode_set(0) by HMCB%d. The return value is %d.\n", nMCB, ret);
        sz_szInfo = tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"No error when calling EDAQ_RB_working_mode_set(0) by HMCB%d. The return value is %d.\n", nMCB, ret);
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo, gLogLevel1);
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT32S db_fpga_cfg_file_len_in_byte = 0;
    //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据
    ETH_CTRL_read_buffer_residual_data(nMCB);
    //---------------------DB FPGA program part 1 start---------------------------------------
    // 读取DB FPGA编程文件中的数据，将其写入到HMCB的DDR缓存中
    //db_fpga_cfg_file_len_in_byte = EDAQ_read_and_write_DB_fpga_program_file_into_RB_via_ECS("E:\\work\\001_Container inspection system\\MainControlBox\\HMCB\\HMCB201804_V5.2\\App\\CB_DAQ_V5.2\\DB_fpga_top.bin");
    db_fpga_cfg_file_len_in_byte = EDAQ_read_and_write_DB_fpga_program_file_into_RB_via_ECS(pFPGAConfFile, nMCB);
    delete[] pFPGAConfFile;
    pFPGAConfFile = NULL;
    //
    if(db_fpga_cfg_file_len_in_byte < 0)
    {
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        sz_szInfo = "The DB FPGA configuration file doesn't exist!\n";
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        return false;
    }
    //
    //持续从ETH读取数据，直到清空缓存中的数据
    //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据
    ETH_CTRL_read_buffer_residual_data(nMCB);
    //设置DB FPGA编程文件长度
    ret = EDAQ_set_DB_FPGA_CFG_data_byte_num(db_fpga_cfg_file_len_in_byte, nMCB);
    if(ret > 0)
    {
        nLastError++;
        memset(tmp,0,512);
        sprintf(tmp,"Error when calling EDAQ_set_DB_FPGA_CFG_data_byte_num() by HMCB%d. The return value is %d.\n", nMCB, ret);
        sz_szInfo =tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"No error when calling EDAQ_set_DB_FPGA_CFG_data_byte_num() by HMCB%d. The return value is %d.\n", nMCB, ret);
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo, gLogLevel1);
    }
    usleep(10*1000);
    //EDAQ_read_DB_fpga_program_data_from_RB_via_ETH_and_dump_into_file();

    //选择使能部分或全部DB Line，允许向其烧写DB FPGA程序
    //EDAQ_DB_Line_FPGA_program_enable
    //功能：使能全部或部分DB Line的DB FPGA程序烧写信号
    //	通过多次调用本函数，选择部分DB Line进行FPGA程序烧写，可以实现向不同DB Line写入不同的DB FPGA程序。
    //参数：
    //	enable_x4: 4 bits.
    //			b3 = 1：允许向DB Line #3的DB FPGA烧写程序
    //			b2 = 1：允许向DB Line #2的DB FPGA烧写程序
    //			b1 = 1：允许向DB Line #1的DB FPGA烧写程序
    //			b0 = 1：允许向DB Line #0的DB FPGA烧写程序
    //返回值：无
    //注意：
    //	要向某个或某些DB Line的DB FPGA烧写程序，除了调用本函数使能相应的DB Line，还必须调用EDAQ_DB_FPGA_cfg_enable(1)函数。
    //EDAQ_DB_Line_FPGA_program_enable(m_nDBLine_FPGAFileConfigFlag[i], nMCB);
    //ret = EDAQ_DB_Line_FPGA_program_enable(15, nMCB);
    //只使能了DB Line #0
    ret = EDAQ_DB_Line_FPGA_program_enable(1, nMCB);
    if(ret > 0)
    {
        nLastError++;
        memset(tmp,0,512);
        sprintf(tmp, "Error when calling EDAQ_DB_Line_FPGA_program_enable(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
        sz_szInfo = tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"No error when calling EDAQ_DB_Line_FPGA_program_enable(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo, gLogLevel1);
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据
    ETH_CTRL_read_buffer_residual_data(nMCB);
    //请求开始向DB FPGA烧写程序
    EDAQ_DB_fpga_config_request(nMCB);
    //
    while (EDAQ_DB_FPGA_cfg_finished_flag_rd(nMCB) == 0)
    {
        loop_delay();
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据
    ETH_CTRL_read_buffer_residual_data(nMCB);
    //每次完成DB FPGA的程序烧写，必须复位HMCB logic。从而，为后续FPGA程序烧写或数据采集做准备。
    ret = EDAQ_CB_logic_reset(nMCB);
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    Sleep(5);
#else
    //usleep(5*1000);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
#endif
    if(ret > 0)
    {
        nLastError++;
        memset(tmp,0,512);
        sprintf(tmp, "Error when calling EDAQ_CB_logic_reset(5) by HMCB%d. The return value is %d.\n", nMCB, ret);
        sz_szInfo = tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"No error when calling EDAQ_CB_logic_reset(5) by HMCB%d. The return value is %d.\n", nMCB, ret);
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //printf("The DB FPGA has been programmed.\n");
    //sz_szInfo.Format(("The DB FPGA has been programmed.\n")) ;
    //std::cout <<(sz_szInfo);
    //WriteLogFile(sz_szInfo);
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Config the control bus between CB and DB to be used as communication bus
    EDAQ_DB_FPGA_cfg_enable(0, nMCB);
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Reset RBIB and RBOB to prepare for data acquisition.
    ret = EDAQ_RB_wr_port_sel(EDAQ_RB_WR_SEL_NO_DATA, nMCB); //Stop writing RBIB.
    if(ret > 0)
    {
        nLastError++;
        memset(tmp,0,512);
        sprintf(tmp,"Error when calling EDAQ_RB_wr_port_sel(EDAQ_RB_WR_SEL_NO_DATA,...) by HMCB%d. The return value is %d.\n", nMCB, ret);
        sz_szInfo = tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"No error when calling EDAQ_RB_wr_port_sel(EDAQ_RB_WR_SEL_NO_DATA,...) by HMCB%d. The return value is %d.\n", nMCB, ret);
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ret = EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_NO_DATA, nMCB); //Stop reading RBOB.
    if(ret > 0)
    {
        nLastError++;
        memset(tmp,0,512);
        sprintf(tmp, "Error when calling EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_NO_DATA,...) by HMCB%d. The return value is %d.\n", nMCB, ret);
        sz_szInfo =tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"No error when calling EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_NO_DATA,...) by HMCB%d. The return value is %d.\n", nMCB, ret);
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    EDAQ_RBxB_reset(nMCB);		//reset RBIB AND RBOB
    //EDAQ_RB_wr_port_sel(EDAQ_RB_WR_SEL_DB_RX_DATA);
    //EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_ETH_DATA);
    /////////////////////////////////////////////////////////////////////////
    //---------------------DB FPGA program part 1 end---------------------------------------
    usleep(10*1000);

    if(0 == nLastError)
    {
        memset(tmp,0,512);
        sprintf(tmp, "HMCB%d completes DB FPGA configuration.\n", i) ;
        sz_szInfo =tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        /////////////////////////////////////////////////////////////////////////
        m_timehot[nMCB] = NucTool::GetTickCount();
        //
        m_MCBReadySts[nMCB] |=0x02;
        m_MCBReadySts[nMCB] |=0x04;
        //
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_ADFPGACFG_OK);
        //
        /////////////////////////////////
        return true;
        /////////////////////////////////////////////////////////////////////////
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"HMCB(4)%d DB FPGA configuration failed.\n", nMCB);
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
        return false;
    }

}

void CMCBCtrlC::SendMCBMsg(UINT msg, WPARAM wParam, LPARAM lParam)
{
    std::string sz_szInfo = ("");
    std::string str_Msg = ("");
    std::string str_wParam = ("");
    std::string str_lParam = ("");

    char tmp[512] = {0};

    if (gCallBack)
    {
        //把消息码及参数转换消息字符串及参数字符串
        Conv_SendAppMsg_code_to_String(msg, str_Msg);
        Conv_SendAppMsg_wParam_to_String(wParam, str_wParam);
        Conv_SendAppMsg_lParam_to_String(wParam, lParam, str_lParam);
        //
        memset(tmp,0,512);
        sprintf(tmp,"HMCB%d Send Message to APP: msg=0x%08x(%s), wParam=0x%08x(%s), lParam=0x%08x(%s).\n", (int)(lParam)>>16, msg, str_Msg.c_str(), wParam, str_wParam.c_str(), lParam, str_lParam.c_str());
        sz_szInfo = tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        (*gCallBack)(msg, wParam, lParam);
        memset(tmp,0,512);
        sprintf(tmp,"HMCB%d Send Message to APP OK.\n", (int)(lParam)>>16);
        sz_szInfo = tmp;
        WriteLogFile(sz_szInfo);
    }
}

//把消息码及参数转换消息字符串及参数字符串
void CMCBCtrlC::Conv_SendAppMsg_code_to_String(UINT msg_code, std::string& str)
{
    switch(msg_code)
    {
    case MCB_MSG_STATUS:
        str = "MCB_MSG_STATUS";
        break;
    case MCB_MSG_DATA:
        str="MCB_MSG_DATA";
        break;
    default:
        str="Unknown Message code";
        break;
    }
}
void CMCBCtrlC::Conv_SendAppMsg_wParam_to_String(WPARAM msg_wParam, std::string& str)
{
    switch(msg_wParam)
    {
    case MCB_DVSTS:
        str="MCB_DVSTS";
        break;
    case MCB_DVREADY:
        str="MCB_DVREADY";
        break;
    case MCB_DVERR:
        str="MCB_DVERR";
        break;
    case MCB_STARTERR:
        str="MCB_STARTERR";
        break;
    case MCB_STOPERR:
        str="MCB_STOPERR";
        break;
    case MCB_STOPOK:
        str="MCB_STOPOK";
        break;
    default:
        str="Unknown Message wParam";
        break;
    }
}

void CMCBCtrlC::Conv_SendAppMsg_lParam_to_String(WPARAM msg_wParam, LPARAM msg_lparam, std::string& str)
{
    int ncode = (int)(msg_lparam);
    char tmp[512] = {0};
    if(MCB_DVSTS == msg_wParam)
    {
        switch(ncode&0xffff)
        {
        case MCB_STSCD_CONNECTING:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d connecting", ncode >>16) ;
            str = tmp;
            break;
        case MCB_STSCD_DATACONNECTOK:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Data port connection established", ncode >>16) ;
            str = tmp;
            m_MCBReadySts[ncode>>16] |=0x01;
            break;
        case MCB_STSCD_CTRLCONNECTOK:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Control port connection established", ncode >>16) ;
            str = tmp;
            m_MCBReadySts[ncode>>16] |=0x01;
            break;
        case MCB_STSCD_CONNECTCLOSE:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d connection closed", ncode >>16) ;
            str = tmp;
            break;
        case MCB_STSCD_CONNECTFAIL:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d connection fail", ncode >>16) ;
            str = tmp;
            break;
        case MCB_STSCD_SOCKETFAIL:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d socket fail", ncode >>16) ;
            str = tmp;
            break;
        case MCB_STSCD_MBFPGACFG_OK:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d MB fpga configure OK", ncode >>16);
            str = tmp;
            m_MCBReadySts[ncode>>16] |=0x02;
            break;
        case MCB_STSCD_ADFPGACFG_OK:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d ADMB fpga configure OK", ncode >>16);
            str= tmp;
            m_MCBReadySts[ncode>>16] |=0x04;
            break;
        case MCB_STSCD_REGSET_OK:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Register Set OK", ncode >>16);
            str = tmp;
            m_MCBReadySts[ncode>>16] |=0x08;
            break;
        case MCB_STSCD_FPGAFILE_ERR:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d fpga configure file Error", ncode >>16);
            str = tmp;
            break;
        case MCB_STSCD_MBFPGACFG_ERR:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d MB fpga configure Error", ncode >>16);
            str = tmp;
            break;
        case MCB_STSCD_REGSET_ERR:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Register Set Error", ncode >>16);
            str = tmp;
            break;
        case MCB_STSCD_STARTOK:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Start OK", ncode >>16);
            str = tmp;
            break;
        case MCB_STSCD_SETREGATRUN:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Receive 'set mode or register' cmd when MCB is running", ncode >>16);
            str = tmp;
            break;
        case MCB_STSCD_STARTRUN:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Receive 'start' cmd when MCB is running", ncode >>16);
            str = tmp;
            break;
        case MCB_STSCD_SETNEWIPRUN:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Receive 'set New IP' cmd when MCB is running", ncode >>16);
            str = tmp;
            break;
        case MCB_STSCD_SETNEWIP_ERR:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d 'set New IP' Error", ncode >>16);
            str = tmp;
            break;
        case MCB_STSCD_SETNEWIP_OK:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d 'set New IP' OK", ncode >>16);
            str = tmp;
            break;
        case MCB_STSCD_NOHOTPKTRUN:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Receive Hot packet overtime(running)", ncode >>16);
            str = tmp;
            break;
        case MCB_STSCD_NOHOTPKTSTOP:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Receive Hot packet overtime(stop)", ncode >>16);
            str = tmp;
            break;
        case MCB_STSCD_NODM:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d No Detector Module", ncode >>16);
            str = tmp;
            break;
        case MCB_STSCD_DMNUM_ERROR:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d detector module number parsed Error.", ncode >>16);
            str = tmp;
            break;
        case MCB_STSCD_DMOVERFLOW:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Detector module overflow", ncode >>16);
            str = tmp;
            break;
        case MCB_STSCD_NOTRIGGER:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d No Trigger source", ncode >>16);
            str = tmp;
            break;
        case MCB_STSCD_NODATA:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d No Data", ncode >>16);
            str = tmp;
            break;
        case MCB_STSCD_SENDERR:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Send Error", ncode >>16);
            str = tmp;
            break;
        case MCB_STSCD_SCNTPCNTDIFF:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Pulse Leak", ncode >>16);
            str= tmp;
            break;
        case MCB_STSCD_CANSELFTEST_OK:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d CAN selftest OK", ncode >>16);
            str = tmp;
            break;
        case MCB_STSCD_CANSELFTEST_ERR:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d CAN selftest ERR", ncode >>16);
            str= tmp;
            break;
        case MCB_STSCD_CHANGE_RANGE0:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d change range: group 0", (ncode >>16));
            str = tmp;
            break;
        case MCB_STSCD_CHANGE_RANGE1:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d change range: group 1", (ncode >>16));
            str = tmp;
            break;
        default:
            str =("Unknown MCB_DVSTS Message lParam");
        }
    }
    else if(MCB_DVERR == msg_wParam)
    {
        switch(ncode&0xffff)
        {
        case MCB_ERRCD_CONNECTCLOSE:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d connection closed.", ncode >>16) ;
            str = tmp;
            break;
        case MCB_ERRCD_CONNECTFAIL:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d connection fail.", ncode >>16) ;
            str = tmp;
            break;
        case MCB_ERRCD_SOCKETFAIL:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d socket fail.", ncode >>16) ;
            str = tmp;
            break;
        case MCB_ERRCD_FPGAFILE_ERR:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d fpga configure file Error.", ncode >>16);
            str = tmp;
            break;
        case MCB_ERRCD_MBFPGACFG_ERR:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d MB fpga configure Error.", ncode >>16);
            str= tmp;
            break;
        case MCB_ERRCD_REGSET_ERR:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Register Set Error.", ncode >>16);
            str = tmp;
            break;
        case MCB_ERRCD_DVNOREADY:
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Receive 'start, set mode or register' cmd when device not ready.", ncode >>16);
            str = tmp;
            break;
        default:
            str =("Unknown MCB_DVERR Message lParam.");
        }
    }
    else
    {
        str="Unknown Message lParam.";
    }
}

void CMCBCtrlC::CheckConnect()
{
    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    unsigned short i = 0;
    m_timenow = NucTool::GetTickCount();
    //
    INT8U ret = 0;
    //////////////////////////////////////////////////////////////////////////////////
    //如果处于初始化阶段，直接返回
    if(m_bMCBxInInitialing || m_bStart)
    {
        sz_szInfo = "Warning: HMCB in initialing or running process, can't check connect status!\n";
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo, gLogLevel3);
        //
        m_timehot[i] = NucTool::GetTickCount();
        return;
    }
   // if((m_timenow - m_timehot[i])> 10000) //10s没有活动
    if((m_timenow - m_timehot[i])> 5000) //5s没有活动
    {
        //如果处于数据采集阶段，直接返回
        //////////////////////////////////////////////////////////////////////////////////
        //如果处于空闲阶段，通过调用ECS_A32D32_read()判断控制板(服务端)是否处于Active状态
        for(i = 0; i < m_nMCBNum; i++)
        {
            m_timehot[i] = NucTool::GetTickCount();
            //EDAQ_read_and_dump_firmware_version();
            EDAQ_FIRMWARE_VERSION_REG reg;
            ret = ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR,&reg.All, i);
            memset(tmp,0,512);
            sprintf(tmp,"HMCB firmware version: %d.%d.%d (ret=%d).\n",reg.Sepe.major_ver,reg.Sepe.minor_ver,reg.Sepe.revision, ret);
            sz_szInfo = tmp;

            //  bool bConnect = checksocket_status(i);

            //
            if(0 == ret)
            {
                memset(tmp,0,512);
                sprintf(tmp,"HMCB%d firmware version can be read normally and is %d.%d.%d (ret=%d). The connection is active\n", i, reg.Sepe.major_ver,reg.Sepe.minor_ver,reg.Sepe.revision, ret);
                sz_szInfo = tmp;
                WriteLogFile(sz_szInfo, gLogLevel2);
            }
            //读取数据失败
            else//(0 != ret)
            {
                memset(tmp,0,512);
                sprintf(tmp,"Failed to read HMCB%d firmware version when check connect, and the connection is not active. Reinitialize it.\n", i);
                sz_szInfo = tmp;
                WriteLogFile(sz_szInfo);
                //重新初始化
                if(m_bEnMCBx4)
                {
                    InitialMCB_HMCB4(i);
                }
                else
                {

                    InitialMCB(i);
                }
            }

        }
    }
}

void CMCBCtrlC::CheckDVReadySts()
{
    /////////////////////////////////////////////////////
    unsigned short i = 0;
    std::string str = ("");
    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    ////////////////////////////////////////////////////
    sz_szInfo = "Enter CheckDVReadySts process.\n";
    WriteLogFile(sz_szInfo, gLogLevel6);
    //
    bool bIni = false;
    for (i =0; i< m_nMCBNum; i++)
    {
        bIni = bIni || m_bMCBxInInitialing;
    }
    if(bIni)
    {
        sz_szInfo = "In the CheckDVReadySts process, but HMCB is in initialization........\n";
        WriteLogFile(sz_szInfo, gLogLevel2);
        return;
    }
    /////////////////////////////////////
    bool b,b1[MAX_MCBNUM];
    //
    b = true;
    for (i =0; i< m_nMCBNum; i++)
    {
        b1[i] = true;
    }
    /////////////////////////////////////
    for (i =0; i< m_nMCBNum; i++)
    {
        b1[i] =  ((m_MCBReadySts[i]&0x0f)==0x0f);//设备就绪状态
        b = b && ((m_MCBReadySts[i]&0x0f)==0x0f);//设备就绪状态
    }
    /////////////////////////////////////
    if((NucTool::GetTickCount()- m_timeInitSpan) > 10000)//10秒钟重新尝试初始化
    {
        for (i = 0; i< m_nMCBNum; i++)
        {
            //if (!b1[i])
            if ((!b1[i]) && ((m_MCBReadySts[i]&0xF0) > 0))//初始化过程失败(建立socket失败(0x10)，模块自检测失败(0x80)
            {
                memset(tmp,0,512);
                sprintf(tmp,"HMCB%d not ready!!! Repeat Initial it again.\n", i);
                sz_szInfo = tmp;
                WriteLogFile(sz_szInfo);
                //设备未就绪，10秒钟重新尝试初始化
                if(m_bEnMCBx4)
                {
                    InitialMCB_HMCB4(i);
                }
                else
                {

                    InitialMCB(i);
                }
            }
        }
    }
    /////////////////////////////////////
    //str.Format(("m_MCBReadySts[1-2]= 0x%02X 0x%02X........\n"), m_MCBReadySts[0], m_MCBReadySts[1]);
    //WriteLogFile(str);
    /////////////////////////////////////
    if (b) // DEVICE READY
    {
        /////////////////////////////////////
        if (!m_bDVReady)
        {
            /////////////////////////////////////////////////////////////////////////////
            sz_szInfo = ("HMCB ready!!! Core Firmware Version--");
            //
            for (i = 0; i < m_nMCBNum; i++)
            {
                memset(tmp,0,512);
                sprintf(tmp,"%s,\t", m_strMCBCoreFirmwareVer[i].c_str());

                sz_szInfo += tmp;
            }
            sz_szInfo += ("\n");
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
            //
            SendMCBMsg(MCB_MSG_STATUS, MCB_DVREADY, (LPARAM)m_nMCB_Version[0]);
            //SendMCBMsg(MCB_MSG_STATUS, MCB_DVREADY, 0);
            m_bDVReady = true;
        }
    }
    else
    {
        m_bDVReady = false;
    }
}

void CMCBCtrlC::UpdateNewTime()
{
    //std::cout <<(("System time change!\n"));
    WriteLogFile(("System time change!\n"));
    for (int i=0; i<m_nMCBNum; i++)
    {
        m_timehot[i] = NucTool::GetTickCount();
        m_timenodata[i] = NucTool::GetTickCount();
    }
    m_timeuncheck = NucTool::GetTickCount();
}

//2s elapse
void CMCBCtrlC::OnTimer()
{
    //boost::unique_lock<boost::mutex> lock(mutex1);
    std::string sz_szInfo = ("");
    ////////////////////////////////////////////////////
    sz_szInfo="Enter OnTimer process.\n";
    WriteLogFile(sz_szInfo, gLogLevel6);
    ////////////////////////////////////////////////////
    //10s在日志文件内记录一次心跳
    time_t t_st;
    //    struct tm * timeinfo = localtime(&t_st);
    //    if((m_nHeartbeatSec != timeinfo->tm_sec) && (0 == (abs(timeinfo->tm_sec - m_nHeartbeatSec) % 10)))
    //    {
    //        m_nHeartbeatSec = timeinfo->tm_sec;
    //        char tmp[512] = {0};
    //        sprintf(tmp,"HeartBeat %d.\n", (timeinfo->tm_hour*60*60 + timeinfo->tm_min*60 + timeinfo->tm_sec)/10);
    //        sz_szInfo = tmp;
    //		WriteLogFile(sz_szInfo, gLogLevel1);
    //	}
    ////////////////////////////////////////////////////
    CheckHMCBStatus();

}

void CMCBCtrlC::CheckHMCBStatus()
{
    std::string sz_szInfo = ("");
    ////////////////////////////////////////////////////
    //5s在检查一次连接状态
    //if (time(NULL) - m_timeuncheck >5)
    //if (GetTickCount() - m_timeuncheck > 5000) //GetTickCount():The return value is the number of milliseconds that have elapsed since the system was started
    //{
    CheckConnect();
    //}
    ////////////////////////////////////////////////////
    //配置了设备自动重连，则2s检查一次设备就绪状态
    if(m_bconnectauto)
    {
        CheckDVReadySts();
    }
    ////////////////////////////////////////////////////
    //如果已启动采集，检查设备状态
    if (m_bStart) //check start status
    {
        //已启动
        if (m_bPreStart)
        {
            CheckRunStatus();
        }
        if (m_bPreStop)
        {
            CheckStopSts();
        }
    }
    ////////////////////////////////////////////////////
    if (m_bPreStop)
    {
        CheckDataEnd();
    }
    //////////////////////////////////
    //设备启动采集，10秒内没有收到数据报 no data 无数据错误
    bool bNoData[MAX_MCBNUM] = {false};
    int i = 0;
    if (m_bStart) //check start status
    {
        for (i = 0; i < m_nMCBNum; i++)
        {
            //m_nWaitDataTrigger[i]++;
            ////////////////////////////////////////
            //if((m_nWaitDataTrigger[i] > 5) && (0 == (m_nWaitDataTrigger[i] % 5)))
            if((NucTool::GetTickCount() - m_timenodata[i]) >10000)
            {
                m_timenodata[i] = NucTool::GetTickCount();
                //
                char tmp[512] = {0};
                sprintf(tmp,"HMCB%d : no data(start)!\n", i);
                sz_szInfo= tmp;
                //std::cout <<(sz_szInfo);
                WriteLogFile(sz_szInfo);
                std::thread th([i]{

                    std::this_thread::sleep_for(std::chrono::milliseconds(2*1000));
                    if((NucTool::GetTickCount() - m_timenodata[i]) >2000)
                    {
                        std::string sz_szInfo = ("");
                        char tmp[512] = {0};
                        sprintf(tmp,"HMCB%d : diagnose no data(start)!\n", i);
                        sz_szInfo= tmp;
                        //std::cout <<(sz_szInfo);
                        WriteLogFile(sz_szInfo);
                        SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(i))<<16) + MCB_STSCD_NODATA);
                        //数据采集是否成功完成标志:失败
                        m_bPreDAQSucessFlag[i] = false;
                        //
                    }
                });
                th.detach();
                bNoData[i] = true;
            }
        }
        for (i = 0; i < m_nMCBNum; i++)
        {
            if(bNoData[i])
            {
                ////////////////////////////////////////
                //执行X命令读取寄存器值
                //DumpAllMCBAllTheECSRegisters(1);
                DumpAllTheECSRegistersRelease(i, 1);
                bNoData[i] = false;
                m_timenodata[i] = NucTool::GetTickCount();
            }
        }
    }
}



void CMCBCtrlC::SetReg(unsigned short nMCB)
{
    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    //
    if (m_bStart)
    {
        sz_szInfo = ("error: MCB is running, can't set register!\n");
        //std::cout << sz_szInfo;
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) +MCB_STSCD_SETREGATRUN);
        WriteLogFile(sz_szInfo);
        return;
    }
    //
    if (m_pDataSocket[nMCB])
    {
        //sz_szInfo.Format(("HMCB No.%d Set Reg: CtrlA:0x%02x, POTMT:%d, POTMW:%d, INTT:%d, Trigger=%s.\n"),
        //	nMCB, m_reg.regCtrlA, m_reg.regPOTMT, m_reg.regPOTMW, m_reg.regINTT, ((0 == nMCB) && m_nTrig) == 0 ? ("0-inner"):("1-outer"));
        if(0 == nMCB)
        {
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Set Reg: POTMT=%dus, POTMW=%dus, INTT=%dus, DATARDYT=%dus, DDI-LatchT=%dus, Trigger=%s.\n",
                    nMCB, m_reg.regPOTMT, m_reg.regPOTMW, m_reg.regINTT, m_reg.regDataRdyT, m_reg.regDDILatchT, ((0 == nMCB) && m_nTrig) == 0 ? ("0-inner"):("1-outer"));
            sz_szInfo= tmp;
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Set Reg: POTMT=%dus, POTMW=%dus, INTT=%dus, DATARDYT=%dus, DDI-LatchT=%dus, Trigger=Cascade trigger.\n",
                    nMCB, m_reg.regPOTMT, m_reg.regPOTMW, m_reg.regINTT, m_reg.regDataRdyT, m_reg.regDDILatchT);
            sz_szInfo= tmp;
        }
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //
        InitCBWorkPara(nMCB);
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"HMCB%d not ready, can't set register!\n", nMCB);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVERR, ((long(nMCB))<<16) +MCB_ERRCD_DVNOREADY);
    }
}
//设置CB工作参数
INT8U CMCBCtrlC::InitCBWorkPara(unsigned short nMCB)
{
    //////////////////////////////////////////////////////////////////////////////////
    std::string sz_szInfo = "";
    char tmp[512] = {0};
    sprintf(tmp,"Start of Init. HMCB%d CB work parameters!\n", nMCB);
    sz_szInfo = tmp;
    //std::cout << sz_szInfo;
    WriteLogFile(sz_szInfo);
    //
    usleep(10*1000);
    //////////////////////////////////////////////////////////////////////////////////
    //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
    ETH_CTRL_read_buffer_residual_data(nMCB);
    //////////////////////////////////////////////////////

    //Unit: us
    //设置命令执行时间T1+T2
    EDAQ_daq_cmd_exct_time_set(m_reg.regINTT + m_reg.regDataRdyT, nMCB);//积分结束时刻与所有DB数据准备好时刻之间的间隔，经验值为480uS
    //EDAQ_daq_cmd_exct_time_set(m_reg.regINTT+480);//积分结束时刻与所有DB数据准备好时刻之间的间隔，经验值为480uS
    EDAQ_DB_integration_time_set(m_reg.regINTT, nMCB);//积分时间，范围为1uS～1S，设定单位为1uS。
    internal_trigger_period_set(m_reg.regPOTMT, nMCB);//内触发间隔
    internal_trigger_pulse_width_set(m_reg.regPOTMW, nMCB);//设置内部触发脉宽,默认16us
    //DDI_latch_latency_set(m_reg.regINTT+200, nMCB);//DDI锁存时刻:积分结束时刻与DDI锁存时刻之间的间隔，经验值为200uS
    DDI_latch_latency_set(m_reg.regDDILatchT, nMCB);//DDI锁存时刻:积分结束时刻与DDI锁存时刻之间的间隔，经验值为200uS
    //////////////////////////////////////////////////////////////////////////////////
    memset(tmp,0,512);
    sprintf(tmp,"End of Init. HMCB%d CB work parameters!\n", nMCB);
    sz_szInfo = tmp;
    //std::cout << sz_szInfo;
    WriteLogFile(sz_szInfo);
    //////////////////////////////////////////////////////////////////////////////////


    return 0;
}

//读取寄存器值范围: 0--部分读取 1--全部读取
void CMCBCtrlC::DumpAllMCBAllTheECSRegisters(UINT nLevel /* = 0 */)
{
    //////////////////////////////////////////////////////////////////////////////////
    for(int i = 0; i < m_nMCBNum; i++)
    {
        DumpAllTheECSRegistersRelease(i, nLevel);
    }
}

//when The setting value of HMCB%d DB num of DB-Line0 is equal to the parsed result of DB num of DB-Line0, return 0, else return > 0
//读取寄存器值范围:参数nLevel: 0--部分读取 1--全部读取
int CMCBCtrlC::DumpAllTheECSRegistersRelease(unsigned short nMCB, UINT nLevel /* = 0 */, UINT nFLag /*= 0*/)
{
    std::string sz_szInfo = ("");
    int i = 0;
    INT8U  ret = 0;
    //////////////////////////////////////////////////////////////////////////////////
    if(nLevel > 0)
    {
        char tmp[512] = {0};
        sprintf(tmp,"Dump all the HMCB%d ECS registers.\n", nMCB);
        sz_szInfo = tmp;
    }
    else
    {
        char tmp[512] = {0};
        sprintf(tmp,"Dump some of the HMCB%d ECS registers.\n", nMCB);
        sz_szInfo = tmp;
    }
    //std::cout << (sz_szInfo);
    WriteLogFile(sz_szInfo);
    //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
    ETH_CTRL_read_buffer_residual_data(nMCB);
    //////////////////////////////////////////////////////////////////////////////////
    char tmp[512] = {0};
    sprintf(tmp,"=================HMCB%d CB ECS Registers Information=(%d)================\n", nMCB, nFLag);
    sz_szInfo = tmp;
    //std::cout << (sz_szInfo);
    WriteLogFile(sz_szInfo);
    //////////////////////////////////////////////////////////////////////////////////
	//非初始化过程，全部直接返回
	//nFLag > 0 :首次初始化过程，读取记录寄存器值
	if(0 == nFLag)
	{
		if(!m_bDebug)
		{
			//2024-08-16修改非调试模式直接返回
			//////////////////////////////////////////////////////////////////////////////////
			memset(tmp,0,512);
			sprintf(tmp,"===================m_bDebug=%d =================================================\n",m_bDebug);
			sz_szInfo = tmp;
			//std::cout << (sz_szInfo);
			WriteLogFile(sz_szInfo);
			return ret;
		}
	}
    //////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////
    INT8U nFunRet = 0;
    INT32U data = 0;
    DB_NUM_SET_REG reg2;
    DB_NUM_STATUS_REG reg3;
    //////////////////////////////////////////////////////////////////////////////////
    if( 0 == nLevel)
    {
        //////////////////////////////////////////////////////////////////////////////////
        //EDAQ_read_and_dump_firmware_version();
        EDAQ_FIRMWARE_VERSION_REG reg;
        ret = ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR,&reg.All, nMCB);
        memset(tmp,0,512);
        sprintf(tmp,"HMCB firmware version: %d.%d.%d (ret=%d).\n",reg.Sepe.major_ver,reg.Sepe.minor_ver,reg.Sepe.revision, ret);
        sz_szInfo = tmp;
        //std::cout << (sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        ////DB daq req sel
        //EDAQ_daq_req_sel_dump();
        INT16U nDaq_req_sel = EDAQ_daq_req_sel_rd(nMCB);
        if(nDaq_req_sel == EDAQ_DAQ_TRIG_SEL_PERIOD_TRIG)
        {
            sz_szInfo = ("The DB DAQ trigger: periodic trigger!\n");
        }
        else if(nDaq_req_sel == EDAQ_DAQ_TRIG_SEL_BEAM_PULSE)
        {
            sz_szInfo = ("The DB DAQ trigger: external beam pulse!\n");
        }
        else if(nDaq_req_sel == EDAQ_DAQ_TRIG_SEL_CASCADE_TRIG)
        {
            sz_szInfo = ("The DB DAQ trigger: cascaded trigger!\n");
        }
        else
        {
            sz_szInfo = ("The DB DAQ trigger: no trigger!\n");
        }
        //std::cout << (sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据
        ETH_CTRL_read_buffer_residual_data(nMCB);
        ////DAQ cmd execute time set
        //EDAQ_daq_cmd_exct_time_dump();
        memset(tmp,0,512);
        sprintf(tmp,"The daq cmd execute time has been set to %d us.\n", EDAQ_daq_cmd_exct_time_rd(nMCB));
        sz_szInfo = tmp;
        //std::cout << (sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        ////non-DAQ cmd execute time set
        //EDAQ_non_daq_cmd_exct_time_dump();
        memset(tmp,0,512);
        sprintf(tmp,"The non-daq cmd execute time has been set to %d us.\n",EDAQ_non_daq_cmd_exct_time_rd(nMCB));
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        ////EDAQ DB integration time set
        //EDAQ_DB_integration_time_dump();
        memset(tmp,0,512);
        sprintf(tmp,"The db integration time has been set to %d us.\n", EDAQ_DB_integration_time_rd(nMCB));
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        ////Internal trigger setting
        //internal_trigger_period_dump();
        memset(tmp,0,512);
        sprintf(tmp,"The internal_trigger_period has been set to %d us.\n",internal_trigger_period_rd(nMCB));
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
        //internal_trigger_pulse_width_dump();
        memset(tmp,0,512);
        sprintf(tmp,"The internal trigger pulse width has been set to %d us.\n",internal_trigger_pulse_width_rd(nMCB));
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
        ETH_CTRL_read_buffer_residual_data(nMCB);
        //////////////////////////////////////////////////////////////////////////////////
        //DDI_latch_latency_dump();
        memset(tmp,0,512);
        sprintf(tmp,"DDI latch latency has been set to %d us.\n",DDI_latch_latency_rd(nMCB));
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
        //EDAQ_HMCB_work_mode_dump();
        if(EDAQ_HMCB_work_mode_rd(nMCB) == 1)
        {
            sz_szInfo = "HMCB work mode: master HMCB!\n";
        }
        else
        {
            sz_szInfo = "HMCB work mode: cascaded HMCB!\n";
        }
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);

        //////////////////////////////////////////////////////////////////////////////////
        //EDAQ_DB_line_num_dump();

        memset(tmp,0,512);
        sprintf(tmp,"The setting value of DB line number via ECS: %d\n",EDAQ_DB_line_num_rd(nMCB));
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
        //打印系统支持的DB Line数目的最大值
        //EDAQ_read_and_dump_DB_line_max_num();
        memset(tmp,0,512);
        sprintf(tmp,"The maximum value of DB line number supported by this module: %d\n",EDAQ_read_DB_line_max_num(nMCB));
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
        //打印实际使能的DB Line数目
        //EDAQ_read_and_dump_DB_line_enable_num();
        memset(tmp,0,512);
        sprintf(tmp,"The number of DB Lines enabled acutually: %d\n", EDAQ_read_DB_line_enable_num(nMCB));
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
        //
        //////////////////////////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据
        ETH_CTRL_read_buffer_residual_data(nMCB);
        //////////////////////////////////////////////////////////////////////////////////
        //EDAQ_read_and_dump_DB_line_enable_vector();
        DB_COMM_STATUS_REG reg1;
        INT8U vec;
        ECS_A32D32_read(DB_COMM_STATUS_REG_ADDR,&reg1.All, nMCB);
        vec = reg1.Sepe.DB_Line_enable_x4;
        memset(tmp,0,512);
        sprintf(tmp,"DB-Line[0..3] enabled flags: %d %d %d %d\n",(vec>>0)&0x1,(vec>>1)&0x1,(vec>>2)&0x1,(vec>>3)&0x1);
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
        ETH_CTRL_read_buffer_residual_data(nMCB);
        //////////////////////////////////////////////////////////////////////////////////
        //EDAQ_DB_num_setting_dump();
        //DB_NUM_SET_REG reg2;
        ECS_A32D32_read(DB_NUM_SET_REG_ADDR,&reg2.All, nMCB);
        memset(tmp,0,512);
        sprintf(tmp,"The setting value of DB num of DB-Line[0..3]: %d, %d, %d, %d\n",reg2.Sepe.DL0_DB_num,reg2.Sepe.DL1_DB_num,reg2.Sepe.DL2_DB_num,reg2.Sepe.DL3_DB_num);
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
        ETH_CTRL_read_buffer_residual_data(nMCB);
        //////////////////////////////////////////////////////////////////////////////////
        //EDAQ_read_and_dump_parsed_DB_num();
        //DB_NUM_STATUS_REG reg3;
        ECS_A32D32_read(DB_NUM_STATUS_REG_ADDR,&reg3.All, nMCB);
        memset(tmp,0,512);
        sprintf(tmp,"The parsed result of DB num of DB-Line[0..3]: %d, %d, %d, %d\n",reg3.Sepe.DL0_DB_num,reg3.Sepe.DL1_DB_num,reg3.Sepe.DL2_DB_num,reg3.Sepe.DL3_DB_num);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    else
    {
        //
        for(i = 0; i < EDAQ_REG_NUM; i++)
        {
            nFunRet = ECS_A32D32_read(EDAQ_REG_BADDR+i*4,&data, nMCB);
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d CB Reg[%d]: %s is 0x%08X.(Ret = %d, Addr=0x%08X))\n", nMCB, i,str_EDAQ_reg_name_list[i].c_str(),data, nFunRet, EDAQ_REG_BADDR+i*4);
            sz_szInfo= tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        //////////////////////////////////////////////////////////////////////////////////
        //EDAQ_read_and_dump_firmware_version();
        EDAQ_FIRMWARE_VERSION_REG reg;
        ret = ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR,&reg.All, nMCB);
        memset(tmp,0,512);
        sprintf(tmp,"HMCB firmware version: %d.%d.%d (ret=%d).\n",reg.Sepe.major_ver,reg.Sepe.minor_ver,reg.Sepe.revision, ret);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        //EDAQ_Board_ID_dump();
        //INT64U rd_ID;
        //rd_ID = EDAQ_EEPROM_ID_read(nMCB);
        //memset(tmp,0,512);
        //sprintf(tmp,"Board ID : 0x%016llX\n",rd_ID);
        INT16U Int16PIDdata[64] = { 0 };
        ret = EDAQ_EEPROM_ID_read(nMCB, Int16PIDdata);
        memset(tmp,0,512);
        sprintf(tmp,"HMCB%d Board ID: 0x%04X%04X%04X%04X.\n", nMCB, Int16PIDdata[3], Int16PIDdata[2], Int16PIDdata[1], Int16PIDdata[0]);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        //EDAQ_production_time_dump();
        //EEPROM中存储的板子生产时间
        //sz_szInfo = ("");
        //char time_str[30] = { 0x00 };
        //time_t t;
        //t = (time_t) EDAQ_production_time_read();
        //strftime(time_str, sizeof(time_str), "%Y/%m/%d %H:%M:%S",localtime(&t));
        //for(int i = 0; i < sizeof(time_str); i++)
        //{
        //	sz_szInfo += time_str[i];
        //}
        //sz_szInfo += ("\n");
        //std::cout <<(sz_szInfo);
        //WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        //EDAQ_read_and_dump_DB_CLK_FREQ();
        DB_COMM_STATUS_REG statusreg;
        ret = ECS_A32D32_read(DB_COMM_STATUS_REG_ADDR,&statusreg.All, nMCB);
        memset(tmp,0,512);
        sprintf(tmp,"DB CLK: %dMHz (ret=%d)\n",statusreg.Sepe.DB_CLK_FREQ*10, ret);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        //EDAQ_DB_FPGA_cfg_enable_dump();
        memset(tmp,0,512);
        sprintf(tmp,"DB_FPGA_cfg_enable = %d\n",EDAQ_DB_FPGA_cfg_enable_rd(nMCB));
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        //打印DB FPGA CFG文件长度
        //EDAQ_dump_DB_FPGA_CFG_data_byte_num();
        memset(tmp,0,512);
        sprintf(tmp,"DB FPGA CFG data len has been set to %d bytes.\n",EDAQ_read_DB_FPGA_CFG_data_byte_num(nMCB));
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        //DAQ mode enable
        //EDAQ_daq_mode_enable_dump();
        if(EDAQ_daq_mode_enable_rd(nMCB) == 1)
        {
            sz_szInfo = ("The DB DAQ mode has been enabled!\n");
        }
        else
        {
            sz_szInfo = ("The DB DAQ mode has been disabled!\n");
        }
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        ////DB daq req sel
        //EDAQ_daq_req_sel_dump();
        if(EDAQ_daq_req_sel_rd(nMCB) == EDAQ_DAQ_TRIG_SEL_PERIOD_TRIG)
        {
            sz_szInfo = ("The DB DAQ trigger: periodic trigger!\n");
        }
        else if(EDAQ_daq_req_sel_rd(nMCB) == EDAQ_DAQ_TRIG_SEL_BEAM_PULSE)
        {
            sz_szInfo = ("The DB DAQ trigger: external beam pulse!\n");
        }
        else if(EDAQ_daq_req_sel_rd(nMCB) == EDAQ_DAQ_TRIG_SEL_CASCADE_TRIG)
        {
            sz_szInfo = ("The DB DAQ trigger: cascaded trigger!\n");
        }
        else
        {
            sz_szInfo = ("The DB DAQ trigger: no trigger!\n");
        }
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        ////DAQ cmd execute time set
        //EDAQ_daq_cmd_exct_time_dump();
        memset(tmp,0,512);
        sprintf(tmp,"The daq cmd execute time has been set to %d us.\n", EDAQ_daq_cmd_exct_time_rd(nMCB));
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        ////non-DAQ cmd execute time set
        //EDAQ_non_daq_cmd_exct_time_dump();
        memset(tmp,0,512);
        sprintf(tmp,"The non-daq cmd execute time has been set to %d us.\n",EDAQ_non_daq_cmd_exct_time_rd(nMCB));
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        ////EDAQ DB integration time set
        //EDAQ_DB_integration_time_dump();
        memset(tmp,0,512);
        sprintf(tmp,"The db integration time has been set to %d us.\n", EDAQ_DB_integration_time_rd(nMCB));
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        ////Internal trigger setting
        //internal_trigger_period_dump();
        memset(tmp,0,512);
        sprintf(tmp,"The internal_trigger_period has been set to %d us.\n",internal_trigger_period_rd(nMCB));
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //internal_trigger_pulse_width_dump();
        memset(tmp,0,512);
        sprintf(tmp,"The internal trigger pulse width has been set to %d us.\n",internal_trigger_pulse_width_rd(nMCB));
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        ////
        //DDI_latch_latency_dump();
        memset(tmp,0,512);
        sprintf(tmp,"DDI latch latency has been set to %d us.\n",DDI_latch_latency_rd(nMCB));
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        ////RB write port set
        //EDAQ_RB_wr_port_dump();
        INT32U port = 0;
        port = EDAQ_RB_wr_port_rd(nMCB);
        switch(port)
        {
        case EDAQ_RB_WR_SEL_NO_DATA:
            sz_szInfo="RB write port selection: No data.\n";
            break;
        case EDAQ_RB_WR_SEL_ECS_FPGA_CFG_DATA:
            sz_szInfo="RB write port selection: DB FPGA config data written from ECS bus!\n";
            break;
        case EDAQ_RB_WR_SEL_DB_RX_DATA:
            sz_szInfo="RB write port selection: DB daq data received from DB!\n";
            break;
        case EDAQ_RB_WR_SEL_INT_GEN_DATA:
            sz_szInfo="RB write port selection: Counting data generated inside EDAQ FPGA!\n";
            break;
        case EDAQ_RB_WR_SEL_ETH_FPGA_CFG_DATA:
            sz_szInfo="RB write port selection: DB FPGA config data received from ETH data link!\n";
            break;
        default:
            sz_szInfo="RB write port selection: unknownd port!\n";
            break;
        }
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        ////RB read port set
        //EDAQ_RB_rd_port_dump();
        port = 0;
        port = EDAQ_RB_rd_port_rd(nMCB);
        switch(port)
        {
        case EDAQ_RB_RD_SEL_NO_DATA:
            sz_szInfo="RB read port selection: No data.\n";
            break;
        case EDAQ_RB_RD_SEL_FPGA_CFG_DATA:
            sz_szInfo="RB read port selection: Read data is used to config DB FPGA!\n";
            break;
        case EDAQ_RB_RD_SEL_ETH_DATA:
            sz_szInfo="RB read port selection: Read data is sent to ETH data transmission link!\n";
            break;
        default:
            sz_szInfo="RB read port selection: unknownd port!\n";
            break;
        }
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        ////EDAQ FIFO error status dump
        //EDAQ_FIFO_error_status_dump();
        memset(tmp,0,512);
        sprintf(tmp,"EDAQ FIFO status: 0x%X\n", EDAQ_FIFO_error_status_rd(nMCB));
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        ////DB fpga config finished flag read
        //EDAQ_DB_FPGA_cfg_finished_flag_dump();
        memset(tmp,0,512);
        sprintf(tmp,"DB_FPGA_cfg_finished = %d\n",EDAQ_DB_FPGA_cfg_finished_flag_rd(nMCB));
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        ////DB communication cmd processing busy flag read
        //EDAQ_DB_comm_cmd_process_busy_flag_dump();
        memset(tmp,0,512);
        sprintf(tmp,"DB_comm_cmd_process_busy = %d\n",EDAQ_DB_comm_cmd_process_busy_flag_rd(nMCB));
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        ////DB RB almost full flag read
        //EDAQ_RB_almost_full_flag_dump();
        memset(tmp,0,512);
        sprintf(tmp,"RB_almost_full_flag = %d\n",EDAQ_RB_almost_full_flag_rd(nMCB));
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        //EDAQ_HMCB_work_mode_dump();
        if(EDAQ_HMCB_work_mode_rd(nMCB) == 1)
        {
            sz_szInfo="HMCB work mode: master HMCB!\n";
        }
        else
        {
            sz_szInfo="HMCB work mode: cascaded HMCB!\n";
        }
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        //EDAQ_DB_line_num_dump();
        memset(tmp,0,512);
        sprintf(tmp,"The setting value of DB line number via ECS: %d\n",EDAQ_DB_line_num_rd(nMCB));
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //打印系统支持的DB Line数目的最大值
        //EDAQ_read_and_dump_DB_line_max_num();
        memset(tmp,0,512);
        sprintf(tmp,"The maximum value of DB line number supported by this module: %d\n",EDAQ_read_DB_line_max_num(nMCB));
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //打印实际使能的DB Line数目
        //EDAQ_read_and_dump_DB_line_enable_num();
        memset(tmp,0,512);
        sprintf(tmp,"The number of DB Lines enabled acutually: %d\n", EDAQ_read_DB_line_enable_num(nMCB));
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //
        //EDAQ_read_and_dump_DB_line_enable_vector();
        DB_COMM_STATUS_REG reg1;
        INT8U vec;
        ECS_A32D32_read(DB_COMM_STATUS_REG_ADDR,&reg1.All, nMCB);
        vec = reg1.Sepe.DB_Line_enable_x4;
        memset(tmp,0,512);
        sprintf(tmp,"DB-Line[0..3] enabled flags: %d %d %d %d\n",(vec>>0)&0x1,(vec>>1)&0x1,(vec>>2)&0x1,(vec>>3)&0x1);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        //EDAQ_DB_num_setting_dump();
        //DB_NUM_SET_REG reg2;
        ECS_A32D32_read(DB_NUM_SET_REG_ADDR,&reg2.All, nMCB);
        memset(tmp,0,512);
        sprintf(tmp,"The setting value of DB num of DB-Line[0..3]: %d, %d, %d, %d\n",reg2.Sepe.DL0_DB_num,reg2.Sepe.DL1_DB_num,reg2.Sepe.DL2_DB_num,reg2.Sepe.DL3_DB_num);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        //EDAQ_read_and_dump_parsed_DB_num();
        //DB_NUM_STATUS_REG reg3;
        ECS_A32D32_read(DB_NUM_STATUS_REG_ADDR,&reg3.All, nMCB);
        memset(tmp,0,512);
        sprintf(tmp,"The parsed result of DB num of DB-Line[0..3]: %d, %d, %d, %d\n",reg3.Sepe.DL0_DB_num,reg3.Sepe.DL1_DB_num,reg3.Sepe.DL2_DB_num,reg3.Sepe.DL3_DB_num);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    //////////////////////////////////////////////////////////////////////////////////
    sz_szInfo = "====================================================================\n";
    //std::cout <<(sz_szInfo);
    WriteLogFile(sz_szInfo);
    //////////////////////////////////////////////////////////////////////////////////

    memset(tmp,0,512);
    sprintf(tmp,"HMCB%d DB num: the software setting value are (%d, %d, %d, %d), the CB saving value are (%d, %d, %d, %d), and the parsed result are  (%d, %d, %d, %d).\n",
            nMCB, m_DMInfo[nMCB].nDBDMNUM[3], m_DMInfo[nMCB].nDBDMNUM[2], m_DMInfo[nMCB].nDBDMNUM[1], m_DMInfo[nMCB].nDBDMNUM[0],
            reg2.Sepe.DL3_DB_num, reg2.Sepe.DL2_DB_num, reg2.Sepe.DL1_DB_num, reg2.Sepe.DL0_DB_num,
            reg3.Sepe.DL3_DB_num, reg3.Sepe.DL2_DB_num, reg3.Sepe.DL1_DB_num, reg3.Sepe.DL0_DB_num);
    sz_szInfo = tmp;
    //std::cout << sz_szInfo;
    WriteLogFile(sz_szInfo);
    //////////////////////////////////////////////////////////////////////////////////
    int nRet = 0;
    if((reg2.Sepe.DL3_DB_num != reg3.Sepe.DL3_DB_num) || (reg2.Sepe.DL2_DB_num != reg3.Sepe.DL2_DB_num) || (reg2.Sepe.DL1_DB_num != reg3.Sepe.DL1_DB_num) || (reg2.Sepe.DL0_DB_num != reg3.Sepe.DL0_DB_num))
    {
        nRet = 1;
        //////////////////////////////////////////////////////////////////////////////////
        memset(tmp,0,512);
        sprintf(tmp,"The setting value of HMCB%d DB num are of DB-Line0 is not equal to the parsed result or the CB saving value of DB num of DB-Line0.\n", nMCB);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //SendMCBMsg(MCB_MSG_STATUS, MCB_DVERR, ((long(nMCB))<<16) + MCB_ERRCD_DMNUM_ERROR);
    }
    //////////////////////////////////////////////////////////////////////////////////
    //set auto get DM number
    //if(m_bAutoDMNum)
    //{
    //	if(m_DMInfo[nMCB].nDMNum != reg3.Sepe.DL0_DB_num)
    //	{
    //		//////////////////////////////////////////////////////////////////////////////////
    //		sz_szInfo.Format(("The setting value of HMCB%d DB num of DB-Line0 is not equal to the parsed result of DB num of DB-Line0.\n"), nMCB);
    //		std::cout <<(sz_szInfo);
    //		WriteLogFile(sz_szInfo);
    //		//////////////////////////////////////////////////////////////////////////////////
    //		sz_szInfo.Format(("Reset the value of HMCB%d DB num of DB-Line0 as the parsed result of DB num of DB-Line0.\n"), nMCB);
    //		std::cout <<(sz_szInfo);
    //		WriteLogFile(sz_szInfo);
    //		//////////////////////////////////////////////////////////////////////////////////
    //		m_DMInfo[nMCB].nDMNum = reg3.Sepe.DL0_DB_num;
    //		SetDMCmdReg(nMCB);
    //	}
    //	else
    //	{
    //		sz_szInfo.Format(("The setting value of HMCB%d DB num of DB-Line0 is equal to the parsed result of DB num of DB-Line0.\n"), nMCB);
    //		std::cout <<(sz_szInfo);
    //		WriteLogFile(sz_szInfo);
    //	}
    //}
    //////////////////////////////////////////////////////////////////////////////////
    //
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    Sleep(10);
#else
    //usleep(10*1000);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
    //
    return nRet;
}
//

bool CMCBCtrlC::DAQFailToReset()
{
    std::string sz_szInfo = "";
    char tmp[512] = {0};
    unsigned short i = 0;
    unsigned short j = 0;
    INT8U  ret = 0;
    ///////////////////////////////////////////////////////////////
    //当上一次数据采集没有成功完成时复位CB端逻辑、重发DB端参数
    for(i = 0; i < m_nMCBNum; i++)
    {
        if(!m_bPreDAQSucessFlag[i])
        {
            memset(tmp,0,512);
            sprintf(tmp,"The previous data collection process of HMCB%d failed, and then re-build socket, re-set CB logic, and re-send DB parameters.\n", i);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
            //////////////////////////////////////////////////////////////////////////////////
            //持续从_ETH_CTRL_socket读取数据，直到清空缓存数据。
            ETH_CTRL_read_buffer_residual_data(i);
            usleep(10);
            ////////////////////////////////////////////////////////////////////
            //            ret = ResetCBLogicBeforeStart(i);				// 正常启动这一步可以重新设置，也可以不重新设置
            //            if(ret > 0)
            //            {
            //                return 0;
            //            }
        }
    }
    ///////////////////////////////////////////////////////////////
    //主HMCB模块，调用EDAQ_daq_mode_enable(0)，确保不处于数据采集模式
    for(i = 0; i < m_nMCBNum; i++)
    {
        ////////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据
        int iResult = ETH_CTRL_read_buffer_residual_data(i);
        //
        if ( iResult > 0 )
        {
            memset(tmp,0,512);
            sprintf(tmp,"When read out all residual data from ETH_CTRL buffer: %d Bytes received.\n", iResult);
            sz_szInfo = tmp;
        }
        else if ( iResult == 0 )
        {
            sz_szInfo = "When read out all residual data from ETH_CTRL buffer: Connection closed.\n";
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"When read out all residual data from ETH_CTRL buffer: The return value of WSAGetLastError() is %d.\n", errno);
            sz_szInfo = tmp;
        }
        //
        WriteLogFile(sz_szInfo, gLogLevel1);
        //
        usleep(10*1000);
        ////////////////////////////////////////////////////////////////
        ret = EDAQ_daq_mode_enable(0, i);
        if(ret != 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Received [start] CMD, error calling EDAQ_daq_mode_enable(0).The return value is %d.\n", i, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo ;
            WriteLogFile(sz_szInfo);
            //
            m_bPreDAQSucessFlag[i] = false;
            //
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Received [start] CMD, no error calling EDAQ_daq_mode_enable(0).The return value is %d.\n", i, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo, gLogLevel1);
        }
    }
    ///////////////////////////////////////////////////////////////
    //包括SetReg(i)和InitCBWorkPara
    //SetTrig(m_nTrig);
    //SetTrigCtrlReg(m_nTrig);
    ///////////////////////////////////////////////////////////////
    ////复位上一次数据采集是否成功标志
    //for(i = 0; i < m_nMCBNum; i++)
    //{
    //	m_bPreDAQSucessFlag[i] = false;
    //}
    ////////////////////////////////////////////////////////////
    //
    for(i =0; i< m_nMCBNum; i++)
    {

        if (!m_pDataSocket[i]) // check connection status
        {
            m_bStart =false;
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d started data collection, but the data socket was not established properly! FAIL TO START!\n", i);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
            SendMCBMsg(MCB_MSG_STATUS, MCB_DVERR, ((long(i))<<16) +MCB_ERRCD_DVNOREADY);
            return 0;
        }
    }
    ///////////////////////////////////////////////////////////////
    unsigned short nMCB = 0;
    //如果只有一个控制盒，则直接启动采集
    //如果有2个或2以上控制盒，则启动从控制盒，在OnTime ->CheckRunStatus函数内再启动主控制盒
    if (1 == m_nMCBNum)  //only one MCB, start at once;
    {
        nMCB = 0;
        m_MCBRunSts[nMCB] &= 0xfe;
        /////////////////////////////
        //不需CheckRunStatus
        m_bPreStart = false;
        //
        std::cout << "Master HMCB start OK, begin running...\n";
        WriteLogFile("Master HMCB start OK, begin running...\n");
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_STARTOK);
        ///////////////////////////

        memset(tmp,0,512);
        sprintf(tmp,"HMCB No.%d start receive data.\n", nMCB);
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
        m_MCBRunSts[nMCB] |=0x01;
        ///////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据
        ETH_CTRL_read_buffer_residual_data(nMCB);
        //////////////////////////////////////////////////////////////
        //逻辑复位FPGA逻辑
        ret = EDAQ_CB_logic_reset(nMCB);
        usleep(5*1000);
        if(ret > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_CB_logic_reset(5) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"No error when calling EDAQ_CB_logic_reset(5) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo, gLogLevel1);
        }
        /////////////////////////////////////////////////////////////////
        //功能： 发出DB FPGA复位请求
        //返回值: 0 = No error, others = Find errors.
        //reset
        ret = EDAQ_DB_reset_req(nMCB);//重新复位 The parsed result of DB num of DB-Line[0..3]
        if(ret > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_DB_reset_req() by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"No error when calling EDAQ_DB_reset_req() by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo, gLogLevel1);
        }
        usleep(10*1000);
        /////////////////////////////////////////////////
        if((gLogLevel1 < m_nLogLevel) && (m_nLogLevel < gLogLevel4 ))
        {
            //执行X命令读取部分寄存器值
            DumpAllTheECSRegistersRelease(nMCB, 0);
        }
        else if( m_nLogLevel >= gLogLevel4)
        {
            //执行X命令读取全部寄存器值
            DumpAllTheECSRegistersRelease(nMCB, 1);
        }
        /////////////////////////////////////////////////////////////////
        //启动数据采集
        ret = EDAQ_DB_daq_cmd_tx_req(nMCB);
        if(0 != ret)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_DB_daq_cmd_tx_req() by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"No error when calling EDAQ_DB_daq_cmd_tx_req() by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo, gLogLevel1);
        }
        /////////////////////////////////////////////////////////////////
        //进入数据模式//CB进入到数据采集模式
        ret = EDAQ_daq_mode_enable(1, nMCB);//进入数据模式//CB进入到数据采集模式
        if(0 != ret)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_daq_mode_enable(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"No error when calling EDAQ_daq_mode_enable(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo, gLogLevel1);
        }
        /////////////////////////////////////////////////////////////////
    }
    else
    {
        //从控制盒是否已全部启动
        m_bSlaveStart = false;
        //start slave MCB first
        for(i = m_nMCBNum; i > 1; i--)
        {
            nMCB = i - 1;
            //
            m_MCBRunSts[nMCB] &=0xfe;
            ///////////////////////////////////////////////////////////////
            memset(tmp,0,512);
            sprintf(tmp,"Slave HMCB%d start OK, begin running...\n", nMCB);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
            //SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_STARTOK);
            ///////////////////////////////////////////////////////////////
            memset(tmp,0,512);
            sprintf(tmp,"Slave HMCB%d start receive data.\n", nMCB);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
            m_MCBRunSts[nMCB] |=0x01;
            ///////////////////////////////////////////////////////////////
            //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据
            ETH_CTRL_read_buffer_residual_data(nMCB);
            //////////////////////////////////////////////////////////////
            //逻辑复位FPGA逻辑
            ret = EDAQ_CB_logic_reset(nMCB);
            usleep(5*1000);
            if(ret > 0)
            {
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling EDAQ_CB_logic_reset(6) by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                //std::cout << sz_szInfo;
                WriteLogFile(sz_szInfo);
            }
            else
            {
                memset(tmp,0,512);
                sprintf(tmp,"No error when calling EDAQ_CB_logic_reset(6) by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                std::cout <<sz_szInfo;
                WriteLogFile(sz_szInfo, gLogLevel1);
            }
            /////////////////////////////////////////////////////////////////
            ////////////////////////////////////////////////////////////////
            //功能： 发出DB FPGA复位请求
            //返回值: 0 = No error, others = Find errors.
            //reset
            ret = EDAQ_DB_reset_req(nMCB);//重新置位 The parsed result of DB num of DB-Line[0..3]
            if(ret > 0)
            {
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling EDAQ_DB_reset_req() by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                //std::cout << sz_szInfo;
                WriteLogFile(sz_szInfo);
            }
            else
            {
                memset(tmp,0,512);
                sprintf(tmp,"No error when calling EDAQ_DB_reset_req() by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                std::cout  << sz_szInfo;
                WriteLogFile(sz_szInfo, gLogLevel1);
            }
            //Sleep(10);
            /////////////////////////////////////////////////////////////////
            if((gLogLevel1 < m_nLogLevel) && (m_nLogLevel < gLogLevel4 ))
            {
                //执行X指令读取部分寄存器值
                DumpAllTheECSRegistersRelease(nMCB, 0);
            }
            else if( m_nLogLevel >= gLogLevel4)
            {
                //执行X指令读取全部寄存器值
                DumpAllTheECSRegistersRelease(nMCB, 1);
            }
            /////////////////////////////////////////////////////////////////
            //启动数据采集
            ret = EDAQ_DB_daq_cmd_tx_req(nMCB);
            if(0 != ret)
            {
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling EDAQ_DB_daq_cmd_tx_req() by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                //std::cout << sz_szInfo;
                WriteLogFile(sz_szInfo);
            }
            else
            {
                memset(tmp,0,512);
                sprintf(tmp,"No error when calling EDAQ_DB_daq_cmd_tx_req() by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                //std::cout << sz_szInfo;
                WriteLogFile(sz_szInfo, gLogLevel1);
            }
            /////////////////////////////////////////////////////////////////
            //进入数据模式//CB进入到数据采集模式
            ret = EDAQ_daq_mode_enable(1, nMCB);
            if(0 != ret)
            {
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling EDAQ_daq_mode_enable(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                //std::cout << sz_szInfo;
                WriteLogFile(sz_szInfo);
            }
            else
            {
                memset(tmp,0,512);
                sprintf(tmp,"No error when calling EDAQ_daq_mode_enable(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                //std::cout << sz_szInfo;
                WriteLogFile(sz_szInfo, gLogLevel1);
            }
            /////////////////////////////////////////////////////////////////
            usleep(10*1000);
        }
    }
    ///////////////////////////////////////////////////////////////
    for(i = 0; i < m_nMCBNum; i++)
    {
        m_bPreDAQSucessFlag[i] = true;
    }
    /////////////////////////////////////////////////////////////
    sz_szInfo = "Exit HMCB resets CB logic flow before starting a new acquisition.\n";
    //std::cout << sz_szInfo;
    WriteLogFile(sz_szInfo);
    /////////////////////////////////////////////////////////////
    return true;
}

void CMCBCtrlC::Start(long lcounter)
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //多端口使能，则通过Start_HMCB4()函数启动采集
    if(m_bEnMCBx4)
    {
        Start_HMCB4(lcounter);
        return;
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::string sz_szInfo = "";
    char tmp[512] = {0};
    unsigned short i = 0;
    unsigned short j = 0;
    INT8U  ret = 0;
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//仅用于调试节点计
	m_lDebugCnt = 0;
    //收到[Start]命令次数
    m_nRecvStartCmdCnt++;
    //
    memset(tmp,0,512);
    sprintf(tmp,"HMCB.dll received [Start] CMD(%d), packet-number:%d...\n", m_nRecvStartCmdCnt, lcounter);
    sz_szInfo = tmp;
    //std::cout << sz_szInfo;
    WriteLogFile(sz_szInfo);
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if(m_bMCBxInInitialing)
    {
        sz_szInfo = "Warning: HMCB is Initialing, can't start now!\n";
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, MCB_STSCD_STARTRUN);
        return;
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if(m_bStart)
    {
        sz_szInfo = "Warning: HMCB is running, can't start again!\n";
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, MCB_STSCD_STARTRUN);
        return;
    }
    /*   ///////////////////////////////////////////////////////////////
    //主HMCB模块，调用EDAQ_daq_mode_enable(0)，确保不处于数据采集模式
    for(i = 0; i < m_nMCBNum; i++)
    {
        ////////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据
        int iResult = ETH_CTRL_read_buffer_residual_data(i);
        ////////////////////////////////////////////////////////////////
        ret = EDAQ_daq_mode_enable(0, i);
        if(ret != 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Received [start] CMD, error calling EDAQ_daq_mode_enable(0).The return value is %d.\n", i, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
            //
            m_bPreDAQSucessFlag[i] = false;
            //
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Received [start] CMD, no error calling EDAQ_daq_mode_enable(0).The return value is %d.\n", i, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo, gLogLevel1);
        }
        usleep(10*1000);
    }
 */   /////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////
    //复位上一次数据采集是否成功完成标志
    for(i = 0; i < m_nMCBNum; i++)
    {
        m_bPreDAQSucessFlag[i] = false;
    }
    ////////////////////////////////////////////////////////////
    //记录data socket原始数据文件
    if( m_nLogLevel >= 5)
    {
        //////////////////////////////////////////////////////
        time_t t_st;
        struct tm * timeinfo = localtime(&t_st);
        //关闭前面打开的文件
        if(m_bSocketRawDatafile)
        {
            m_SocketRawDatafile.close();
            m_bSocketRawDatafile = false;
        }
        //
        m_unSocketRawWrCnt = 0;//写入记录的总数据字节数
        //随时间形成的文件名
        memset(tmp,0,512);
        sprintf(tmp,"%s\\SocketRawData_%04d%02d%02d%02d%02d%02d.txt", m_szPath, 1900+timeinfo->tm_year, timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        m_SocketRawDatafilename = tmp;
        //创建打开新文件
        locale loc = locale::global(locale("")); //要打开的文件路径含中文，设置全局locale为本地环境
        //
        m_SocketRawDatafile.open(m_SocketRawDatafilename, std::ios::out| std::ios::trunc);
        if (m_SocketRawDatafile.is_open()) //
        {
            std::cout << "Open Raw data file error!\n";
            m_bSocketRawDatafile = false;
        }
        else
        {
            m_SocketRawDatafile.seekg(0,std::ios_base::end);
            m_bSocketRawDatafile = true;
        }
        //打开文件
        locale::global(loc);//回复全局locale</span>
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    m_bStart = true;
    m_bPreStop = false;
    m_bPreDataEnd = false;
	//已收到停止采集命令
	m_bRecStopCmd = false;
	//
    m_nSendAllRecorded = 0;//进入send all 模式日志是否记录 0 初始 1需要记录 2已记录

    //m_nMissAA55Cnt = 0;
    for(int i=0;i<m_nMCBNum;i++)
    {
        m_nMissAA55Cnt[i] =0;
    }
    //
    ///////////////////////////////////////////////////////////////
    ////一次采集需要获取的触发数据包数量
    //0:无限包数
    //>0:需要目标包数
    //ULONG m_lcounter = (lcounter==0) ? 0 : lcounter + INVALID_DATA_PACKET_NUM;
    m_lcounter = (lcounter==0) ? 0 : lcounter + INVALID_DATA_PACKET_NUM;
    //
    for(i =0; i< m_nMCBNum; i++)
    {
        ///////////////////////////////////////////////////////////////
        m_timenodata[i] = NucTool::GetTickCount();
        ///////////////////////////////////////////////////////////////

        m_nMissAA55Cnt[i] = 0;
        //
        m_MCBRunSts[i] &= 0xfb;
        m_scounter[i] = 0;  //scounter reset
        m_nNeedSendData[i] =0;
        m_MCBData[i].d_head.head_use.scounter = 0;
        memset(m_MCBData, 0x00, sizeof(m_MCBData));
        //
        memset(m_MCBDataBackup, 0x00, sizeof(m_MCBDataBackup));//前次数据备份
        //
        m_lPrePulseCnt[i] = 0;//前次正常解析数据
        //
        if (!m_pDataSocket[i]) // check connection status
        {
            m_bStart =false;
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d started data collection, but the data socket was not established properly! FAIL TO START!\n", i);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
            SendMCBMsg(MCB_MSG_STATUS, MCB_DVERR, ((long(i))<<16) +MCB_ERRCD_DVNOREADY);
            return;
        }
    }
    //
    m_timeprestart = NucTool::GetTickCount();
    //使能CheckRunStatus
    m_bPreStart = true;

    memset(m_uDetData, 0, sizeof(m_uDetData));
    memset(m_uDBParameter, 0, sizeof(m_uDBParameter));
    memset(m_uDBPulse, 0, sizeof(m_uDBPulse));
    /////////////////////////////////////////////////////////////
    for(i = 0; i < MAX_MCBNUM; i++)
    {
        parsed_pkg_n[i] = 0;//已经解析的数据包个数
        Update_pkg_n[i] = 0;//已经解析且发送App更新的数据包个数
        SC_1st_pkg_index[i] = 0;//每一个DB Line返回的第一个数据包
        //
        SC_1st_pkg_TriggCnt[i] = 0;//每一个DB Line返回的第一个数据包内触发计数
        //SC_Current_pkg_TriggCnt[i] = 0;//每一个DB Line返回的当前数据包内触发计数
        //
        SC_pre_pkg_index[i] = 0;//每一个DB Line返回的前一个数据包索引
        SC_current_pkg_index[i] = 0;//每一个DB Line返回的当前数据包索引
        ///////////////////////////////////////////////////////////////
        //是否已获取到开始的0xA5数据包
        m_bGet0xA5Packet[i] = false;
        //获取到0xA5数据包前已解析出的数据包
        m_nMiss0XA5PacketCnt[i] = 0;
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }
    ///////////////////////////////////////////////////////////////
    INT8U ADFormat_Bit = (m_nADBit > 16) ? 20 : 16; //AD转换位数
    //
    UINT nRowNum[MAX_MCBNUM] = { 0 };//每行/列笔数
    UINT nColNum[MAX_MCBNUM] = { 0 };//行列数
    UINT nDMNum[MAX_MCBNUM] = { 0 };//每个DBLine带的有效模块数
    for(i = 0; i < MAX_MCBNUM; i++)
    {
        nRowNum[i] = m_DMInfo[i].nRowNum;//每行/列笔数
        nColNum[i] = m_DMInfo[i].nColNum;//行列数
        nDMNum[i] = m_DMInfo[i].nDMNum;//每个DBLine带的有效模块
    }
    ///////////////////////////////////////////////////////////
    //
    INT32U OneDBSize = 0;
    //一个CBDataPackage的size = 6字节逻辑+12字节数据+1字节CRC
    INT32 CBDataPackSize = 6 + 12 + 1;
    //
    for(i = 0; i < m_nMCBNum; i++)
    {
        //一个DB包size = 6字节头+（2+通道数）个通道数据+1个CRC
        //2022-08-09改为一个DB包size = 6字节头+（模块通道数）个通道数据+1个CRC
        OneDBSize = 6 + (nRowNum[i]*nColNum[i])*ADFormat_Bit/8 + 1;
        //一个DB包size = 6字节头+（2+通道数）个通道数据+1个CRC
        //2022-08-09改为一个DB包size = 6字节头+（模块通道数）个通道数据+1个CRC
        OneDBSize = 6 + (nRowNum[i]*nColNum[i])*ADFormat_Bit/8 + 1;
        //一个DBline的数据size = 10个包头 + DB数量*一个DB包size + 一个CBDataPackage的size
        //SC数据包: 每个DB Line的DB和CB数据都单独构成1个SC数据包。
        //多个DB Line的SC数据包按照DB Line #0到#3的顺序依次发送到PC机。
        gbl_DBLine_Size[i][0] = 10 + nDMNum[i]*OneDBSize + CBDataPackSize;
        //
        glb_data_AccWrBytes[i] = 0;		//记录总接收数据字节数
//        glb_data_AccRdBytes[i] = 0;		//记录总读出处理字节数
//        gbl_data_buf_wraddr[i] = 0;		//记录数据缓存区下一个数据的写入位置
//        gbl_data_buf_rdaddr[i] = 0;		//记录下一个数据的读取地址
        gbl_sc_pkg_index[i] = -1;		//记录各个DB Line上一个sc pkg的包编号
    }
    ////////////////////////////////////////////////////////////
    //hMutex = CreateMutex(NULL, FALSE, NULL);
    ////////////////////////////////////////////////////////////
    //启动多线程准备数据解析
    //ReadParseData(m_lcounter);
    /////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////
    //for(i = 0; i < m_nMCBNum; i++)
    //{
    //	//持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
    //	ETH_CTRL_read_buffer_residual_data(i);
    //}
    ///////////////////////////////////////////////////////////////
    unsigned short nMCB = 0;
    //如果只有一个控制盒，则直接启动采集
    //如果有2个或2个以上控制盒，则先启动从控制盒，在OnTime ->CheckRunStatus函数内再启动主控制盒
    if (1 == m_nMCBNum)  //only one MCB, start at once;
    {
        nMCB = 0;
        m_MCBRunSts[nMCB] &= 0xfe;
        /////////////////////////////
        //不需CheckRunStatus
        m_bPreStart = false;
        //
        std::cout << "Master HMCB start OK, begin running...\n";
        WriteLogFile("Master HMCB start OK, begin running...\n");
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_STARTOK);
        ///////////////////////////
        ProcStartDaqCmd(nMCB);
        memset(tmp,0,512);
        sprintf(tmp,"HMCB No.%d start receive data.\n", nMCB);
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
        m_MCBRunSts[nMCB] |=0x01;
        ///////////////////////////////////////////////////////////////

        /*       //////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
        ETH_CTRL_read_buffer_residual_data(nMCB);
        /////////////////////////////////////////////////////////////////
        //启动数据采集
        //功能：发出DB DAQ命令传输请求
        ret = EDAQ_DB_daq_cmd_tx_req(nMCB);
        if(0 != ret)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_DB_daq_cmd_tx_req() by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"No error when calling EDAQ_DB_daq_cmd_tx_req() by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo, gLogLevel1);
        }
        /////////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
        ETH_CTRL_read_buffer_residual_data(nMCB);
        /////////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
        //选择触发方式
        ret = SetTrigCtrlReg(nMCB);
        if(0 != ret)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling SetTrigCtrlReg() by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"No error when calling SetTrigCtrlReg() by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo, gLogLevel1);
        }
        /////////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
        ETH_CTRL_read_buffer_residual_data(nMCB);
        /////////////////////////////////////////////////////////////////
        //进入取数模式//CB进入到数据采集模式
        ret = EDAQ_daq_mode_enable(1, nMCB);//进入取数模式//CB进入到数据采集模式
        if(0 != ret)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_daq_mode_enable(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"No error when calling EDAQ_daq_mode_enable(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo, gLogLevel1);
        }
 */       /////////////////////////////////////////////////////////////////
        //sz_szInfo.Format(_T("After EDAQ_daq_mode_enable(1, %d) and DumpAllTheECSRegistersRelease(%d, 1) .\n"), nMCB, nMCB);
        //WriteLogFile(sz_szInfo, gLogLevel1);
        //DumpAllTheECSRegistersRelease(nMCB, 1);
        //return;
    }
    else
    {
        //从控制盒是否已全部启动
        m_bSlaveStart = false;
        //start slave MCB first
        //for(i = m_nMCBNum; i > 1; i--)
        for(i=1; i<m_nMCBNum;i++)
        {
            //nMCB = i - 1;
            nMCB = i;
            //
            m_MCBRunSts[nMCB] &=0xfe;
            ///////////////////////////////////////////////////////////////
            memset(tmp,0,512);
            sprintf(tmp,"Slave HMCB%d start OK, begin running...\n", nMCB);
            sz_szInfo = tmp ;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
            SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_STARTOK);
            ///////////////////////////////////////////////////////////////
            ProcStartDaqCmd(nMCB);
            //////////////////////////////////////////////////////////////
            memset(tmp,0,512);
            sprintf(tmp,"Slave HMCB%d start receive data.\n", nMCB);
            sz_szInfo = tmp;
            //std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
            m_MCBRunSts[nMCB] |=0x01;
            ///////////////////////////////////////////////////////////////

            /*           //////////////////////////////////////////////////////////////
            //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
            ETH_CTRL_read_buffer_residual_data(nMCB);
            usleep(10);
            //////////////////////////////////////////////////////////////
            //启动数据采集
            ret = EDAQ_DB_daq_cmd_tx_req(nMCB);
            if(0 != ret)
            {
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling EDAQ_DB_daq_cmd_tx_req() by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                //std::cout << sz_szInfo;
                WriteLogFile(sz_szInfo);
            }
            else
            {
                memset(tmp,0,512);
                sprintf(tmp,"No error when calling EDAQ_DB_daq_cmd_tx_req() by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                //std::cout << sz_szInfo;
                WriteLogFile(sz_szInfo, gLogLevel1);
            }
            /////////////////////////////////////////////////////////////////
            //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
            //选择触发方式
            ret = SetTrigCtrlReg(nMCB);
            if(0 != ret)
            {
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling SetTrigCtrlReg() by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                //std::cout << sz_szInfo;
                WriteLogFile(sz_szInfo);
            }
            else
            {
                memset(tmp,0,512);
                sprintf(tmp,"No error when calling SetTrigCtrlReg() by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                //std::cout << sz_szInfo;
                WriteLogFile(sz_szInfo, gLogLevel1);
            }
            /////////////////////////////////////////////////////////////////
            //进入取数模式//CB进入到数据采集模式
            ret = EDAQ_daq_mode_enable(1, nMCB);
            if(0 != ret)
            {
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling EDAQ_daq_mode_enable(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                //std::cout << sz_szInfo;
                WriteLogFile(sz_szInfo);
            }
            else
            {
                memset(tmp,0,512);
                sprintf(tmp,"No error when calling EDAQ_daq_mode_enable(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                //std::cout << sz_szInfo;
                WriteLogFile(sz_szInfo, gLogLevel1);
            }
            /////////////////////////////////////////////////////////////////
            usleep(10*1000);
 */           /////////////////////////////////////////////////////////////////
            //sz_szInfo.Format(_T("After EDAQ_daq_mode_enable(1, %d) and DumpAllTheECSRegistersRelease(%d, 1) .\n"), nMCB, nMCB);
            //WriteLogFile(sz_szInfo, gLogLevel1);
            //DumpAllTheECSRegistersRelease(nMCB, 1);
            /////////////////////////////////////////////////////////////////
        }
    }
    //
    for(i =0; i< m_nMCBNum; i++)
    {
        m_timenodata[i] = NucTool::GetTickCount();
    }
}


void CMCBCtrlC::ProcStartDaqCmd(unsigned short nMCB)
{
    //////////////////////////////////////////////////////////////
    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    unsigned short i = 0;
    unsigned short j = 0;
    INT8U  ret = 0;
    //////////////////////////////////////////////////////////////
    //������_ETH_CTRL_socket��������������������������������
    ETH_CTRL_read_buffer_residual_data(nMCB);
    //////////////////////////////////////////////////////////////
    //������������
    usleep(1*1000);
    ret = EDAQ_DB_daq_cmd_tx_req(nMCB);
    if(0 != ret)
    {
        memset(tmp,0,512);
        sprintf(tmp,"Error when calling EDAQ_DB_daq_cmd_tx_req() by HMCB%d. The return value is %d.\n", nMCB, ret);
        sz_szInfo = tmp;
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////
        //����������������������
        ret = EDAQ_DB_daq_cmd_tx_req(nMCB);
        if(0 != ret)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_DB_daq_cmd_tx_req() by HMCB%d secondly. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            WriteLogFile(sz_szInfo);
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"No error when calling EDAQ_DB_daq_cmd_tx_req() by HMCB%d secondly. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            WriteLogFile(sz_szInfo, gLogLevel1);
        }
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"No error when calling EDAQ_DB_daq_cmd_tx_req() by HMCB%d. The return value is %d.\n", nMCB, ret);
        sz_szInfo = tmp;
        WriteLogFile(sz_szInfo, gLogLevel1);
    }
    //////////////////////////////////////////////////////////////
    //������_ETH_CTRL_socket��������������������������������
    ETH_CTRL_read_buffer_residual_data(nMCB);
    /////////////////////////////////////////////////////////////////
    //������_ETH_CTRL_socket��������������������������������
    //������������
    //ret = SetTrigCtrlReg(nMCB);
	//设置Trigger 并 使能 采集
	ret = SetTrigCtrlRegAndEnableDAQ(1, nMCB);
    if(0 != ret)
    {
        memset(tmp,0,512);
        sprintf(tmp,"Error when calling SetTrigCtrlRegAndEnableDAQ() by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
        sz_szInfo = tmp;
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////
        //����������������������
        ret = SetTrigCtrlRegAndEnableDAQ(1, nMCB);
        if(0 != ret)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling SetTrigCtrlRegAndEnableDAQ() by HMCB(4)%d secondly. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            WriteLogFile(sz_szInfo);
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"No error when calling SetTrigCtrlRegAndEnableDAQ() by HMCB(4)%d secondly. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            WriteLogFile(sz_szInfo, gLogLevel1);
        }
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"No error when calling SetTrigCtrlRegAndEnableDAQ() by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
        sz_szInfo = tmp;
        WriteLogFile(sz_szInfo, gLogLevel1);
    }
    /////////////////////////////////////////////////////////////////
    //������_ETH_CTRL_socket��������������������������������
    //ETH_CTRL_read_buffer_residual_data(nMCB);
    ////������������//CB������������������
    //ret = EDAQ_daq_mode_enable(1, nMCB);
    //if(0 != ret)
    //{
    //    memset(tmp,0,512);
    //    sprintf(tmp,"Error when calling EDAQ_daq_mode_enable(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
    //    sz_szInfo = tmp;
    //    WriteLogFile(sz_szInfo);
    //    //////////////////////////////////////////////////////////////
    //    //����������������������
    //    ret = EDAQ_daq_mode_enable(1, nMCB);
    //    if(0 != ret)
    //    {
    //        memset(tmp,0,512);
    //        sprintf(tmp,"Error when calling EDAQ_daq_mode_enable(1) by HMCB%d secondly. The return value is %d.\n", nMCB, ret);
    //        sz_szInfo = tmp;
    //        WriteLogFile(sz_szInfo);
    //    }
    //    else
    //    {
    //        memset(tmp,0,512);
    //        sprintf(tmp,"No error when calling EDAQ_daq_mode_enable(1) by HMCB%d secondly. The return value is %d.\n", nMCB, ret);
    //        sz_szInfo = tmp;
    //        WriteLogFile(sz_szInfo, gLogLevel1);
    //    }
    //}
    //else
    //{
    //    memset(tmp,0,512);
    //    sprintf(tmp,"No error when calling EDAQ_daq_mode_enable(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
    //    sz_szInfo = tmp;
    //    WriteLogFile(sz_szInfo, gLogLevel1);
    //}
    /////////////////////////////////////////////////////////////////
    usleep(1*1000);
}
//停止主控制盒
void CMCBCtrlC::Stop(int nCmd, unsigned short nMCB /*= 0*/)//0--Cmd 1--Acq  2--Overtime
{
    //////////////////////////////////////////////////////////////////////////////////
    m_bPreStop = true;
    //////////////////////////////////////////////////////////////////////////////////
	m_timepreend = NucTool::GetTickCount();
    int i = 0;
    //////////////////////////////////////////////////////////////////////////////////
    bool b = true;
    for(int i = 0; i < m_nMCBNum; i++)
    {
        b = b && m_bPreDAQSucessFlag[i];

    }
    //////////////////////////////////////////////////////////////////////////////////
    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    if(0 == nCmd)
    {
        memset(tmp,0,512);
        sprintf(tmp,"HMCB.dll Receive Stop CMD!\n");
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
		//已收到停止采集命令
		m_bRecStopCmd = true;
    }
    else if(1 == nCmd)
    {
        memset(tmp,0,512);
        sprintf(tmp,"HMCB%d has acquired enough data to stop acquisition!\n", nMCB);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
    }
    else// 2--Overtime
    {
        memset(tmp,0,512);
        sprintf(tmp,"HMCB.dll execution command timeout and stop!\n");
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
    }
    WriteLogFile(sz_szInfo);
    //////////////////////////////////////////////////////////////////////////////////
    //m_bPreStop = true;
    //////////////////////////////////////////////////////////////////////////////////
    INT8U  ret = 1;
    //////////////////////////////////////////////////////////////////////////////////
    m_timeprestop = NucTool::GetTickCount();
    //////////////////////////////////////////////////////////////////////////////////
    unsigned short nMasterMCB = 0;
    //如果只有一个控制盒，则直接停止主控制盒采集
    //如果有2个或2个以上控制盒，则也先停止主控制盒
    //在OnTime ->CheckStopSts函数内再停止丛控制盒采集
    //unsigned short nMCB = 0;
    //第3bit置0
    m_MCBRunSts[nMasterMCB] &= 0xfb;
    //
    if(m_pDataSocket[nMasterMCB])
    {
        memset(tmp,0,512);
        sprintf(tmp,"Start calling SetMainHMCBTrigCtrlRegAsNONE() by HMCB%d.\n", nMasterMCB);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //
        //ret = EDAQ_daq_mode_enable(0, nMCB);
        //qieduanchufayuan
        ret = SetMainHMCBTrigCtrlRegAsNONE();
        if(0 != ret)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_daq_mode_enable(0) by HMCB%d. The return value is %d.\n", nMasterMCB, ret);
            sz_szInfo= tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
            //ret = EDAQ_daq_mode_enable(0, nMCB);
            //qieduanchufayuan
            ret = SetMainHMCBTrigCtrlRegAsNONE();
            if(0 != ret)
            {
                memset(tmp,0,512);
                sprintf(tmp,"Error again when calling SetMainHMCBTrigCtrlRegAsNONE() by HMCB%d. The return value is %d.\n", nMasterMCB, ret);
                sz_szInfo= tmp;
                //std::cout <<(sz_szInfo);
                WriteLogFile(sz_szInfo);
            }
            else
            {
                memset(tmp,0,512);
                sprintf(tmp,"Repeated call to SetMainHMCBTrigCtrlRegAsNONE() by HMCB%d without any errors.\n", nMasterMCB);
                sz_szInfo= tmp;
                //std::cout <<(sz_szInfo);
                WriteLogFile(sz_szInfo);
            }
        }
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"HMCB%d : not ready, can't receive and execute stop command!\n", nMasterMCB);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVERR, ((long(nMasterMCB))<<16) + MCB_ERRCD_DVNOREADY);
    }
    //////////////////////////////////////////////////////////////////////////////////
    if(0 == ret)
    {
        memset(tmp,0,512);
        sprintf(tmp,"HMCB%d called SetMainHMCBTrigCtrlRegAsNONE() and executed successfully. STOP data acquisition.\n", nMasterMCB);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        ///////////////////////////
        m_MCBRunSts[nMasterMCB] |= 0x04;
        //
        //SendMCBMsg(MCB_MSG_STATUS, MCB_STOPOK, ((long(nMCB))<<16) + 0);
    }
    else
    {
        /////////////////////////////////////////////////////////////////////
        //流程关注
        sz_szInfo= "?????Master HMCB failed to stop and terminated abnormally.\n";
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        ///////////////////////////
        m_MCBRunSts[nMasterMCB] |= 0x04;
        //
        m_timepreend = NucTool::GetTickCount();
        //SendMCBMsg(MCB_MSG_STATUS, MCB_STOPOK, ((long(nMCB))<<16) + 0);
    }
    //////////////////////////////////////////////////////////////////////////////////
    if(m_bSocketRawDatafile)
    {
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        m_SocketRawDatafile.Close();
#else
        m_SocketRawDatafile.close();
#endif
        m_bSocketRawDatafile = false;
    }
    //////////////////////////////////////////////////////////////////////////////////
}

void CMCBCtrlC::CheckRunStatus()
{
    /////////////////////////////////////////////////////
    std::string sz_szInfo = ("");
    char tmp[512] = {0};
	/////////////////////////////////////////////////////
	if(m_bRecStopCmd)
	{
	    sz_szInfo = "Previously received MCB_CMD_STOP command, process terminated abnormally.\n";
		WriteLogFile(sz_szInfo);
		return;
	}
	/////////////////////////////////////////////////////
    unsigned short i = 0;
    bool b = false;
    unsigned short nMCB = 0;
    INT8U ret = 0;
    //
    sz_szInfo = "Enter CheckRunStatus process.\n";
    WriteLogFile(sz_szInfo, gLogLevel6);
    //如果只有一个控制盒，start已直接启动采集
    //如果有2个或2个以上控制盒，则start先启动从控制盒，在OnTime ->CheckRunStatus函数内再启动主控制盒
    if (m_nMCBNum >1 )
    {
        if (false == m_bSlaveStart) //check if slave MCB ready to start
        {
            b = true;
            for (i = m_nMCBNum; i > 1; i--)
            {
                b = b &&((m_MCBRunSts[i-1]&0x01)==1);
            }
            if (b) // start master MCB
            {
                //std::cout <<(("Slave HMCB ready, start master HMCB...\n"));
                WriteLogFile(("Slave HMCB ready, start master HMCB...\n"));
                m_bSlaveStart = true;
                ///////////////////////////
                nMCB = 0;
                m_MCBRunSts[nMCB] &=0xfe;
                /////////////////////////////
                //不需CheckRunStatus
                m_bPreStart = false;
                /////////////////////////////
                //std::cout <<(("Master HMCB start OK, begin running...\n"));
                WriteLogFile(("Master HMCB start OK, begin running...\n"));
                SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_STARTOK);
                ///////////////////////////
                memset(tmp,0,512);
                sprintf(tmp,"HMCB No.%d start receive data.\n", nMCB);
                sz_szInfo= tmp;
                //std::cout <<(sz_szInfo);
                WriteLogFile(sz_szInfo);
                m_MCBRunSts[nMCB] |=0x01;
                /////////////////////////////////////////////////////////////

               // usleep(200*1000);
                ProcStartDaqCmd(nMCB);
                /*             /////////////////////////////////////////////////////////////
                //执行X命令读取部分寄存器值
                DumpAllTheECSRegistersRelease(0, 0);
                ///
                //ret = EDAQ_daq_mode_enable(1, nMCB);//进入取数模式//CB进入到数据采集模式
                //if(0 != ret)
                //{
                //	sz_szInfo.Format(("START master HMCB No.%d EDAQ_daq_mode_enable Find errors.\n"), nMCB);
                //	std::cout <<(sz_szInfo);
                //	WriteLogFile(sz_szInfo);
                //}
                ///////////////////////////////////////////////////////////////////
                //ret = EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_ETH_DATA, nMCB);//选择DDR缓存的读出数据的目的地：以太网
                //if(0 != ret)
                //{
                //	sz_szInfo.Format(("START master HMCB No.%d EDAQ_RB_rd_port_sel Find errors.\n"), nMCB);
                //	std::cout <<(sz_szInfo);
                //	WriteLogFile(sz_szInfo);
                //}
                ////
                ///////////////////////////////////////////////////////////////////
                ////设置RB缓存工作模式. 1: CMD mode. 0: Auto mode.
                //ret = EDAQ_RB_working_mode_set(1, nMCB);
                //if(0 != ret)
                //{
                //	sz_szInfo.Format(("START master HMCB No.%d EDAQ_RB_working_mode_set Find errors.\n"), nMCB);
                //	std::cout <<(sz_szInfo);
                //	WriteLogFile(sz_szInfo);
                //}
                ///////////////////////////////////////////////////////////////
                nMCB = 0;
                m_MCBRunSts[nMCB] &= 0xfe;
                /////////////////////////////
                //不需CheckRunStatus
                m_bPreStart = false;
                //
                //std::cout <<(("Master HMCB start OK, begin running...\n"));
                WriteLogFile(("Master HMCB start OK, begin running...\n"));
                SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_STARTOK);
                ///////////////////////////
                memset(tmp,0,512);
                sprintf(tmp,"HMCB No.%d start receive data.\n", nMCB);
                sz_szInfo= tmp;
                //std::cout <<(sz_szInfo);
                WriteLogFile(sz_szInfo);
                m_MCBRunSts[nMCB] |=0x01;
                //////////////////////////////////////////////////////////////
                //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
                ETH_CTRL_read_buffer_residual_data(nMCB);
                //启动数据采集
                ret = EDAQ_DB_daq_cmd_tx_req(nMCB);
                if(0 != ret)
                {
                    memset(tmp,0,512);
                    sprintf(tmp,"Error when calling EDAQ_DB_daq_cmd_tx_req() by HMCB（4）%d. The return value is %d.\n", nMCB, ret);
                    sz_szInfo= tmp;
                    //std::cout <<(sz_szInfo);
                    WriteLogFile(sz_szInfo);
                }
                else
                {
                    memset(tmp,0,512);
                    sprintf(tmp,"No error when calling EDAQ_DB_daq_cmd_tx_req() by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
                    sz_szInfo= tmp;
                    //std::cout <<(sz_szInfo);
                    WriteLogFile(sz_szInfo, gLogLevel1);
                }
                /////////////////////////////////////////////////////////////////
                //Set trigger mode Ctrl reg
                ret = SetTrigCtrlReg(nMCB);//
                if(0 != ret)
                {
                    memset(tmp,0,512);
                    sprintf(tmp,"Error when calling SetTrigCtrlReg() by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
                    sz_szInfo= tmp;
                    //std::cout <<(sz_szInfo);
                    WriteLogFile(sz_szInfo);
                }
                else
                {
                    memset(tmp,0,512);
                    sprintf(tmp,"No error when calling SetTrigCtrlReg() by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
                    sz_szInfo= tmp;
                    //std::cout <<(sz_szInfo);
                    WriteLogFile(sz_szInfo, gLogLevel1);
                }
                /////////////////////////////////////////////////////////////////
                //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
                ETH_CTRL_read_buffer_residual_data(nMCB);
                //进入取数模式//CB进入到数据采集模式
                ret = EDAQ_daq_mode_enable(1, nMCB);//进入取数模式//CB进入到数据采集模式
                if(0 != ret)
                {
                    memset(tmp,0,512);
                    sprintf(tmp,"Error when calling EDAQ_daq_mode_enable(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
                    sz_szInfo= tmp;
                    //std::cout <<(sz_szInfo);
                    WriteLogFile(sz_szInfo);
                }
                else
                {
                    memset(tmp,0,512);
                    sprintf(tmp,"No error when calling EDAQ_daq_mode_enable(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
                    sz_szInfo= tmp;
                    //std::cout <<(sz_szInfo);
                    WriteLogFile(sz_szInfo, gLogLevel1);
                }
*/              ///////////////////////////////////////////////////////////////////
                return;
            }
            else if ((NucTool::GetTickCount() - m_timeprestart) > 8000)
            {
                //std::cout <<(("Slave HMCB start error, over time!\n"));
                WriteLogFile(("Slave HMCB start error, over time!\n"));
                m_bStart = false;
                SendMCBMsg(MCB_MSG_STATUS, MCB_STARTERR, 1);
            }
        }
        else //check master MCB start status
        {
            if ((m_MCBRunSts[0]&0x01) ==1)
            {
                //std::cout <<(("Master HMCB start OK, begin running...\n"));
                //WriteLogFile(("Master HMCB start OK, begin running...\n"));
                m_bPreStart =false;
                //SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, MCB_STSCD_STARTOK);
            }
            else if ((NucTool::GetTickCount()-m_timeprestart) >10000)
            {
                //std::cout <<(("Master HMCB start error, over time!\n"));
                WriteLogFile(("Master HMCB start error, over time!\n"));
                m_bStart = false;
                SendMCBMsg(MCB_MSG_STATUS, MCB_STARTERR, 0);
                //
                m_timeprestart = NucTool::GetTickCount();
            }
        }
    }
    else if (m_nMCBNum ==1)
    {
        if ((m_MCBRunSts[0]&0x01) ==1)
        {
            //std::cout <<(("Master HMCB start OK, begin running...\n"));
            //WriteLogFile(("Master HMCB start OK, begin running...\n"));
            //不需CheckRunStatus
            m_bPreStart = false;
            //SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, MCB_STSCD_STARTOK);
        }
        else if ((NucTool::GetTickCount()-m_timeprestart) >10000)
        {
            //std::cout <<(("Master HMCB start error, over time!\n"));
            WriteLogFile(("Master HMCB start error, over time!\n"));
            m_bStart = false;
            SendMCBMsg(MCB_MSG_STATUS, MCB_STARTERR, 0);
            //
            m_timeprestart = NucTool::GetTickCount();
        }
    }
}

void CMCBCtrlC::CheckStopSts()
{
    ///////////////////////////////////////////////////////
    //如果只有一个控制盒，则直接停止主控制盒采集
    //如果有2个或2个以上控制盒，则也先停止主控制盒
    //在OnTime ->CheckStopSts函数内再停止丛控制盒采集
    std::string sz_szInfo = ("");
    std::string str = ("");
    char tmp[512] = {0};
    unsigned short i = 0;
    //
    bool b = true;
    //
    sz_szInfo = "Enter CheckStopSts process.\n";
    WriteLogFile(sz_szInfo, gLogLevel6);
    ///////////////////////////////////////////////////////
    INT8U ret = 0;
    unsigned short nMCB = 0;
    //主控制盒状态
    b = ((m_MCBRunSts[nMCB]&0x04)== 0x04);
    ///////////////////////////////////////////////////////
    if(!b)
    {
        sz_szInfo= "Master HMCB not end running and end receive data.\n";
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //重发停止采集??
        //Stop(2);
    }
    else
    {
		for(i = 0; i < m_nMCBNum; i++)
		{
			if((NucTool::GetTickCount() - m_timeSocketRecData[i]) < 1000)////任何一个控制盒的socket数据端口在近1秒内有数据，则直接返回不停止采集
			{
				memset(tmp,0,512);
				sprintf(tmp,"[CheckStopSts]: (NucTool::GetTickCount() - m_timeSocketRecData[i]) < 1000  HMCB %d \n",i);
				sz_szInfo = tmp;
				WriteLogFile(sz_szInfo);
				//////////////////////////////////////////////////////////////////
				//如果收到stop命令后2秒钟内还没结束，则等待；大于2秒钟则不再接收数据，直接停止采集
				if ((NucTool::GetTickCount() - m_timeprestop) < 3000)
				{
					return;
				}
				else
				{
					memset(tmp,0,512);
					sprintf(tmp,"[CheckStopSts]: (NucTool::GetTickCount() - m_timeprestop > 3000, Stop receiving all HMCB data.\n");
					sz_szInfo = tmp;
					WriteLogFile(sz_szInfo);
				}
			}
		}
        //		if (1 == m_nMCBNum)  //only one MCB, stop at once;
        //		{
        //		}
        //		else
        {
            //stop slave MCB
            //for(i = m_nMCBNum; i > 1; i--)
            for(i = 0; i < m_nMCBNum; i++)
            {
                //nMCB = i - 1;
                nMCB = i;
                //第3bit置0
                m_MCBRunSts[nMCB] &= 0xfb;
                //
                if(m_pDataSocket[nMCB])
                {
                    ret = EDAQ_daq_mode_enable(0, nMCB);//停止采集
                    //ret = EDAQ_daq_mode_enable(0, nMCB);//停止采集
					//WriteLogFile("EDAQ_daq_mode_enable(0)  @@@@.\n");
					memset(tmp,0,512);
                    sprintf(tmp,"HMCB%d EDAQ_daq_mode_enable(0)  @@@@.\n", nMCB);
                    sz_szInfo= tmp;
                    //std::cout <<(sz_szInfo);
                    WriteLogFile(sz_szInfo);
                }
                else
                {
                    memset(tmp,0,512);
                    sprintf(tmp,"HMCB%d : not ready, can't receive stop command!\n", nMCB);
                    sz_szInfo= tmp;
                    //std::cout <<(sz_szInfo);
                    WriteLogFile(sz_szInfo);
                    SendMCBMsg(MCB_MSG_STATUS, MCB_DVERR, ((long(nMCB))<<16) + MCB_ERRCD_DVNOREADY);
                }
                //////////////////////////////////////////////////////////////////////////////////
                if(0 == ret)
                {
                    memset(tmp,0,512);
                    sprintf(tmp,"HMCB%d stop running and receive data.\n", nMCB);
                    sz_szInfo= tmp;
                    //std::cout <<(sz_szInfo);
                    WriteLogFile(sz_szInfo);
                    ///////////////////////////
                    m_MCBRunSts[nMCB] |=0x04;
                    //
                    //SendMCBMsg(MCB_MSG_STATUS, MCB_STOPOK, ((long(nMCB))<<16) + 0);
                }
                else
                {
                    WriteLogFile("EDAQ_daq_mode_enable  @@@@ 0 != ret ");
                }
            }
        }
    }
    /////////////////////////////////////////////////////////
    for (i =0; i< m_nMCBNum; i++)
    {
        b = b && ((m_MCBRunSts[i]&0x04)== 0x04);//设备就绪状态
    }
    /////////////////////////////////////////////////////////
    /*   bool bReceiveAll = false;
    if(b)
    {
        //找到所有MCB中"end receive data by command, send packet to APP"的最大值和最小值
        //最大值和最小值相差16以上时，进入enter send all data mode，最长8秒钟
        //m_nNeedSendData取值处需要修改，每更新一次包裹数更新一次
        LONG lMinPacket = m_nNeedSendData[0];
        LONG lMaxPacket = m_nNeedSendData[0];
        //
        for (i =0; i < m_nMCBNum; i++)
        {
            if(lMinPacket > m_nNeedSendData[i])
            {
                lMinPacket = m_nNeedSendData[i];
            }
            if(lMaxPacket < m_nNeedSendData[i])
            {
                lMaxPacket = m_nNeedSendData[i];
            }
        }
        if((lMaxPacket - lMinPacket) > 16)
        {
            bReceiveAll = true;
            if(0 == m_nSendAllRecorded)
            {
                m_nSendAllRecorded = 1;//进入send all 模式日志是否已记录 0 初始 1需要记录 2已记录
            }
        }
        if(1 == m_nSendAllRecorded)
        {
            m_nSendAllRecorded = 2;//进入send all 模式日志是否已记录 0 初始 1需要记录 2已记录
            //std::cout <<(("Enter send all data mode!\n"));
            WriteLogFile(("Enter send all data mode!\n"));
        }
        if (bReceiveAll && ((NucTool::GetTickCount() - m_timeprestop) < 8000))
        {
            return; //wait for SendAllData
        }
    }
*/	/////////////////////////////////////////////////////////
    if(b)
    {
        //m_bPreStop = FALSE;
        m_bStart = false;
    }
    /////////////////////////////////////////////////////////
    if ((NucTool::GetTickCount() - m_timeprestop) > 30000)
    {
        //std::cout <<(("HMCB stop error, over time!\n"));
        WriteLogFile(("HMCB stop error, over time(1)!\n"));
        //
        //m_bPreStop = FALSE;
        m_bStart = false;
        //
        SendMCBMsg(MCB_MSG_STATUS, MCB_STOPERR, 0);
    }
    /////////////////////////////////////////////////////////
}

void CMCBCtrlC::CheckDataEnd()
{
    /////////////////////////////////////////////////////
    std::string sz_szInfo = ("");
    char tmp[512] ={0};
    sz_szInfo = "Enter CheckDataEnd process.\n";
    WriteLogFile(sz_szInfo, gLogLevel6);
    //
    int id = 0;
    unsigned long i = 0;
    unsigned short j = 0;
    bool b = true;
    std::string str = ("");

    b = true;
    for (id =0; id < m_nMCBNum; id++)
    {
        b = b &&((m_MCBRunSts[id]&0x04) == 0x04);
  }
    if (b) // all MCB already end data acquire normally
    {
		for(i = 0; i < m_nMCBNum; i++)
		{
			if((NucTool::GetTickCount() - m_timeSocketRecData[i]) < 1000)////任何一个控制盒的socket数据端口在近1秒内有数据，则直接返回不停止采集
			{
				memset(tmp,0,512);
				sprintf(tmp,"[CheckDataEnd]: (NucTool::GetTickCount() - m_timeSocketRecData[i]) < 1000  HMCB %d \n",i);
				sz_szInfo = tmp;
				WriteLogFile(sz_szInfo);
				//////////////////////////////////////////////////////////////////
				//如果收到stop命令后2秒钟内还没结束，则等待；大于3秒钟则不再接收数据，直接停止采集
				if ((NucTool::GetTickCount() - m_timeprestop) < 3000)
				{
					return;
				}
				else
				{
					memset(tmp,0,512);
					sprintf(tmp,"[CheckDataEnd]: (NucTool::GetTickCount() - m_timeprestop > 3000, Stop receiving all HMCB data.\n");
					sz_szInfo = tmp;
					WriteLogFile(sz_szInfo);
				}
			}
		}
        //std::cout <<(("All HMCBs normal ended receiving data successfully!\n"));
        WriteLogFile(("All HMCBs normal ended receiving data successfully!\n"));
        m_bStart = false;
         //
        m_bPreStop = false;  
		//
        //SendMCBMsg(MCB_MSG_STATUS, MCB_STOPOK, 1);
        //
        if(!m_bPreDataEnd)
        {
            for (id =0; id <m_nMCBNum; id++)
            {
                NormalEndDAQ(id);
            }
        }

 		////////////////////////////////////////////////////////////////////////
		//ResAD();
		//给软件回调 MCB_STOPOK
		SendMCBMsg(MCB_MSG_STATUS, MCB_STOPOK, (m_bRecStopCmd ? 0 : 1));//0 -by cmd, 1 - normal
       ////////////////////////////////////////////////////////////////////////
        // record goudang info
        if (m_scounter[0] >INVALID_DATA_PACKET_NUM)
        {
            bool b = false;
            int dlast = m_godown[0];
            for (i = 0; i < min(m_scounter[0]-INVALID_DATA_PACKET_NUM-1, (unsigned long)GODOWNNUM); i++)
            {
                if (dlast !=m_godown[i])
                {
                    if (!b)
                    {
                        sz_szInfo = ("GOUDANG INFO:\n");
                        memset(tmp,0,512);
                        sprintf(tmp,"                         %d,%d\n", 0, m_godown[0]);
                        sz_szInfo += tmp;
                        //
                        b =true;
                    }

                    memset(tmp,0,512);
                    sprintf(tmp,"                         %d,%d\n", i+1, m_godown[i]);
                    sz_szInfo += tmp;
                    dlast =m_godown[i];
                }
            }
            if (b)  //write goudong info
            {
                //std::cout <<(sz_szInfo);
                WriteLogFile(sz_szInfo);
            }
        }
        ////////////////////////////////////////////////////////////////////////
        if (m_scounter[0] >INVALID_DATA_PACKET_NUM)  //output 1channel det_data
        {
            memset(tmp,0,512);
            sprintf(tmp,"Det_Data INFO(%d):\n", m_nRecvStartCmdCnt);
            sz_szInfo= tmp;
            sz_szInfo += "                         SN,DI0,DI1,DI2,DI3,MCB0CH0";
            for (id=1; id<m_nMCBNum; id++)
            {

                memset(tmp,0,512);
                sprintf(tmp,",MCB%dCH0",id);
                sz_szInfo += tmp;
            }
            sz_szInfo +=("\n");
            for (i=0; i< min(m_scounter[0]-INVALID_DATA_PACKET_NUM-1, (unsigned long)DDataNUM); i++)
            {
                memset(tmp,0,512);
                sprintf(tmp,"                         %d,%d,%d,%d,%d,%d", i+1, m_InfoExdata[2][i]&0x1, (m_InfoExdata[2][i]>>1)&0x1, (m_InfoExdata[2][i]>>2)&0x1, (m_InfoExdata[2][i]>>3)&0x1, m_uDetData[0][i]);

                sz_szInfo += tmp;
                for (j=1; j<m_nMCBNum; j++)
                {
                    memset(tmp,0,512);
                    sprintf(tmp,",%d",m_uDetData[j][i]);
                    sz_szInfo += tmp;
                }
                sz_szInfo +=("\n");
            }
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        ////////////////////////////////////////////////////////////////////////
        //打印最后一次CB状态
        if (m_scounter[0] >INVALID_DATA_PACKET_NUM)  //output 1channel det_data
        {
            sz_szInfo = ("The last packet status:\n");
            for (id=0; id< m_nMCBNum; id++)
            {
                memset(tmp,0,512);
                sprintf(tmp,"                         HMCB%d: SC_pkg_index=%d,\t CB_logic_err1=0x%02X, CB_logic_err2=0x%02X, \t", id, m_pOneDBLineOnePackageStatue[id].SC_pkg_index, m_pOneDBLineOnePackageStatue[id].CB_logic_err1, m_pOneDBLineOnePackageStatue[id].CB_logic_err2);

                sz_szInfo += tmp;
                memset(tmp,0,512);
                sprintf(tmp,"one DB status: DB_board_index = %d, DB_logic_err1=0x%02X, DB_logic_err2=0x%02X.\n",
                        m_pOneDBLineOnePackageStatue[id].DB_board_index[0], m_pOneDBLineOnePackageStatue[id].DB_logic_err1[0], m_pOneDBLineOnePackageStatue[id].DB_logic_err2[0]);

                sz_szInfo += tmp;
            }
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
            WriteLogFile(("\n"));
        }
        ////////////////////////////////////////////////////////////////////////
        if (m_bSaveExdata)  //save DI, ptsm data...
        {
            SaveExdata();
        }
        //return;
    }
    //else if ((time(NULL)-m_timepreend) >5)
    else if ((NucTool::GetTickCount() - m_timepreend) >5000)
    {
        //std::cout <<(("HMCB stop error, over time(2)!\n"));
        WriteLogFile(("HMCB stop error, over time(2)!\n"));
        SendMCBMsg(MCB_MSG_STATUS, MCB_STOPERR, 1);
        //
        m_timepreend = NucTool::GetTickCount();
        //
        Stop(2);
    }
    /////////////////////////////////////////////////////////////
    //检测是否有采集失败的
    //DAQFailToReset();
//    for(int i=0;i< m_nMCBNum;i++)
//    {
//        glb_data_buf_vetor[i].clear();
//    }
}

void CMCBCtrlC::SetMode(unsigned short nMode)
{
#define MAXMODE 4

    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    if (m_bStart)
    {
        std::cout <<(("error: MCB is running, can't change mode!\n"));
        return;
    }

    if (nMode >MAXMODE-1)
    {
        nMode = MAXMODE-1;
    }

    memset(tmp,0,512);
    sprintf(tmp,"Set [MODE%d] and related parameters.\n", nMode);
    sz_szInfo= tmp;
    //std::cout <<(sz_szInfo);
    WriteLogFile(sz_szInfo);
    //
    memset(tmp,0,512);
    sprintf(tmp,"MODE%d", nMode);
    sz_szInfo= tmp;
    GetModeParam(sz_szInfo);
    for(unsigned short i =0; i< m_nMCBNum; i++)
    {
        SetReg(i);
    }
}
//设置探测器工作参数
// 0--success
//others -- error
int CMCBCtrlC::ConfigureUserParameter(unsigned short nMCB)
{
    std::string sz_szInfo = ("");
    std::string str = ("");
    char tmp[512] = {0};
    //////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////
    //参数长度：DB数目*2
    //其中，第i*2～i*2+1个字节的参数对应着编号为i的DB的参数。
    //将字节转化为参数时，按照大端格式进行，即序号小的参数对应着参数的高字节。
    INT8U i,dbline_i;
    INT8U wr_prm[256] = { 0x00 };
    INT8U rd_prm[256] = { 0x00 };
    INT8U nDMNum = m_DMInfo[nMCB].nDMNum;
    INT8U prm_len = 2*nDMNum;
    INT8U nFucRet = 0;//函数返回值
    INT8U nLastError = 0;//异常标识值
    //
    memset(tmp,0,512);
    sprintf(tmp,"Configure HMCB%d %d DM parameters:", nMCB,nDMNum);
    sz_szInfo += tmp;
    //
    for(i=0; i < nDMNum; i++)
    {
        memset(tmp,0,512);
        sprintf(tmp,"[DM%d]-0x%04X, ", i, m_nADMCMDREG[nMCB][nDMNum - 1 - i]);
        sz_szInfo += tmp;
        //
        wr_prm[2*i]     = (m_nADMCMDREG[nMCB][nDMNum - 1 - i] & 0xff00)>>8;
        wr_prm[2*i + 1] = (m_nADMCMDREG[nMCB][nDMNum - 1 - i] & 0x00ff);
        //指定MCBx是否进入诊断模式，是，则配置参数最高位第15位（最低位为0位）置1
        if(m_bMCBxDTM[nMCB])
        {
            wr_prm[2*i] |= 0x80;
        }
    }
    //std::cout <<(sz_szInfo +("\n"));

    WriteLogFile(sz_szInfo +("\n"));
    //////////////////////////////////////////////////////////////////////////////////
    //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
    ETH_CTRL_read_buffer_residual_data(nMCB);
    //////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////
    //Config the control bus between CB and DB to be used as communication bus
    EDAQ_DB_FPGA_cfg_enable(0, nMCB);
    //////////////////////////////////////////////////////////////////////////////////
    //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
    ETH_CTRL_read_buffer_residual_data(nMCB);
    //
    EDAQ_daq_mode_enable(0, nMCB);
    /////////////////////////////////////////////////
    //功能：设置RB缓存工作模式
    //参数：
    //	Mode =	1: CMD mode. DB数据缓存采用该模式。
    //			0: Auto mode. 测试数据和DB FPGA下载程序的缓存采用该模式。
    //返回值: 0 = No error, others = Find errors.
    nFucRet = EDAQ_RB_working_mode_set(1, nMCB);	//设置RB缓存工作模式. 1: CMD mode. 0: Auto mode.
    if(nFucRet > 0)
    {
        nLastError++;
        memset(tmp,0,512);
        sprintf(tmp,"Error when calling EDAQ_RB_working_mode_set(1) by HMCB%d. The return value is %d.\n", nMCB, nFucRet);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"No error when calling EDAQ_RB_working_mode_set(1) by HMCB%d. The return value is %d.\n", nMCB, nFucRet);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo, gLogLevel1);
    }
    /////////////////////////////////////////////////
    //功能：选择Ring buffer写入端口
    //返回值: 0 = No error, others = Find errors.
    nFucRet = EDAQ_RB_wr_port_sel(EDAQ_RB_WR_SEL_DB_RX_DATA, nMCB);  //选择DDR缓存的写入数据的来源：DB数据
    if(nFucRet > 0)
    {
        nLastError++;
        memset(tmp,0,512);
        sprintf(tmp,"Error when calling EDAQ_RB_wr_port_sel(EDAQ_RB_WR_SEL_DB_RX_DATA) by HMCB%d. The return value is %d.\n", nMCB, nFucRet);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"No error when calling EDAQ_RB_wr_port_sel(EDAQ_RB_WR_SEL_DB_RX_DATA) by HMCB%d. The return value is %d.\n", nMCB, nFucRet);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo, gLogLevel1);
    }
    /////////////////////////////////////////////////
    //功能：选择Ring buffer读取端口
    //返回值: 0 = No error, others = Find errors.
    nFucRet = EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_ETH_DATA, nMCB);	 //选择DDR缓存的读出数据的目的地：以太网
    if(nFucRet > 0)
    {
        nLastError++;
        memset(tmp,0,512);
        sprintf(tmp,"Error when calling EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_ETH_DATA) by HMCB%d. The return value is %d.\n", nMCB, nFucRet);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"No error when calling EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_ETH_DATA) by HMCB%d. The return value is %d.\n", nMCB, nFucRet);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo, gLogLevel1);
    }
    /////////////////////////////////////////////////
    //功能：使能/除能SC data pkg monitor
    //参数：
    //	en=	1: Enable
    //		0: Disable
    //返回值: 0 = No error, others = Find errors.
    nFucRet = EDAQ_SC_data_pkg_monitor_enable(1, nMCB);	//使能(置一)或禁用（清零）FPGA内嵌的SC数据包检查模块
    if(nFucRet > 0)
    {
        nLastError++;
        memset(tmp,0,512);
        sprintf(tmp,"Error when calling EDAQ_SC_data_pkg_monitor_enable(1) by HMCB%d. The return value is %d.\n", nMCB, nFucRet);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"No error when calling EDAQ_SC_data_pkg_monitor_enable(1) by HMCB%d. The return value is %d.\n", nMCB, nFucRet);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo, gLogLevel1);
    }
    //////////////////////////////////////////////////////////////////////////////////
    //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
    ETH_CTRL_read_buffer_residual_data(nMCB);
    /////////////////////////////////////////////////
    //EDAQ_DB_prm_cfg_RAM_test();
    dbline_i = 0;
    int nLoop = 0;
    //for(dbline_i = 0; dbline_i < 4; dbline_i++)
    do
    {
        //////////////////////////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
        ETH_CTRL_read_buffer_residual_data(nMCB);
        //////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////
        //功能：把参数写入到参数配置RAM中，并检查写入是否正确
        //nFucRet = EDAQ_DB_prm_cfg_RAM_write_with_check(dbline_i,wr_prm, prm_len, nMCB);
        nFucRet = EDAQ_DB_prm_cfg_RAM_write_with_check(dbline_i, wr_prm, rd_prm, prm_len, nMCB);
        ////////////////////////////////////////////////////////////////////////
        sz_szInfo = "";
        memset(tmp,0,512);
        sprintf(tmp,"Read back HMCB%d %d DMs parameters:", nMCB, nDMNum);
        sz_szInfo += tmp;
        //
        for(i=0; i < nDMNum; i++)
        {
            memset(tmp,0,512);
            sprintf(tmp,"[DM%d]-0x%02X%02X, ", i, rd_prm[i*2], rd_prm[i*2+1]);
            str = tmp;
            sz_szInfo += str;
        }
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo +("\n"));
        ///////////////////////////////////////////////////////////////////////
        if(nFucRet > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_DB_prm_cfg_RAM_write_with_check(dbline_i,wr_prm, prm_len) by HMCB%d. The return value is %d.\n", nMCB, nFucRet);
            sz_szInfo= tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"No error when calling EDAQ_DB_prm_cfg_RAM_write_with_check(dbline_i,wr_prm, prm_len) by HMCB%d. The return value is %d.\n", nMCB, nFucRet);
            sz_szInfo= tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo, gLogLevel1);
        }
        nLoop++;
        usleep(10*1000);
    }while((nFucRet != 0) && (nLoop < gCmdReptN));
    /////////////////////////////////////////////////
    if(nFucRet > 0)
    {
        nLastError++;
    }
    /////////////////////////////////////////////////
    //功能：发出DB FPGA复位请求
    //返回值: 0 = No error, others = Find errors.
    //reset
    nFucRet = EDAQ_DB_reset_req(nMCB);
    if(nFucRet > 0)
    {
        nLastError++;
        memset(tmp,0,512);
        sprintf(tmp,"Error when calling EDAQ_DB_reset_req() by HMCB%d. The return value is %d.\n", nMCB, nFucRet);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"No error when calling EDAQ_DB_reset_req() by HMCB%d. The return value is %d.\n", nMCB, nFucRet);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo, gLogLevel1);
    }
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    Sleep(10);
#else
    //usleep(10*1000);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
    //////////////////////////////////////////////////////////////////////////////////
    //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
    ETH_CTRL_read_buffer_residual_data(nMCB);
    /////////////////////////////////////////////////
    //功能：发出DB定位请求
    //locate
    nFucRet = EDAQ_DB_locate_req(nMCB);
    if(nFucRet > 0)
    {
        nLastError++;
        memset(tmp,0,512);
        sprintf(tmp,"Error when calling EDAQ_DB_locate_req() by HMCB%d. The return value is %d.\n", nMCB, nFucRet);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"No error when calling EDAQ_DB_locate_req() by HMCB%d. The return value is %d.\n", nMCB, nFucRet);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo, gLogLevel1);
    }
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    Sleep(10);
#else
    //usleep(10*1000);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
    //////////////////////////////////////////////////////////////////////////////////
    //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
    ETH_CTRL_read_buffer_residual_data(nMCB);
    /////////////////////////////////////////////////
    //功能：发出DB参数配置请求
    //DB prm cfg
    nFucRet = EDAQ_DB_prm_cfg_req(nMCB);
    if(nFucRet > 0)
    {
        nLastError++;
        memset(tmp,0,512);
        sprintf(tmp,"Error when calling EDAQ_DB_prm_cfg_req() by HMCB%d. The return value is %d.\n", nMCB, nFucRet);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"No error when calling EDAQ_DB_prm_cfg_req() by HMCB%d. The return value is %d.\n", nMCB, nFucRet);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo, gLogLevel1);
    }

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    Sleep(10);
#else
    //usleep(10*1000);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
    //当设置小于4个时耗时较长，注释掉不影响功能
    //ECM_read_and_parse_data(1, DB_RST_PKG, 1, 1, 0, cmd_pkg_parse_log_file,0);
    //ECM_read_and_parse_data(1, DB_LCT_PKG, 1, 1, 0, cmd_pkg_parse_log_file,0);
    //ECM_read_and_parse_data(1, DB_PRM_CFG_PKG, 1, 1, 0, cmd_pkg_parse_log_file,0);
    //
    //////////////////////////////////////////////////////////////////////////////////
    //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
    ETH_CTRL_read_buffer_residual_data(nMCB);
    //////////////////////////////////////////////////////////////////////////////////
    int nRet = 0;

    //	do{
    //Dump all the ECS registers
    nRet = DumpAllTheECSRegistersRelease(nMCB, 0, 1);
    if(nRet != 0)
    {
        nLoop++;
    }
    //	}while((nRet != 0) && (nLoop < gCmdReptN));
    ///////////////////////////////////////////////////
    //正确完成模式数识别
    if(0 == nRet)
    {
        //sz_szInfo.Format(_T("HMCB%d Auto DM number:%d.\n"), nMCB, 0);
        //TRACE(sz_szInfo);
        //WriteLogFile(sz_szInfo);
        //
        m_MCBReadySts[nMCB] |=0x08;
        //SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_REGSET_OK);
    }
    //else
    if(0 != nRet)
    {
        //////////////////////////////////////////////////////////////////////////////////
        memset(tmp,0,512);
        sprintf(tmp,"HMCB%d failed to self check the number of detector modules. The device will be reconnected..\n", nMCB);
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
        //sz_szInfo.Format(_T("The setting value of HMCB%d DB num of DB-Line0 is not equal to the parsed result of DB num of DB-Line0.\n"), nMCB);
        //TRACE(sz_szInfo);
        //WriteLogFile(sz_szInfo);
        m_MCBReadySts[nMCB] |=0x80;//自检失败
        //兼容老版本报 Detector module overflow
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_DMOVERFLOW);
        //新版本报 Detector module number error.
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_DMNUM_ERROR);
    }
    //
    return nRet;
}
//通过用户参数设置增益模式 SETRANGE
int CMCBCtrlC::SetDMCmdReg(unsigned short nMCB)
{
    std::string sz_szInfo = "";
    char tmp[512] = {0};
    int nRet = 0;
    //
    if (m_bStart)
    {
        std::cout << "Error: HMCB is running, can't set DM cmd register!\n";
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) +MCB_STSCD_SETREGATRUN);
        WriteLogFile(sz_szInfo);
        return 2;
    }
    //////////////////////////////////////////
    //通过用户参数设置增益模式 SETRANGE
    nRet = ConfigureUserParameter(nMCB);
    /////////////////////////////////////////
    //参数设置成功HMCB successfully completed setting DM CMD Reg value to 0
    if(0 == nRet)
    {
        memset(tmp,0,512);
        sprintf(tmp,"HMCB%d successfully completed setting DMs' Reg value to : 0x%04X, 0x%04X, 0x%04X, 0x%04X...\n", nMCB,
                m_nADMCMDREG[nMCB][0], m_nADMCMDREG[nMCB][1], m_nADMCMDREG[nMCB][2], m_nADMCMDREG[nMCB][3]);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //if set DM info
        m_bSetDMInfo[nMCB] = true;
    }
    //else
    if(0 != nRet)
    {
        memset(tmp,0,512);
        sprintf(tmp,"HMCB%d failed to Set DMs' CMD Reg value.The error code returned by the function is : %d.\n", nMCB, nRet);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //参数配置错误，重新初始化
        //if set DM info
        //m_bSetDMInfo[nMCB] = FALSE;
        //InitialMCB(nMCB);/////?
    }
    return nRet;
}
//

void CMCBCtrlC::SetRange(unsigned short nRange)
{
#define MAXRANGE 8

    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    if (!m_bDVReady)
    {
        sz_szInfo="HMCB not ready, can't set Range!\n";
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(0))<<16) +MCB_STSCD_SETREGATRUN);
        return;
    }
    //
    if (m_bStart)
    {
        memset(tmp,0,512);
        sprintf(tmp,"RANGE%d", nRange);
        sz_szInfo= tmp;
        //std::cout <<(("error: MCB is running, can't change Range: ")+ sz_szInfo +("\n"));
        WriteLogFile(("error: MCB is running, can't set Range: ")+ sz_szInfo +("\n"));
        return;
    }

    if (nRange >MAXRANGE-1)
    {
        nRange =MAXRANGE-1;
    }
    memset(tmp,0,512);
    sprintf(tmp,"RANGEMODE%d", nRange);
    sz_szInfo= tmp;
    //std::cout <<(("Set Range: ")+ sz_szInfo +("\n"));
    WriteLogFile(("Set Range: ")+ sz_szInfo +("\n"));
    GetRangeParam(sz_szInfo);
    //
    GetMCBxDMyPGz(("ADM"));
    //
    for(unsigned short i =0; i< m_nMCBNum; i++)
    {
        SetDMCmdReg(i);
    }
}

void CMCBCtrlC::SetRange(unsigned char * pRange)
{
    std::string sz_szInfo, str;
    char tmp[512] = {0};
    UINT i;

    if (m_bStart)
    {
        //std::cout <<(("error: MCB is running, can't change Range...\n"));
        WriteLogFile(("error: MCB is running, can't set Range...\n"));
        return;
    }

    unsigned short nMCB;
    //nMCB =*pRange;
    memcpy((unsigned char *)&nMCB,pRange,sizeof(unsigned short));
    if (nMCB >= m_nMCBNum)
    {
        nMCB =m_nMCBNum-1;
    }

    //sz_szInfo.Format(("MCB NO.%d --%d, %d, %d, %d, %d..."), nMCB, *(pRange +1), *(pRange +2), *(pRange +3), *(pRange +4), *(pRange +5));
    memset(tmp,0,512);
    sprintf(tmp,"MCB NO.%d --", nMCB);
    sz_szInfo= tmp;
    for (i=0; i< m_DMInfo[nMCB].nDMNum; i++)
    {
        memset(tmp,0,512);
        sprintf(tmp,"%d,", *(pRange +1 +i));

        sz_szInfo +=tmp;
    }
    //std::cout <<(("Set Range: ")+ sz_szInfo +("\n"));
    WriteLogFile(("Set Range: ")+ sz_szInfo +("\n"));

    UINT range;
    for (i=0; i< m_DMInfo[nMCB].nDMNum; i++)
    {
        range = (UINT)(*(pRange +i+1));
        if (range > MAX_DDCRANGE)
        {
            range =MAX_DDCRANGE;
        }
        else if (range <MIN_DDCRANGE)
        {
            range =MIN_DDCRANGE;
        }
        m_nADMCMDREG[nMCB][i] = ((m_nADMCMDREG[nMCB][i]&0xfffffff8)) + (range&0x07);
    }

    SetDMCmdReg(nMCB);

}
//获取增益设置参数
void CMCBCtrlC::GetRangeParam(std::string str)
{
    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    //
    CIniFile IniFile;
    IniFile.SetFileName(m_strMCBCtrlIniFile);
    //
    UINT range = MAX_DDCRANGE;
    int i = 0;
    int j = 0;
    int k = 0;
    for (i=0; i<MAX_MCBNUM; i++)
    {
        for(j=0; j<MAX_MCBADNUM; j++)
        {
            memset(tmp,0,512);
            sprintf(tmp,"MCB%dDM%d", i, j);
            sz_szInfo= tmp;
            //第1个dbline的增益默认值取最大值
            //range = (UINT)IniFile.GetInt(str, sz_szInfo, (j==0)?MAX_DDCRANGE:range);
            range = (UINT)IniFile.GetInt(str, sz_szInfo, range);
            ///////////////////////////
            if (range > MAX_DDCRANGE)
            {
                range =MAX_DDCRANGE;
            }
            else if (range <MIN_DDCRANGE)
            {
                range =MIN_DDCRANGE;
            }
            m_nADMCMDREG[i][j] = ((m_nADMCMDREG[i][j]&0xfffffff8)) +(range&0x07);
        }
    }
    /////////////////////////////////////////////////
    range = MAX_DDCRANGE;
    //探测器板工作寄存器配置参数(支持多块多端口级联)
    for (i=0; i<MAX_MCBNUM; i++)
    {
        for (j = 0; j < MAX_DBLINENUM; j++)
        {
            for(k = 0; k < MAX_MCBADNUM; k++)
            {
                //读取MCBxDMy作为默认值
                memset(tmp,0,512);
                sprintf(tmp,"MCB%dDM%d", i, k);
                sz_szInfo = tmp;
                range = (UINT)IniFile.GetInt(str, sz_szInfo, range);
                ////////////////////////////////////////////////////////////
                memset(tmp,0,512);
                sprintf(tmp,"MCB%dDB%dDM%d", i, j, k);
                sz_szInfo = tmp;
                //第1个dbline的增益默认值取最大值
                //range = (UINT)IniFile.GetInt(str, sz_szInfo, (j==0)?MAX_DDCRANGE:range);
                range = (UINT)IniFile.GetInt(str, sz_szInfo, range);
                ///////////////////////////
                if (range > MAX_DDCRANGE)
                {
                    range = MAX_DDCRANGE;
                }
                else if (range <MIN_DDCRANGE)
                {
                    range = MIN_DDCRANGE;
                }
                m_nMCBxDByREG[i][j][k] = ((m_nMCBxDByREG[i][j][k]&0xfffffff8)) +(range&0x07);
            }
        }
    }
}

void CMCBCtrlC::GetMCBxDMyPGz(std::string str)
{
    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    //
    CIniFile IniFile;
    IniFile.SetFileName(m_strMCBCtrlIniFile);

    double fPGValue = 1.0;
    int i, j , k;

    for (i=0; i<MAX_MCBNUM; i++)
    {
        for (j=0; j<MAX_MCBADNUM; j++)
        {
            for(k = 0; k < (MAX_DMROWNUM*MAX_DMCOLNUM/16); k++)
            {
                memset(tmp,0,512);
                sprintf(tmp,"MCB%dDM%dPG%d", i, j, k);
                sz_szInfo= tmp;
                fPGValue = IniFile.GetDouble(str, sz_szInfo, fPGValue);
                m_fMCBxDMyPGz[i][j][k] = fPGValue;
            }
        }
    }
}
//获取工作时序逻辑参数
void CMCBCtrlC::GetModeParam(std::string str)
{
    CIniFile IniFile;
    IniFile.SetFileName(m_strMCBCtrlIniFile);

    //m_reg.regCtrlA =unsigned char(IniFile.GetInt(str, ("CTRLA"), (m_nADBit ==24)?0x00:0x02));  //outer trig SE(20bit)/DIFF(24bit)
    //m_reg.regCtrlB =unsigned char(IniFile.GetInt(str, ("CTRLB"), 0x03));
    m_reg.regPOTMT =IniFile.GetInt(str, ("POTMT"), 4000);
    m_reg.regPOTMW =IniFile.GetInt(str, ("POTMW"), 100);
    m_reg.regINTT =IniFile.GetInt(str, ("INTT"), 200);
    ////////////////////////////////////////////////////////
    //如果INTT配置值大于65536的一半，则积分时间等于周期的一半
    if(m_reg.regINTT > 32768)
    {
        m_reg.regINTT = m_reg.regPOTMT / 2;
    }
    ////////////////////////////////////////////////////////
    m_reg.regDataRdyT = IniFile.GetInt(str, ("DATARDYT"), 480);//积分结束时刻与所有DB数据准备好时刻之间的间隔，经验值为480uS
    //DDI锁存时刻
    unsigned long nDefaultDDILatchT = m_reg.regINTT+200;//积分结束时刻与DDI锁存时刻之间的间隔，经验值为200uS
    //DDI锁存时刻也可以在MCBCtrl.ini文件内配置
    m_reg.regDDILatchT = IniFile.GetInt(str, ("DDILATCHT"), nDefaultDDILatchT);
}

void CMCBCtrlC::SetTrig(int trig)
{
    std::string sz_szInfo = ("");
    //
    m_nTrig = trig;
    //
    sz_szInfo= trig==0?("0-inner"):("1-outer");
    //std::cout <<(("Set trigger: ")+ sz_szInfo +("\n"));
    WriteLogFile(("Set trigger: ")+ sz_szInfo +("\n"));
    /////////////////////////////////////////////////////////
    //方案1
    //只改变主控制盒的触发方式设置
    //SetReg(0);
    //SetMode(0);
    //	////////////////////////////////////
    //	//方案2
    //	if(m_bEnMCBx4)
    //	{
    //		SetReg(0);
    //	}
    //	else
    //	{
    //		for(unsigned short i =0; i< m_nMCBNum; i++)
    //		{
    //			SetReg(i);
    //		}
    //	}
}

//������������������
INT8U CMCBCtrlC::SetMainHMCBTrigCtrlRegAsNONE()
{
    unsigned short nMCB = 0;
    //
    std::string sz_szInfo = ("");
    std::string sz_Temp = ("");
    char tmp[512] = {0};
    INT8U nLastError = 0;
    ////////////////////////////////////
    if (m_pDataSocket[nMCB])
    {
        sz_szInfo = "Set HMCB0 Trigger = EDAQ_DAQ_TRIG_SEL_NONE first when receive MCB_CMD_STOP cmd.\n";
        //std::cout << (sz_szInfo);
        WriteLogFile(sz_szInfo);
        //////////////////////////////////////////////////////////////////////////////////
        //������_ETH_CTRL_socket��������������������������������
        ETH_CTRL_read_buffer_residual_data(nMCB);
        //////////////////////////////////////////////////////
        //������������������������������
        nLastError = EDAQ_daq_req_sel(EDAQ_DAQ_TRIG_SEL_NONE, nMCB);//������������������
        usleep(10*1000);
        if(0 != nLastError)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_daq_req_sel(EDAQ_DAQ_TRIG_SEL_NONE) by HMCB%d. The return value is %d.\n", nMCB, nLastError);
            sz_szInfo = tmp;
            WriteLogFile(sz_szInfo);
            //////////////////////////////////////////////////////////////
            //����������������������
            nLastError = EDAQ_daq_req_sel(EDAQ_DAQ_TRIG_SEL_NONE, nMCB);//������������������
            usleep(10*1000);
            if(0 != nLastError)
            {
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling EDAQ_daq_req_sel(EDAQ_DAQ_TRIG_SEL_NONE) by HMCB%d secondly. The return value is %d.\n", nMCB, nLastError);
                sz_szInfo = tmp;
                WriteLogFile(sz_szInfo);
            }
            else
            {
                memset(tmp,0,512);
                sprintf(tmp,"No error when calling EDAQ_daq_req_sel(EDAQ_DAQ_TRIG_SEL_NONE) by HMCB%d secondly. The return value is %d.\n", nMCB, nLastError);
                sz_szInfo = tmp;
                WriteLogFile(sz_szInfo, gLogLevel1);
            }
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"No error when calling EDAQ_daq_req_sel(EDAQ_DAQ_TRIG_SEL_NONE) by HMCB%d. The return value is %d.\n", nMCB, nLastError);
            sz_szInfo = tmp;
            WriteLogFile(sz_szInfo, gLogLevel1);
        }
    }
    else
    {
        nLastError = -1;
        memset(tmp,0,512);
        sprintf(tmp,"HMCB%d not ready, can't set register in SetMainHMCBTrigCtrlRegAsNONE()!\n", nMCB);
        sz_szInfo = tmp;
        WriteLogFile(sz_szInfo);
        //SendMCBMsg(MCB_MSG_STATUS, MCB_DVERR, ((long(nMCB))<<16) +MCB_ERRCD_DVNOREADY);
    }
    return nLastError;
}
INT8U CMCBCtrlC::SetTrigCtrlReg(unsigned short nMCB)
{
    //    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    //    boost::unique_lock<boost::shared_mutex> read_lock(mutex1);
    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    INT8U nLastError = 0;
    ////////////////////////////////////
    if (m_pDataSocket[nMCB])
    {
        //sz_szInfo.Format(("HMCB No.%d Set Reg: CtrlA:0x%02x, POTMT:%d, POTMW:%d, INTT:%d, Trigger=%s.\n"),
        //	nMCB, m_reg.regCtrlA, m_reg.regPOTMT, m_reg.regPOTMW, m_reg.regINTT, ((0 == nMCB) && m_nTrig) == 0 ? ("0-inner"):("1-outer"));
        if(0 == nMCB)
        {
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Set Trigger=%s.\n", nMCB, ((0 == nMCB) && m_nTrig) == 0 ? ("0-inner"):("1-outer"));
            sz_szInfo= tmp;
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Set Trigger=Cascade trigger.\n", nMCB);
            sz_szInfo= tmp;
        }
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
        ///////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
        ETH_CTRL_read_buffer_residual_data(nMCB);
        ///////////////////////////////////////////////////////////////

        //主控制盒可以工作在内触发或外部脉冲触发
        if(0 == nMCB)
        {
            if(0 == m_nTrig)
            {
                EDAQ_daq_req_sel(EDAQ_DAQ_TRIG_SEL_PERIOD_TRIG, nMCB);//选择内触发
            }
            else
            {
                EDAQ_daq_req_sel(EDAQ_DAQ_TRIG_SEL_BEAM_PULSE, nMCB);//选择外触发
            }
        }
        else//所有从控制盒都工作在级联触发
        {
            nLastError = EDAQ_daq_req_sel(EDAQ_DAQ_TRIG_SEL_CASCADE_TRIG, nMCB);//选择级联触发
            //EDAQ_daq_req_sel(EDAQ_DAQ_TRIG_SEL_PERIOD_TRIG, nMCB);//选择外触发
        }
        //EDAQ_daq_mode_enable(1, nMCB);
        usleep(1*1000);
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"HMCB%d not ready, can't set register!\n", nMCB);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        // WriteLogFile(sz_szInfo);
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVERR, ((long(nMCB))<<16) +MCB_ERRCD_DVNOREADY);
    }
    return nLastError;
}

	//设置触发源并使能DAQ
INT8U CMCBCtrlC::SetTrigCtrlRegAndEnableDAQ(INT32U enable, unsigned short nMCB)
{
    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    INT8U nLastError = 0;
	////////////////////////////////////
	if (m_pDataSocket[nMCB])
	{
        if(0 == nMCB)
        {
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Set Trigger=%s.\n", nMCB, ((0 == nMCB) && m_nTrig) == 0 ? ("0-inner"):("1-outer"));
            sz_szInfo= tmp;
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"HMCB%d Set Trigger=Cascade trigger.\n", nMCB);
            sz_szInfo= tmp;
        }
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
		//////////////////////////////////////////////////////////////////////////////////
		//持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
		ETH_CTRL_read_buffer_residual_data(nMCB);
		//////////////////////////////////////////////////////
		//m_nTrig = trig;
		//MCB--trig source 0 -inner ,1-outer
		//HMCB--trig source 1 -inner ,0-outer
		//EDAQ_daq_req_sel(EDAQ_DAQ_TRIG_SEL_PERIOD_TRIG, nMCB);//选择内触发
		//主控制盒可以工作在内触发或外部脉冲触发
		if(0 == nMCB)
		{
			if(0 == m_nTrig)
			{
				//==nLastError = EDAQ_daq_req_sel(EDAQ_DAQ_TRIG_SEL_PERIOD_TRIG, nMCB);//选择内触发(周期触发)
				nLastError = EDAQ_daq_req_sel_And_mode_enable(enable, EDAQ_DAQ_TRIG_SEL_PERIOD_TRIG, nMCB);//选择内触发(周期触发)
			}
			else
			{
				//==nLastError = EDAQ_daq_req_sel(EDAQ_DAQ_TRIG_SEL_BEAM_PULSE, nMCB);//选择外触发
				nLastError = EDAQ_daq_req_sel_And_mode_enable(enable, EDAQ_DAQ_TRIG_SEL_BEAM_PULSE, nMCB);//选择外触发
				//
				//nLastError = EDAQ_daq_req_sel(EDAQ_DAQ_TRIG_SEL_CASCADE_TRIG, nMCB);//选择级联触发,仅供测试
			}
		}
		else//所有从控制盒都工作在级联触发
		{
			//if(0 == m_nTrig)
			//{
			//	nLastError = EDAQ_daq_req_sel(EDAQ_DAQ_TRIG_SEL_PERIOD_TRIG, nMCB);//选择内触发(周期触发)
			//}
			//else
			//{
			//	nLastError = EDAQ_daq_req_sel(EDAQ_DAQ_TRIG_SEL_BEAM_PULSE, nMCB);//选择外触发
			//}
			//==nLastError = EDAQ_daq_req_sel(EDAQ_DAQ_TRIG_SEL_CASCADE_TRIG, nMCB);//选择级联触发
			nLastError = EDAQ_daq_req_sel_And_mode_enable(enable, EDAQ_DAQ_TRIG_SEL_CASCADE_TRIG, nMCB);//选择级联触发
		}
		//EDAQ_daq_mode_enable(1, nMCB);
		//Sleep(1);
		 usleep(1*1000);
	}
	else
	{			
		nLastError = -1;

        memset(tmp,0,512);
        sprintf(tmp,"HMCB%d not ready, can't set register in SetTrigCtrlReg()!\n", nMCB);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        // WriteLogFile(sz_szInfo);
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVERR, ((long(nMCB))<<16) +MCB_ERRCD_DVNOREADY);
	}
	return nLastError;
}

void CMCBCtrlC::SetDA(DWORD sda, unsigned short nMCB)
{
    std::string sz_szInfo = ("");
    //
    char tmp[512] = {0};
    sprintf(tmp,"HMCB %d Set DA:%08X.\n", nMCB, sda);
    sz_szInfo = tmp;
    //std::cout <<(sz_szInfo);
    WriteLogFile(sz_szInfo);

    //m_reg.regSDAH = (unsigned short)(sda>>16);//DA 高16位
    //m_reg.regSDAL = (unsigned short)(sda);//DA低16位

    //DAC数据值更新
    //功能：向某个地址写入一个32bit数值
    //返回值: 0 = No error, others = Find errors.
    //注意：该函数返回时，可以确保ECS写操作已经执行。
    ECS_A32D32_write_confirmed(CB_DAC_UPDATEDATA_REG_ADDR, sda, nMCB);
}

void CMCBCtrlC::SetTMode(bool bT)
{
    std::string sz_szInfo = ("");
    UINT i = 0;
    UINT j = 0;
    UINT k = 0;
    //
    char tmp[512] = {0};
    sprintf(tmp,"Set Test Mode:%s.\n", (bT)?("TEST"):("Normal"));
    sz_szInfo= tmp;
    //std::cout <<(sz_szInfo);
    WriteLogFile(sz_szInfo);
    //
    for(i =0; i< m_nMCBNum; i++)
    {
        for (UINT j=0; j< m_DMInfo[i].nDMNum; j++)
        {
            if (bT)  //DDC Test mode
            {
                m_nADMCMDREG[i][j] |= 0x08;
            }
            else
            {
                m_nADMCMDREG[i][j] &= 0xfffffff7;
            }
        }
        //////////////////////////////////////////
        //多端口控制板
        for (j=0; j < MAX_DBLINENUM; j++)
        {
            for (k =0; k < MAX_MCBADNUM; k++)
                if (bT)  //DDC Test mode
                {
                    m_nMCBxDByREG[i][j][k] |= 0x08;
                }
                else
                {
                    m_nMCBxDByREG[i][j][k] &= 0xfffffff7;
                }
        }
        //////////////////////////////////////////
        if(m_bEnMCBx4)
        {
            SetDMCmdReg_HMCB4(i);
        }
        else
        {
            SetDMCmdReg(i);
        }
    }
    ////
    //DAQFailToReset();
}

UINT CMCBCtrlC::GetMCBNum(void)
{
    char tmp[512] = {0};
    std::string sz_szInfo = "\n";
    WriteLogFile(sz_szInfo);
    //
    memset(tmp,0,512);
    sprintf(tmp,"HMCB.dll Received a Command from the APP: CMD=0x%04x(%s).\n", MCB_CMD_GETMCBNUM, "MCB_CMD_GETMCBNUM");
    sz_szInfo = tmp;
    WriteLogFile(sz_szInfo);
    ////////////////////////////
    memset(tmp,0,512);
    sprintf(tmp,"HMCB.dll return value to the APP: MCBNUM = %d.\n", m_nMCBNum);
    sz_szInfo = tmp;
    WriteLogFile(sz_szInfo);
    ////////////////////////////
    return UINT(m_nMCBNum);
}

void CMCBCtrlC::GetDMInfo(DMINFO *pDMInfo)
{
    std::string sz_szInfo = ("\n");
    //
    WriteLogFile(sz_szInfo);
    //
    char tmp[512] ={0};
    sprintf(tmp,"HMCB.dll Received a Command from the APP: CMD=0x%04x(%s).\n",MCB_CMD_GETDMINFO, "MCB_CMD_GETDMINFO");
    sz_szInfo = tmp;
    WriteLogFile(sz_szInfo);
    ////////////////////////////
    //参数错误，超出m_nMCBNum个数
    if (pDMInfo->nMCB >= UINT(m_nMCBNum))
    {
        pDMInfo->nDMNum =0;
        pDMInfo->nRowNum =0;
        pDMInfo->nColNum =0;
        pDMInfo->nAllChnNum =0;
    }
    else
    {

        pDMInfo->nDMNum =m_DMInfo[pDMInfo->nMCB].nDMNum;
        pDMInfo->nRowNum =m_DMInfo[pDMInfo->nMCB].nRowNum;
        pDMInfo->nColNum =m_DMInfo[pDMInfo->nMCB].nColNum;
        pDMInfo->nAllChnNum = m_DMInfo[pDMInfo->nMCB].nAllChnNum;

        //多列合一
        if (m_bColCombine) // combine column
        {
            pDMInfo->nColNum =1;
        }
    }
    //
    //std::string sz_szInfo = ("");
    //char tmp[512] = {0};
    memset(tmp,0,512);
    sprintf(tmp,"App Get Detector Informations from dll: nMCB = %d, nDMNum = %d, nRowNum = %d, nColNum = %d, nAllChnNum = %d.\n", pDMInfo->nMCB, pDMInfo->nDMNum, pDMInfo->nRowNum, pDMInfo->nColNum, pDMInfo->nAllChnNum);
    sz_szInfo= tmp;
    WriteLogFile(sz_szInfo);
}

void CMCBCtrlC::GetDMExInfo(DMEXINFO *pDMExInfo)
{
    if(pDMExInfo->nMCB >= UINT(m_nMCBNum))
    {
        pDMExInfo->nLBlkRowNum =0;
        pDMExInfo->nFBlkRowNum =0;
        pDMExInfo->nAllValidChnNum =0;
    }
    else
    {
        pDMExInfo->nFBlkRowNum = 0;
        pDMExInfo->nLBlkRowNum = 0;
        pDMExInfo->nAllValidChnNum = m_DMInfo[pDMExInfo->nMCB].nAllChnNum;
    }
}

void CMCBCtrlC::SetPOTMT(int nTm)
{
    std::string sz_szInfo = "";
    char tmp[512] = {0};
    int nSetPOTMT = nTm;
    ////////////////////////////////////////////////////////////
    //如果nTm参数为0,则从*.ini文件内重新读取POTMT参数
    if(0 == nSetPOTMT)
    {
        CIniFile IniFile;
        IniFile.SetFileName(m_strMCBCtrlIniFile);
        //m_reg.regCtrlA =unsigned char(IniFile.GetInt(str, _T("CTRLA"), (m_nADBit ==24)?0x00:0x02));  //outer trig SE(20bit)/DIFF(24bit)
        //m_reg.regCtrlB =unsigned char(IniFile.GetInt(str, _T("CTRLB"), 0x03));
        nSetPOTMT =IniFile.GetInt(("MODE0"), ("POTMT"), 4000);
        memset(tmp,0,512);
		sprintf(tmp,"MCB_CMD_SETPOTMT command paramerter is 0, then get POTMT parameter value as %d from %s when set POTMT.\n", nSetPOTMT, m_strMCBCtrlIniFile);
        sz_szInfo = tmp;
        WriteLogFile(sz_szInfo);
    }
    ////////////////////////////////////////////////////////////
    //参数值判断，POTM period不能小于1
    if(nSetPOTMT < 100)
    {
        memset(tmp,0,512);
        sprintf(tmp,"POTM period time:%d less than 100. It is too small to Set.\n", nSetPOTMT);
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(0))<<16) + MCB_STSCD_REGSET_ERR);
        return;
    }
    ////////////////////////////////////////////////////////////
    memset(tmp,0,512);
    sprintf(tmp,"Set POTM period:%d.\n", nSetPOTMT);
    sz_szInfo =tmp;
    //std::cout << sz_szInfo;
    WriteLogFile(sz_szInfo);
	//////////////////////////////////////////////////////////////////////////////////
    //新设置值与目前值一样，不需改变，直接返回
	if( m_reg.regPOTMT == nSetPOTMT)
	{
		memset(tmp,0,512);
		sprintf(tmp,"The POTMT parameter is the same as before, and there is no need to update them this time.\n");
		sz_szInfo =tmp;
		//std::cout << sz_szInfo;
		WriteLogFile(sz_szInfo);
		//
		return;
	}
	//////////////////////////////////////////////////////////////////////////////////
	m_reg.regPOTMT = nSetPOTMT;
	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
    //只需设置主控制盒
    //SetReg(0);
	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
	//只需设置主控制盒
	//直接修改内触发间隔参数寄存器值
	unsigned short nMCB = 0;
	unsigned char ret = 0;
	//持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
	ETH_CTRL_read_buffer_residual_data(nMCB);
	//////////////////////////////////////////////////////
    memset(tmp,0,512);
    sprintf(tmp,"End of ETH_CTRL_read_buffer_residual_data() HMCB%d!\n", nMCB);
    sz_szInfo =tmp;
    //std::cout << sz_szInfo;
    WriteLogFile(sz_szInfo);
	//////////////////////////////////////////////////////
	//internal_trigger_period_set(m_reg.regPOTMT, nMCB);//内触发间隔
	//void internal_trigger_period_set(INT32U t, unsigned short nMCB)
	//ECS_A32D32_write_confirmed(INT_TRIG_PERIOD_SET_REG_ADDR, m_reg.regPOTMT, nMCB);
	//////////////////////////////////////////////////////
	INT32U  addr = INT_TRIG_PERIOD_SET_REG_ADDR;
	INT32U  nWriteRegdata = m_reg.regPOTMT;
    ret = ECS_A32D32_write(addr, nWriteRegdata, nMCB);
	//////////////////////////////////////////////////////
    memset(tmp,0,512);
    sprintf(tmp,"End of ECS_A32D32_write(INT_TRIG_PERIOD_SET_REG_ADDR, %d, %d) and Ret=%d of HMCB%d!\n", nWriteRegdata, nMCB, ret, nMCB);
    sz_szInfo =tmp;
    //std::cout << sz_szInfo;
    WriteLogFile(sz_szInfo);
	//////////////////////////////////////////////////////
	//功能：从某个地址读取一个32bit数值
	//返回值: 0 = No error, others = Find errors.
	//注意：该函数返回时，可以确保ECS读操作已经执行。
	unsigned char  buf[8] = { 0x00 };
	INT32U nReadRegValue = 0;
	//重复执行次数
	ret = ETH_CTRL_read(addr, buf, 4, nMCB);
	nReadRegValue = (buf[0]<<24) + (buf[1]<<16) + (buf[2]<<8) + buf[3];
	//////////////////////////////////////////////////////
    memset(tmp,0,512);
    sprintf(tmp,"End ETH_CTRL_read(INT_TRIG_PERIOD_SET_REG_ADDR)  and Ret=%d and return RegValue=%d from HMCB%d!\n", ret, nReadRegValue, nMCB);
    sz_szInfo =tmp;
    //std::cout << sz_szInfo;
    WriteLogFile(sz_szInfo);
	//////////////////////////////////////////////////////
	//如果写与读的数值不一致，则仅重复一遍写
	if(nWriteRegdata != nReadRegValue)
	{
		ret = ECS_A32D32_write(addr, nWriteRegdata, nMCB);
		//////////////////////////////////////////////////////
		//////////////////////////////////////////////////////
		memset(tmp,0,512);
		sprintf(tmp,"End of ECS_A32D32_write(INT_TRIG_PERIOD_SET_REG_ADDR, %d, %d) and Ret=%d of HMCB%d second!\n", nWriteRegdata, nMCB, ret, nMCB);
		sz_szInfo =tmp;
		//std::cout << sz_szInfo;
		WriteLogFile(sz_szInfo);
	}
	/////////////////////////////////////////////////////	/////////////////////////////////////////////////////
    memset(tmp,0,512);
    sprintf(tmp,"End Set POTM period parameter of HMCB%d!\n", nMCB);
    sz_szInfo =tmp;
    //std::cout << sz_szInfo;
    WriteLogFile(sz_szInfo);
}

void CMCBCtrlC::SetPOTMW(int nTm)
{
    std::string sz_szInfo;
    char tmp[512] = {0};
    ////////////////////////////////////////////////////////////
    //参数值判断，POTMT不能小于1
    if(nTm < 1)
    {
        memset(tmp,0,512);
        sprintf(tmp,"POTM width time:%d.is too small to Set.\n", nTm);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(0))<<16) + MCB_STSCD_REGSET_ERR);
        return;
    }
    ////////////////////////////////////////////////////////////
    memset(tmp,0,512);
    sprintf(tmp,"Set POTM width:%d.\n", nTm);
    sz_szInfo= tmp;
    //std::cout <<(sz_szInfo);
    WriteLogFile(sz_szInfo);
    //
    m_reg.regPOTMW = (nTm);
    //只需设置主控制盒
    SetReg(0);
}

void CMCBCtrlC::SetINTTm(int nTm)
{
    std::string sz_szInfo = ("");
    char tmp[512] ={0};
    int nSetINT = nTm;
    ////////////////////////////////////////////////////////////
    //如果nTm参数为0,则从*.ini文件内重新读取POTMT参数
    if(0 == nSetINT)
    {
        CIniFile IniFile;
        IniFile.SetFileName(m_strMCBCtrlIniFile);
        //m_reg.regCtrlA =unsigned char(IniFile.GetInt(str, _T("CTRLA"), (m_nADBit ==24)?0x00:0x02));  //outer trig SE(20bit)/DIFF(24bit)
        //m_reg.regCtrlB =unsigned char(IniFile.GetInt(str, _T("CTRLB"), 0x03));
        nSetINT =IniFile.GetInt(("MODE0"), ("INTT"), 200);
        memset(tmp,0,512);
        sprintf(tmp,"Get INTT parameter value as %d from %s when set INTT.\n", nSetINT, m_strMCBCtrlIniFile);
        sz_szInfo = tmp;
        WriteLogFile(sz_szInfo);
    }
    ////////////////////////////////////////////////////////////
    //参数值判断，INTT不能小于1
    if(nSetINT <1)
    {
        memset(tmp,0,512);
        sprintf(tmp,"INT time:%d.is too small to Set.\n", nSetINT);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(0))<<16) + MCB_STSCD_REGSET_ERR);
        return;
    }
    ////////////////////////////////////////////////////////////
    memset(tmp,0,512);
    sprintf(tmp,"Set INT time:%d.\n", nSetINT);
    sz_szInfo= tmp;
    //std::cout <<(sz_szInfo);
    WriteLogFile(sz_szInfo);

    m_reg.regINTT =nSetINT;
    //只需设置主控制盒
    SetReg(0);
}


void CMCBCtrlC::SetPTFDiv(UINT nFDiv)
{
}

void CMCBCtrlC::SetNewIPAddress(unsigned char * pnNewIP, unsigned short nMCB)
{
    std::string sz_szInfo = ("");
    char tmp[1024] = {0};
    int i = 0;
    ////////////////////////////////////////////////
    if (m_bStart)
    {
        memset(tmp,0,1024);
        sprintf(tmp,"error: HMCB%d is running, can't set New IP Address!\n", nMCB);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, (0<<16) + MCB_STSCD_SETNEWIPRUN);
        return;
    }
    ////////////////////////////////////////////////
    if (!m_bDVReady)
    {
        memset(tmp,0,1024);
        sprintf(tmp,"HMCB%d : not ready, can't set New IP Address!\n", nMCB);
        sz_szInfo= tmp;
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVERR, (0<<16) + MCB_ERRCD_DVNOREADY);
        return;
    }
    ////////////////////////////////////////////////
    else
    {
        UCHAR chIPField[12] = { 0x00 };
        for(i = 0; i < 12; i++)
        {
            chIPField[i] = (UCHAR)(*(pnNewIP+i));
        }
        //--------------------------------------------------------------
        //注意：只有选择使用EEPROM以太网参数，本命令才有意义。
        //--------------------------------------------------------------
        INT8U ret = 0;
        //MAC地址默认为0x000A35002239 + IP地址最后字节
        //INT64U nSetNewMAC_addr = 0x000A35002239 + chIPField[3];//Upper 6 bytes of MAC - Xilinx Ethernet OUI = 00-0A-35
        INT16U nSetNewMAC_addr[4] = { 0 };
        nSetNewMAC_addr[0] = 0x2239 + chIPField[3];//Upper 6 bytes of MAC - Xilinx Ethernet OUI = 00-0A-35
        nSetNewMAC_addr[1] = 0x3500;
        nSetNewMAC_addr[2] = 0x000A;
        //新的IP地址
        INT32U nSetNewIPv4_addr =    (chIPField[0]<<24) + (chIPField[1]<<16) + (chIPField[2]<<8) + chIPField[3];
        //子网掩码
        INT32U nSetNewsubnet_mask =  (chIPField[4]<<24) + (chIPField[5]<<16) + (chIPField[6]<<8) + chIPField[7];
        INT32U nSetNewGateway_addr = (chIPField[8]<<24) + (chIPField[9]<<16) + (chIPField[10]<<8) + chIPField[11];
        INT16U nSetECS_TCP_PORT = 0x400;
        INT16U nSetDATA_TCP_PORT = 0x404;
        //
        ret += (0 == EDAQ_EEPROM_write_zero(0x30, 24, nMCB)) ? 0 : 0x01; //Clear Page 3 - Page 5：全写零。
        ret += (0 == EDAQ_EEPROM_MAC_addr_write(nSetNewMAC_addr, nMCB)) ? 0 : 0x02;
        ret += (0 == EDAQ_EEPROM_IPv4_addr_write(nSetNewIPv4_addr, nMCB)) ? 0 : 0x04;  //192.168.10.32
        ret += (0 == EDAQ_EEPROM_subnet_mask_write(nSetNewsubnet_mask, nMCB)) ? 0 : 0x08;
        ret += (0 == EDAQ_EEPROM_Gateway_addr_write(nSetNewGateway_addr, nMCB)) ? 0 : 0x10;
        ret += (0 == EDAQ_EEPROM_ECS_TCP_PORT_write(nSetECS_TCP_PORT, nMCB)) ? 0 : 0x20;
        ret += (0 == EDAQ_EEPROM_DATA_TCP_PORT_write(nSetDATA_TCP_PORT, nMCB)) ? 0 : 0x40;
        //
        if(0 == ret)
        {
            //sz_szInfo.Format(_T("Configure HMCB%d EEPROM and Set New MAC Address: %02X-%02X-%02X-%02X-%02X-%02X. New IPv4 Address :%d.%d.%d.%d. New Subnet Maks :%d.%d.%d.%d. New Gateway IP :%d.%d.%d.%d.\n"),
            //	nMCB, ((nSetNewMAC_addr & 0xFF0000000000)>>40), ((nSetNewMAC_addr & 0x00FF00000000)>>32), ((nSetNewMAC_addr & 0x0000FF000000)>>24), ((nSetNewMAC_addr & 0x000000FF0000)>>16), ((nSetNewMAC_addr & 0x00000000FF00)>>8), ((nSetNewMAC_addr & 0x0000000000FF)),
            //	chIPField[0], chIPField[1], chIPField[2], chIPField[3], chIPField[4], chIPField[5], chIPField[6], chIPField[7], chIPField[8], chIPField[9], chIPField[10], chIPField[11]);
            memset(tmp,0,512);
            sprintf(tmp,"Configure HMCB%d EEPROM and Set New MAC Address: %02X-%02X-%02X-%02X-%02X-%02X. New IPv4 Address :%d.%d.%d.%d. New Subnet Maks :%d.%d.%d.%d. New Gateway IP :%d.%d.%d.%d.\n",
                    nMCB, ((nSetNewMAC_addr[2] & 0xFF00)>>8), (nSetNewMAC_addr[2] & 0x00FF), ((nSetNewMAC_addr[1] & 0xFF00)>>8), (nSetNewMAC_addr[1] & 0x00FF), ((nSetNewMAC_addr[0] & 0xFF00)>>8), (nSetNewMAC_addr[0] & 0x00FF),
                    chIPField[0], chIPField[1], chIPField[2], chIPField[3], chIPField[4], chIPField[5], chIPField[6], chIPField[7], chIPField[8], chIPField[9], chIPField[10], chIPField[11]);
            sz_szInfo = tmp;
        }
        else
        {
            memset(tmp,0,1024);
            sprintf(tmp,"Fialed to Configure HMCB%d EEPROM and set new Ethernet parameters", nMCB);
            sz_szInfo= tmp;
        }
        //std::cout <<(sz_szInfo);
        WriteLogFile(sz_szInfo);
        //
        //执行如下语句，会使EDAQ FPGA立即从EEPROM中读取以太网参数。当然，如果不执行如下语句，在下次系统上电，也会自动读取EEPROM中的以太网参数。
        //但只有选择使用EEPROM以太网参数，从EEPROM中读取的以太网参数才有效。
        //EDAQ_EEPROM_key_data_onboard_rdreq();	//Request the EDAQ FPGA logic to read Ethernet parameters from EEPROM and upgrade the correspongding internal signals.
        //
        //INT64U nRdMAC_addr = 0;
        INT16U nRdMAC_addr[4] = { 0 };
        INT32U nRdIPv4_addr = 0;
        INT32U nRdsubnet_mask = 0;
        INT32U nRdGateway_addr = 0;
        INT16U nRdECS_TCP_PORT = 0;
        INT16U nRdDATA_TCP_PORT = 0;
        //
        ret = EDAQ_EEPROM_MAC_addr_read(nMCB, nRdMAC_addr);
        nRdIPv4_addr = EDAQ_EEPROM_IPv4_addr_read(nMCB);
        nRdsubnet_mask = EDAQ_EEPROM_subnet_mask_read(nMCB);
        nRdGateway_addr = EDAQ_EEPROM_Gateway_addr_read(nMCB);
        nRdECS_TCP_PORT = EDAQ_EEPROM_ECS_TCP_PORT_read(nMCB);
        nRdDATA_TCP_PORT = EDAQ_EEPROM_DATA_TCP_PORT_read(nMCB);
        if((nSetNewMAC_addr[0] != nRdMAC_addr[0]) || (nSetNewMAC_addr[1] != nRdMAC_addr[1]) ||(nSetNewMAC_addr[2] != nRdMAC_addr[2]) ||(nSetNewMAC_addr[3] != nRdMAC_addr[3]) ||
            (nSetNewIPv4_addr != nRdIPv4_addr) || (nSetNewsubnet_mask != nRdsubnet_mask) ||
            (nSetNewGateway_addr != nRdGateway_addr) || (nSetECS_TCP_PORT != nRdECS_TCP_PORT) || (nSetDATA_TCP_PORT != nRdDATA_TCP_PORT))
        {
            memset(tmp,0,1024);
            sprintf(tmp,"Ethernet parameters from EEPROM not equal to the set value. EEPROM MAC address: 0x%04X%04X%04X, IPv4 address: 0x%08X, SUBNET MASK: 0x%08X, Gateway IP addr: 0x%08X, ECS TCP PORT: 0x%04X, DATA TCP PORT: 0x%04X\n",
                    nRdMAC_addr[2], nRdMAC_addr[1], nRdMAC_addr[0], nRdIPv4_addr, nRdsubnet_mask, nRdGateway_addr, nRdECS_TCP_PORT, nRdDATA_TCP_PORT);
            sz_szInfo= tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
            SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, (0<<16) + MCB_STSCD_SETNEWIP_ERR);
        }
        else
        {
            memset(tmp,0,1024);
            sprintf(tmp,"HMCB%d successfully set the new IP address to %d.%d.%d.%d.\n", nMCB, chIPField[0], chIPField[1], chIPField[2], chIPField[3]);
            sz_szInfo= tmp;
            //std::cout <<(sz_szInfo);
            WriteLogFile(sz_szInfo);
            SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, (0<<16) + MCB_STSCD_SETNEWIP_OK);
            //执行如下语句，会使EDAQ FPGA立即从EEPROM中读取以太网参数。当然，如果不执行如下语句，在下次系统上电，也会自动读取EEPROM中的以太网参数。
            //但只有选择使用EEPROM以太网参数，从EEPROM中读取的以太网参数才有效。
            EDAQ_EEPROM_key_data_onboard_rdreq(nMCB);	//Request the EDAQ FPGA logic to read Ethernet parameters from EEPROM and upgrade the correspongding internal signals.
        }
    }
}

void CMCBCtrlC::WriteLogFile(std::string lpsz, UINT nLogLevel /*=0*/)
{

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    if (!m_bCLogPath) //log file error
    {
        return;
    }
    //////////////////////////////////////////////////////
    if(nLogLevel > m_nLogLevel)
    {
        return;
    }
    //////////////////////////////////////////////////////
    char str[512] = {0};
    time_t t_st;
    time(&t_st);
    struct tm * timeinfo = localtime(&t_st);
    sprintf(str,"%d-%02d-%02d %02d:%02d:%02d.%03d  ",
            timeinfo->tm_year + 1900, timeinfo->tm_mon +1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, /*t_st.wMilliseconds*/0);
    //////////////////////////////////////////////////////
    std::string szfilenamebak = ("");
    std::string szfilenamebakold = ("");
    ///////////////////////////////////////////////////
    //若没有设置日志路径,则取当前路径
    if(!m_bSetLogFilePath)
    {
       // if (m_file.GetLength() > 20*1024*1024)
		 if (m_file.GetLength() > LOGFILE_SIZE)
        {
            szfilenamebak =  m_szPath;
            szfilenamebakold = m_szPath;
            szfilenamebak = "\\HMCBCtrlBak.log";
            szfilenamebakold += "\\HMCBCtrlBakOld.log";
            //
            CopyFile(szfilenamebak, szfilenamebakold, FALSE);
            CopyFile(m_filename, szfilenamebak, FALSE);
            m_file.SetLength(0);
        }
        else
        {
            m_file.SeekToEnd();
        }
        m_file.WriteString(str + lpsz);
        m_file.Flush();
    }
    //若设置了日志路径,则取设置值吗，每日一个日志文件
    else
    {
        //创建数据文件夹
        std::string lpszDefName = ("");
        lpszDefName.Format(("%s\\HMCBCtrl.%04d-%02d-%02d.log"), m_strLogFilePath, t_st.wYear, t_st.wMonth, t_st.wDay);
        //
        if(m_filename.Compare(lpszDefName) != 0)
        {
            m_filename = lpszDefName;
            if(m_bCLogPath)
            {
                m_file.Close();
                m_bCLogPath = FALSE;
            }
            //打开文件
            locale loc = locale::global(locale("")); //要打开的文件路径含中文，设置全局locale为本地环境
            //
            if (m_file.Open(m_filename, CFile::modeWrite |CFile::modeCreate |CFile::modeNoTruncate
                                            |CFile::typeText |CFile::shareDenyWrite ) ==0) //
            {
                std::cout <<(("open LOG file error!\n"));
                m_bCLogPath =FALSE;
            }
            else
            {
                m_file.SeekToEnd();
                m_bCLogPath =true;
            }
            //打开文件
            locale::global(loc);//恢复全局locale</span>
        }
        //
        if(m_bCLogPath) //log file error
        {
            //str.Format(("%d-%02d-%02d %02d:%02d:%02d.%03d  "), t_st.wYear, t_st.wMonth, t_st.wDay, t_st.wHour, t_st.wMinute, t_st.wSecond, t_st.wMilliseconds);
            m_file.WriteString(str + lpsz);
            m_file.Flush();
        }
    }
#else
    unique_lock<mutex> lock(_mutex);
    //std::cout << lpsz <<std::endl;
    if (!m_bCLogPath) //log file error
    {
        lock.unlock();
        return;
    }
    //////////////////////////////////////////////////////
    if(nLogLevel > m_nLogLevel)
    {
        lock.unlock();
        return;
    }
    //////////////////////////////////////////////////////
    char str[512] = {0};
    time_t t_st;
    time(&t_st);
    struct tm * timeinfo = localtime(&t_st);
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    sprintf(str,"%d-%02d-%02d %02d:%02d:%02d.%03d  ",
            timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, ts.tv_nsec/1000000);
    //////////////////////////////////////////////////////
    std::string szfilenamebak = ("");
    std::string szfilenamebakold = ("");
    ///////////////////////////////////////////////////
    //若没有设置日志路径,则取当前路径
    if(!m_bSetLogFilePath)
    {
#if __cplusplus >= 201703L

        if(!std::filesystem::exists(m_filename))
        {
            std::cout << " file exit error " << std::endl;
            m_file.open(m_filename,std::ios::out|std::ios::app);
        }
       // if (std::filesystem::file_size(m_filename) > 20*1024*1024)
        if (std::filesystem::file_size(m_filename) > LOGFILE_SIZE)
		{
#else
        struct stat statbuf;
        stat(m_filename.c_str(), &statbuf);
        // 获取文件大小
        size_t filesize = statbuf.st_size;
        //if(filesize > 20*1024*1024)
		if(filesize > LOGFILE_SIZE)
        {
#endif
            m_file.close();
            szfilenamebak =  m_szPath;
            szfilenamebakold = m_szPath;
            szfilenamebak += "/HMCBCtrlBak.log";
            szfilenamebakold += "/HMCBCtrlBakOld.log";
            //

#if __cplusplus >= 201703L
            if(std::filesystem::exists(szfilenamebak))
            {
                       std::filesystem::remove(szfilenamebakold);
                       std::filesystem::copy_file(szfilenamebak,szfilenamebakold);

            }
            if(std::filesystem::exists(m_filename))
            {
                       std::filesystem::remove(szfilenamebak);
                        std::filesystem::copy_file(m_filename, szfilenamebak);
            }
#else
                       std::string strCpyCmd1 = "cp " + szfilenamebak + " " + szfilenamebakold;
                       popen(strCpyCmd1.c_str(),"r");
                       usleep(50*1000);
                       std::string strCpyCmd2 = "cp " + m_filename + " " + szfilenamebak;
                       popen(strCpyCmd2.c_str(),"r");
#endif

                        usleep(50*1000);
                        m_file.open(m_filename, std::ios::out|std::ios::trunc);
                        usleep(50*1000);
        }
        else
        {
            //m_file.seekg(0,std::ios_base::end);
        }
        m_file << (str + lpsz);
        m_file.flush();
    }
    //若设置了日志路径,则取设置值吗，每日一个日志文件
    else
    {
        //创建数据文件夹
        std::string lpszDefName = ("");
        memset(str,0,512);
        sprintf(str,("%s/HMCBCtrl.%04d-%02d-%02d.log"), m_strLogFilePath.c_str(), timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);
        lpszDefName = str;
        //
        if(m_filename !=lpszDefName)
        {
            m_filename = lpszDefName;
            if(m_bCLogPath)
            {
                m_file.close();
                m_bCLogPath = false;
            }
            //打开文件
            locale loc = locale::global(locale("")); //要打开的文件路径含中文，设置全局locale为本地环境
            //

            m_file.open(m_filename ,std::ios::out|std::ios::app);
            if ( !m_file.is_open()) //
            {
                std::cout <<(("open LOG file error!  1 \n"));
                std::cout << m_filename << std::endl;
                m_bCLogPath =false;
            }
            else
            {
                m_file.seekg(0,std::ios_base::end);
                m_bCLogPath =true;
            }
            //打开文件
            locale::global(loc);//恢复全局locale</span>
        }
        //
        if(m_bCLogPath) //log file error
        {
            //str.Format(("%d-%02d-%02d %02d:%02d:%02d.%03d  "), t_st.wYear, t_st.wMonth, t_st.wDay, t_st.wHour, t_st.wMinute, t_st.wSecond, t_st.wMilliseconds);

            struct timeval tv;
            struct timezone tz;
            struct tm *t;

            gettimeofday(&tv, &tz);
            t = localtime(&tv.tv_sec);
                //printf("%d-%d-%d %d:%d:%d.%ld\n", 1900+t->tm_year, 1+t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec);
            sprintf(str,"%d-%02d-%02d %02d:%02d:%02d.%ld  ", 1900+t->tm_year, 1+t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec);
            m_file << (std::string(str) + lpsz);
            m_file.flush();
        }
    }
#endif
    lock.unlock();
}

void CMCBCtrlC::WriteDataErrorLogFile(std::string lpsz, UINT nMCB)
{
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    if (!m_bDataErrorCLogPath[nMCB]) //log file error
    {
        //打开文件
        locale loc = locale::global(locale("")); //要打开的文件路径含中文，设置全局locale为本地环境
        //
        if (m_DataErrorfile[nMCB].Open(m_DataErrorfilename[nMCB], CFile::modeWrite |CFile::modeCreate |CFile::modeNoTruncate
                                                                      |CFile::typeText |CFile::shareDenyWrite ) ==0) //
        {
            std::cout <<(("open DataError LOG file error!\n"));
            m_bDataErrorCLogPath[nMCB] = FALSE;
            return;
        }
        m_DataErrorfile[nMCB].SeekToEnd();
        m_bDataErrorCLogPath[nMCB] =true;
        //打开文件
        locale::global(loc);//恢复全局locale</span>
    }
    //////////////////////////////////////////////////////
    std::string str;
    SYSTEMTIME t_st;
    GetLocalTime(&t_st);
    str.Format(("%d-%02d-%02d %02d:%02d:%02d.%03d  "), t_st.wYear, t_st.wMonth, t_st.wDay, t_st.wHour, t_st.wMinute, t_st.wSecond, t_st.wMilliseconds);
    //////////////////////////////////////////////////////
    std::string szfilenamebak = ("");
    std::string szfilenamebakold = ("");
    ///////////////////////////////////////////////////
    //若没有设置日志路径,则取当前路径
    if(!m_bSetLogFilePath)
    {
       // if (m_DataErrorfile[nMCB].GetLength() > 20*1024*1024)
		 if (m_DataErrorfile[nMCB].GetLength() > LOGFILE_SIZE)
        {
            szfilenamebak.Format(("%s\\HMCB%dCtrlDataErrorBak.log"), m_szPath, nMCB);
            szfilenamebakold.Format(("%s\\HMCB%dCtrlDataErrorBakOld.log"), m_szPath, nMCB);
            //
            CopyFile(szfilenamebak, szfilenamebakold, FALSE);
            CopyFile(m_DataErrorfilename[nMCB], szfilenamebak, FALSE);
            m_DataErrorfile[nMCB].SetLength(0);
        }
        else
        {
            m_DataErrorfile[nMCB].SeekToEnd();
        }
        //m_DataErrorfile.WriteString(lpsz);
        m_DataErrorfile[nMCB].WriteString(str + lpsz);
        m_DataErrorfile[nMCB].Flush();
    }
    //若设置了日志路径,则取设置值
    else
    {
        //创建数据文件夹
        std::string lpszDefName = ("");
        lpszDefName.Format(("%s\\HMCB%dCtrl.%04d-%02d-%02dDataError.log"), m_strLogFilePath, nMCB, t_st.wYear, t_st.wMonth, t_st.wDay);
        //
        if(m_DataErrorfilename[nMCB].Compare(lpszDefName) != 0)
        {
            m_DataErrorfilename[nMCB] = lpszDefName;
            if(m_bDataErrorCLogPath[nMCB])
            {
                m_DataErrorfile[nMCB].Close();
                m_bDataErrorCLogPath[nMCB] = FALSE;
            }
            //打开文件
            locale loc = locale::global(locale("")); //要打开的文件路径含中文，设置全局locale为本地环境
            //
            if (m_DataErrorfile[nMCB].Open(m_DataErrorfilename[nMCB], CFile::modeWrite |CFile::modeCreate |CFile::modeNoTruncate
                                                                          |CFile::typeText |CFile::shareDenyWrite ) ==0) //
            {
                std::cout <<(("open LOG file error!\n"));
                m_bDataErrorCLogPath[nMCB] =FALSE;
            }
            else
            {
                m_DataErrorfile[nMCB].SeekToEnd();
                m_bDataErrorCLogPath[nMCB] =true;
            }
            //打开文件
            locale::global(loc);//恢复全局locale</span>
        }
        //
        if(m_bDataErrorCLogPath[nMCB]) //log file error
        {
            //str.Format(("%d-%02d-%02d %02d:%02d:%02d.%03d  "), t_st.wYear, t_st.wMonth, t_st.wDay, t_st.wHour, t_st.wMinute, t_st.wSecond, t_st.wMilliseconds);
            //m_DataErrorfile.WriteString(lpsz);
            m_DataErrorfile[nMCB].WriteString(str + lpsz);
            m_DataErrorfile[nMCB].Flush();
        }
    }
#else
    if (!m_bDataErrorCLogPath[nMCB]) //log file error
    {
        //打开文件
        locale loc = locale::global(locale("")); //要打开的文件路径含中文，设置全局locale为本地环境
        //
        m_DataErrorfile[nMCB].open(m_DataErrorfilename[nMCB], std::ios::out|std::ios::trunc);
        if (! m_DataErrorfile[nMCB].is_open()) //
        {
            std::cout <<(("open DataError LOG file error!\n"));
            m_bDataErrorCLogPath[nMCB] = false;
            return;
        }
        m_DataErrorfile[nMCB].seekg(0,std::ios_base::end);
        m_bDataErrorCLogPath[nMCB] =true;
        //打开文件
        locale::global(loc);//恢复全局locale</span>
    }
    //////////////////////////////////////////////////////
    std::string str;
    char tmp[512] = {0};
    time_t t_st;
    struct tm * timeinfo = localtime(&t_st);
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    memset(tmp,0,512);
    sprintf(tmp,"%d-%02d-%02d %02d:%02d:%02d.%03d  ", timeinfo->tm_year +1900, timeinfo->tm_mon + 1 , timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, ts.tv_nsec/1000000);
    str = tmp;
    //////////////////////////////////////////////////////
    std::string szfilenamebak = ("");
    std::string szfilenamebakold = ("");
    ///////////////////////////////////////////////////
    //若没有设置日志路径,则取当前路径
    if(!m_bSetLogFilePath)
    {
#if __cplusplus >= 201703L

        //if (std::filesystem::file_size(m_DataErrorfilename[nMCB]) > 20*1024*1024)
		if (std::filesystem::file_size(m_DataErrorfilename[nMCB]) > LOGFILE_SIZE)
        {
#else
        struct stat statbuf;
        stat(m_DataErrorfilename[nMCB].c_str(), &statbuf);
        // 获取文件大小
        size_t filesize = statbuf.st_size;
       // if(filesize > 20*1024*1024)
		 if(filesize > LOGFILE_SIZE)
        {
#endif
            memset(tmp,0,512);
            sprintf(tmp,"%s\\HMCB%dCtrlDataErrorBak.log", m_szPath, nMCB);
            szfilenamebak = tmp;
            memset(tmp,0,512);
            sprintf(tmp,"%s\\HMCB%dCtrlDataErrorBakOld.log", m_szPath, nMCB);
            szfilenamebakold = tmp;
            //
#if __cplusplus >= 201703L
            std::filesystem::copy_file(szfilenamebak,szfilenamebakold);
            std::filesystem::copy_file(m_DataErrorfilename[nMCB], szfilenamebak);
#else
            std::string strCpyCmd1 = "cp " + szfilenamebak + " " + szfilenamebakold;
            popen(strCpyCmd1.c_str(),"r");
            std::string strCpyCmd2 = "cp " + m_DataErrorfilename[nMCB] + " " + szfilenamebak;
            popen(strCpyCmd2.c_str(),"r");
#endif
            m_DataErrorfile[nMCB].close();
            m_DataErrorfile[nMCB].open(m_DataErrorfilename[nMCB], std::ios::out|std::ios::trunc);

        }
        else
        {
            m_DataErrorfile[nMCB].seekg(0,std::ios_base::end);
        }
        //m_DataErrorfile.WriteString(lpsz);
        m_DataErrorfile[nMCB]<< (str + lpsz);
        m_DataErrorfile[nMCB].flush();
    }
    //若设置了日志路径,则取设置值
    else
    {
        //创建数据文件夹
        std::string lpszDefName = ("");
        memset(tmp,0,512);
        sprintf(tmp,"%s/HMCB%dCtrl.%04d-%02d-%02dDataError.log", m_strLogFilePath.c_str(), nMCB, timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);
        lpszDefName = tmp;
        //
        if(m_DataErrorfilename[nMCB]!=lpszDefName)
        {
            m_DataErrorfilename[nMCB] = lpszDefName;
            if(m_bDataErrorCLogPath[nMCB])
            {
                m_DataErrorfile[nMCB].close();
                m_bDataErrorCLogPath[nMCB] = false;
            }
            //打开文件
            locale loc = locale::global(locale("")); //要打开的文件路径含中文，设置全局locale为本地环境
            //
            m_DataErrorfile[nMCB].open(m_DataErrorfilename[nMCB],std::fstream::in|std::fstream::out/*std::ios::in|std::ios::trunc*/);
            if (!m_DataErrorfile[nMCB].is_open()) //
            {
                std::cout <<(("open LOG file error! 2 \n"));
                std::cout << m_DataErrorfilename[nMCB] << std::endl;
                m_bDataErrorCLogPath[nMCB] =false;
            }
            else
            {
                m_DataErrorfile[nMCB].seekg(0,std::ios_base::end);
                m_bDataErrorCLogPath[nMCB] =true;
            }
            //打开文件
            locale::global(loc);//恢复全局locale</span>
        }
        //
        if(m_bDataErrorCLogPath[nMCB]) //log file error
        {
            //str.Format(("%d-%02d-%02d %02d:%02d:%02d.%03d  "), t_st.wYear, t_st.wMonth, t_st.wDay, t_st.wHour, t_st.wMinute, t_st.wSecond, t_st.wMilliseconds);
            //m_DataErrorfile.WriteString(lpsz);
            m_DataErrorfile[nMCB]<< (str + lpsz);
            m_DataErrorfile[nMCB].flush();
        }
    }
#endif
}

void CMCBCtrlC::SaveExdata()
{
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    static int n_filesn =0;
    std::string str, str1;
    CStdioFile file;

    str.Format(("EXData_%d.csv"), (n_filesn++%10));

    if (m_bPath)
    {
        m_filenameExd.Format(("%s\\"), m_szPath);
        m_filenameExd +=str;
        if (file.Open(m_filenameExd, CFile::modeWrite |CFile::modeCreate
                                         |CFile::typeText |CFile::shareDenyWrite ) ==0) //
        {
            str1.Format(("Open Exdata file %s error!\n"), str);
            std::cout <<(str1);
            WriteLogFile(str1);
            return;
        }
        else
        {
            str1.Format(("Write Exdata file %s.\n"), str);
            std::cout <<(str1);
            WriteLogFile(str1);
        }
        file.WriteString(("S_NO.,P_NO.,DI(0-3 BIN),EXDATA(64Bit HEX)\n"));
        if (m_scounter[0] >INVALID_DATA_PACKET_NUM)
        {
            for (unsigned long i=0; i< min(m_scounter[0]-INVALID_DATA_PACKET_NUM-1, MAXEXDATANUM); i++)
            {
                str.Format(("%d,%d,%d%d%d%d,%08x%08x\n"), m_InfoExdata[0][i], m_InfoExdata[1][i],
                           m_InfoExdata[2][i]&0x1, (m_InfoExdata[2][i]>>1)&0x1, (m_InfoExdata[2][i]>>2)&0x1, (m_InfoExdata[2][i]>>3)&0x1,
                           m_InfoExdata[3][i], m_InfoExdata[4][i]);
                file.WriteString(str);
            }
        }
        file.Close();
    }
#else
    static int n_filesn =0;
    std::string str, str1;
    std::fstream file;

    char tmp[512] = {0};
    sprintf(tmp,"EXData_%d.csv", (n_filesn++%10));
    str = tmp;

    if (m_bPath)
    {
        memset(tmp,0,512);
        sprintf(tmp,"%s\\", m_szPath);
        m_filenameExd = tmp;
        m_filenameExd +=str;
        file.open(m_filenameExd, std::ios::out|std::ios::trunc);
        if (!file.is_open()) //
        {
            memset(tmp,0,512);
            sprintf(tmp,"Open Exdata file %s error!\n", str.c_str());
            str1 = tmp;
            //std::cout <<(str1);
            WriteLogFile(str1);
            return;
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"Write Exdata file %s.\n", str.c_str());
            str1 = tmp;
            //std::cout <<(str1);
            WriteLogFile(str1);
        }
        file << "S_NO.,P_NO.,DI(0-3 BIN),EXDATA(64Bit HEX)\n";
        if (m_scounter[0] >INVALID_DATA_PACKET_NUM)
        {
            for (unsigned long i=0; i< min(m_scounter[0]-INVALID_DATA_PACKET_NUM-1, (unsigned long)MAXEXDATANUM); i++)
            {
                memset(tmp,0,512);
                sprintf(tmp,"%d,%d,%d%d%d%d,%08x%08x\n", m_InfoExdata[0][i], m_InfoExdata[1][i],
                        m_InfoExdata[2][i]&0x1, (m_InfoExdata[2][i]>>1)&0x1, (m_InfoExdata[2][i]>>2)&0x1, (m_InfoExdata[2][i]>>3)&0x1,
                        m_InfoExdata[3][i], m_InfoExdata[4][i]);
                str = tmp;
                file<<(str);
            }
        }
        file.close();
    }
#endif
}

void CMCBCtrlC::SetSendAllData(int nSAD)
{
    std::string sz_szInfo = "";

    m_bSendAllData = (nSAD!=0);
    char tmp[512] = {0};
    sprintf(tmp,"%s",m_bSendAllData?("1-Send all data mode."):("0-Normal mode."));
    sz_szInfo = tmp;
    //std::cout << ("Set Send Data Mode: ") << sz_szInfo << std::endl;
    WriteLogFile(("Set Send Data Mode: ")+ sz_szInfo +("\n"));

}
//多线程解析数据
void CMCBCtrlC::ReadParseData(ULONG pkg_num)
{
    std::string sz_szInfo = "";
    sz_szInfo = "Data Thread_Enter ReadParseData(). \n";
    WriteLogFile(sz_szInfo);
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    for (int i = 0; i < m_nMCBNum; i++)
    {
        if( 0 == i)
        {

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            RECVPARAM *pRecvParam0 = new RECVPARAM;
            pRecvParam0->nMCBIndex = index;
            pRecvParam0->pkg_num = pkg_num;
            CreateThread(NULL, 0, ETH_read_and_parse_dataEx_Thread, (void *) pRecvParam0, 0, NULL);
#else \
    //std::thread th(ETH_read_and_parse_dataEx_Thread, (void *) pRecvParam0);
            std::thread th([=]{
                RECVPARAM *pRecvParam0 = new RECVPARAM;
                pRecvParam0->nMCBIndex = 0;
                pRecvParam0->pkg_num = pkg_num;
                ETH_read_and_parse_dataEx_Thread(pRecvParam0);
            });
            th.detach();
#endif
        }
        if( 1 == i)
        {
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            CreateThread(NULL, 0, ETH_read_and_parse_dataEx_Thread, (void *) pRecvParam1, 0, NULL);
#else
            std::thread th([=]{
                RECVPARAM *pRecvParam1 = new RECVPARAM;
                pRecvParam1->nMCBIndex = 1;
                pRecvParam1->pkg_num = pkg_num;
                ETH_read_and_parse_dataEx_Thread(pRecvParam1);
            });
            th.detach();
#endif
        }
        if( 2 == i)
        {

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            CreateThread(NULL, 0, ETH_read_and_parse_dataEx_Thread, (void *) pRecvParam2, 0, NULL);
#else
            std::thread th([=]{
                RECVPARAM *pRecvParam2 = new RECVPARAM;
                pRecvParam2->nMCBIndex = 2;
                pRecvParam2->pkg_num = pkg_num;
                ETH_read_and_parse_dataEx_Thread(pRecvParam2);
            });
            th.detach();
#endif

        }
        if( 3 == i)
        {
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            CreateThread(NULL, 0, ETH_read_and_parse_dataEx_Thread, (void *) pRecvParam3, 0, NULL);
#else
            std::thread th([=]{
                RECVPARAM *pRecvParam3 = new RECVPARAM;
                pRecvParam3->nMCBIndex = 3;
                pRecvParam3->pkg_num = pkg_num;
                ETH_read_and_parse_dataEx_Thread(pRecvParam3);
            });
            th.detach();
#endif

        }
        if( 4 == i)
        {
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            CreateThread(NULL, 0, ETH_read_and_parse_dataEx_Thread, (void *) pRecvParam4, 0, NULL);
#else
            std::thread th([=]{
                RECVPARAM *pRecvParam4 = new RECVPARAM;
                pRecvParam4->nMCBIndex = 4;
                pRecvParam4->pkg_num = pkg_num;
                ETH_read_and_parse_dataEx_Thread(pRecvParam4);
            });
            th.detach();
#endif

        }
        if( 5 == i)
        {

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            CreateThread(NULL, 0, ETH_read_and_parse_dataEx_Thread, (void *) pRecvParam5, 0, NULL);
#else
            std::thread th([=]{
                RECVPARAM *pRecvParam5 = new RECVPARAM;
                pRecvParam5->nMCBIndex = 5;
                pRecvParam5->pkg_num = pkg_num;
                ETH_read_and_parse_dataEx_Thread(pRecvParam5);
            });
            th.detach();
#endif

        }
        if( 6 == i)
        {
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            CreateThread(NULL, 0, ETH_read_and_parse_dataEx_Thread, (void *) pRecvParam6, 0, NULL);
#else
            std::thread th([=]{
                RECVPARAM *pRecvParam6 = new RECVPARAM;
                pRecvParam6->nMCBIndex = 6;
                pRecvParam6->pkg_num = pkg_num;
                ETH_read_and_parse_dataEx_Thread(pRecvParam6);
            });
            th.detach();
#endif
        }
        if( 7 == i)
        {
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            CreateThread(NULL, 0, ETH_read_and_parse_dataEx_Thread, (void *) pRecvParam7, 0, NULL);
#else
            std::thread th([=]{
                RECVPARAM *pRecvParam7 = new RECVPARAM;
                pRecvParam7->nMCBIndex = 7;
                pRecvParam7->pkg_num = pkg_num;
                ETH_read_and_parse_dataEx_Thread(pRecvParam7);
            });
            th.detach();
#endif
        }
    }
}

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
DWORD WINAPI CMCBCtrlC::ETH_read_and_parse_dataEx_Thread(void * lpParameter)
#else
DWORD  CMCBCtrlC::ETH_read_and_parse_dataEx_Thread(void * lpParameter)
#endif
{
    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    //对应的MCB索引
    INT8U nMCB = ((RECVPARAM *)lpParameter)->nMCBIndex;
    //一次采集需要获取的触发数据包数量
    //0:无限包数
    //>0:需要目标包数
    ULONG nTarget_pkg_num = ((RECVPARAM *)lpParameter)->pkg_num;

//    memset(tmp,0,512);
//    sprintf(tmp," ETH_read_and_parse_dataEx_Thread : nTarget_pkg_num is %d . \n", nTarget_pkg_num);
//    sz_szInfo = tmp;
//    WriteLogFile(sz_szInfo);

    delete lpParameter;
    //
    if(nMCB >= MAX_MCBNUM )
    {
        memset(tmp,0,512);
        sprintf(tmp,"HMCB Index Parameter of ETH_read_and_parse_dataEx_Thread function is %d, bigger than MAX_MCBNUM = %d. \n", nMCB, MAX_MCBNUM);
        sz_szInfo = tmp;
        WriteLogFile(sz_szInfo);
        return 0;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////
//    memset(tmp,0,512);
//    sprintf(tmp,"Enter HMCB%d ETH_read_and_parse_dataEx_Thread.\n", nMCB);
//    sz_szInfo = tmp;
//    //WriteLogFile(sz_szInfo, gLogLevel1);
//    WriteLogFile(sz_szInfo);
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    INT32S i = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //开始数据解析
    ULONGLONG buffDataLength = 0;
    //
    OneDBLineOnePackage pDBDataPackage;
    //
    INT32U rdAddr0, rdAddr1, rdAddr2, rdAddr3, rdAddr4, rdAddr5, rdAddr6, rdAddr7, rdAddr8, rdAddr9, rdAddr10;
    INT8U  buf_data, buf_data1, buf_data2, buf_data3, buf_data4, buf_data5, buf_data6, buf_data7, buf_data8, buf_data9, buf_data10;
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    INT16U SC_pkg_lead_code = 0;  //引导码0xAA 0x55
    INT8U  SC_pkg_rev = 0x00;//保留字节
    INT32U SC_pkg_len = 0;//数据包总长度3字节
    INT8U  SC_pkg_db_line = 0;//DB line index(0~3)
    INT8U  SC_pkg_type = 0;	//CMD code类型
    INT16U SC_pkg_index = 0;//数据包编号
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    INT8U DB_board_index = 0;
    INT8U Max_DB_board_index = 0;
    INT8U overall_err = 0;	//总错误
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    INT8U  xB_pkg_type  = 0;//数据包类型
    INT16U xB_pkg_size = 0;//数据包长度低8位+高4位
    INT8U  xB_pkg_index  = 0;//数据包编号
    INT8U  xB_pkg_board_i  = 0;//探测器板编号(CB or DB板)
    INT8U  xB_pkg_logic_err1  = 0;//控制接口逻辑状态
    INT8U  xB_pkg_logic_err2  = 0;//其他逻辑状态
    INT8U  xB_pkg_sel = DB_PKG_PARSE_SEL; //表示当前解析的是DB还是CB数据包。
    //错误检查
    INT8U pkg_parse_err = 0;	//数据包解析错误，非FPGA逻辑错误。一旦出现该类型错误，重置解析状态机。
    INT8U pkg_data_err = 0;		//包解析正常，但发现包内容有一些异常
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    INT8U  parse_mode = PM_START; //数据包解析状态
    INT32U nHeadBytes = 10;//数据包头字节数
    INT32U nxB_DataPackBytes = 0;//CB or DB包字节数
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool tB1 = true;
    //采集已开始或 解析结束
    if(m_bStart)
    //while((m_bStart))
    {
        //对于定包数采集，如果所有控制盒解析的包数量达到目标，则自动发停止采集命令
        if(nTarget_pkg_num > 0)
        {
            tB1 = true;
            for (i = 0; i < m_nMCBNum; i++)
            {
                //任意一个控制盒没有采到足够数据，则不自动停止
                if(parsed_pkg_n[i]  < (nTarget_pkg_num /*+ INVALID_DATA_PACKET_NUM*/) )
                {
                    tB1 = false;
                }
            }
            if((tB1) && (!m_bPreStop))
            {

                memset(tmp,0,512);
                sprintf(tmp," parsed_pkg_n[i] : %d  nTarget_pkg_num : %d \n",parsed_pkg_n[i],nTarget_pkg_num);
                sz_szInfo = tmp;
                WriteLogFile(sz_szInfo);
                Stop(1,nMCB);
                //break;
                return 0;
            }
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        WaitForSingleObject(hMutex, INFINITE);
#else \
    // todo ssx 2023年10月13日17:29:04
     // unique_lock<mutex> lock(m_Data_mutex[nMCB]);
#endif


        ////////////////////////////////////////////////////////////////////////////////////////////////////
        //总接收数据字节数多于总读出处理字节数，存在未处理字节
        //if(glb_data_AccWrBytes[nMCB] > glb_data_AccRdBytes[nMCB])
        //{
        //	buffDataLength = glb_data_AccWrBytes[nMCB] - glb_data_AccRdBytes[nMCB];
        //}
        //else
        //{
        //	buffDataLength = 0;
        //}
        buffDataLength = glb_data_buf_vetor[nMCB].size();
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        //sz_szInfo.Format(("Enter MCB%d ETH_read_and_parse_dataEx_Thread. glb_data_AccWrBytes[nMCB]= %I64u; glb_data_AccRdBytes[nMCB]=%I64u; and buffDataLength = %I64u;\n"),
        //	nMCB, glb_data_AccWrBytes[nMCB], glb_data_AccRdBytes[nMCB], buffDataLength);
        //WriteLogFile(sz_szInfo, gLogLevel2);
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        //缓存区内有大于等于一次触发的未处理数据
        while(buffDataLength >= 2*gbl_DBLine_Size[nMCB][0])
        {
			////////////////////////////////////////////////////////////////////////////////////////////////////
			//对于定包数采集，如果控制盒解析的包数量达到目标，则停止继续解析数据
			if(nTarget_pkg_num > 0)
			{
				//任意一个控制盒没有采到足够数据，则不自动停止
				if(parsed_pkg_n[nMCB] >= nTarget_pkg_num)
				{
					glb_data_buf_vetor[nMCB].clear();//清空当前的vector里面数据
					buffDataLength = glb_data_buf_vetor[nMCB].size();
					break;
				}
			}
			////////////////////////////////////////////////////////////////////////////////////////////////////
            //pDBDataPackage数据清0
            memset(&pDBDataPackage, 0, sizeof(pDBDataPackage));
            //每次数据解析起始地址
            i = 0;
            buf_data1 = glb_data_buf_vetor[nMCB].at(i++);
            buf_data2 = glb_data_buf_vetor[nMCB].at(i++);
            buf_data3 = glb_data_buf_vetor[nMCB].at(i++);//reserve保留, 暂补零
            buf_data4 = glb_data_buf_vetor[nMCB].at(i++);//数据包总长度(高8位)
            buf_data5 = glb_data_buf_vetor[nMCB].at(i++);//数据包总长度(中8位)
            buf_data6 = glb_data_buf_vetor[nMCB].at(i++);//数据包总长度(低8位)
            buf_data7 = glb_data_buf_vetor[nMCB].at(i++);//DB line index(0~3)
            buf_data8 = glb_data_buf_vetor[nMCB].at(i++);//CMD code
            buf_data9 = glb_data_buf_vetor[nMCB].at(i++);//数据包编号(高8位)
            buf_data10 = glb_data_buf_vetor[nMCB].at(i++);//数据包编号(低8位)
            ///////////////////////////////////////////////////////////////////
            //rdAddr0 = gbl_data_buf_rdaddr[nMCB] % GLB_DATA_BUF_SIZE;
            ////
            //rdAddr1 = rdAddr0;
            //rdAddr2 = (rdAddr1 + 1) % GLB_DATA_BUF_SIZE;
            ////rdAddr3 = (rdAddr1 + 2) % GLB_DATA_BUF_SIZE;//reserve保留, 暂补零
            //rdAddr4 = (rdAddr1 + 3) % GLB_DATA_BUF_SIZE;
            //rdAddr5 = (rdAddr1 + 4) % GLB_DATA_BUF_SIZE;
            //rdAddr6 = (rdAddr1 + 5) % GLB_DATA_BUF_SIZE;
            //rdAddr7 = (rdAddr1 + 6) % GLB_DATA_BUF_SIZE;
            //rdAddr8 = (rdAddr1 + 7) % GLB_DATA_BUF_SIZE;
            //rdAddr9 = (rdAddr1 + 8) % GLB_DATA_BUF_SIZE;
            //rdAddr10 = (rdAddr1 + 9) % GLB_DATA_BUF_SIZE;
            //////////////////////////////////////////////////////////////////////////////////////////////////////
            //buf_data1 = glb_data_buf[nMCB][rdAddr1];
            //buf_data2 = glb_data_buf[nMCB][rdAddr2];
            ////buf_data3 = glb_data_buf[rdAddr3];//reserve保留, 暂补零
            //buf_data4 = glb_data_buf[nMCB][rdAddr4];//数据包总长度(高8位)
            //buf_data5 = glb_data_buf[nMCB][rdAddr5];//数据包总长度(中8位)
            //buf_data6 = glb_data_buf[nMCB][rdAddr6];//数据包总长度(低8位)
            //buf_data7 = glb_data_buf[nMCB][rdAddr7];//DB line index(0~3)
            //buf_data8 = glb_data_buf[nMCB][rdAddr8];//CMD code
            //buf_data9 = glb_data_buf[nMCB][rdAddr9];//数据包编号(高8位)
            //buf_data10 = glb_data_buf[nMCB][rdAddr10];//数据包编号(低8位)
            //引导码0xAA 0x55
            SC_pkg_lead_code = (buf_data1<<8) + buf_data2;
            //数据包总长度3字节
            SC_pkg_len = (buf_data4<<16) + (buf_data5<<8) + (buf_data6);
            //DB line index(0~3)
            SC_pkg_db_line = buf_data7;
            //有效性判断
            SC_pkg_db_line = (SC_pkg_db_line < (MAX_DBLINENUM - 1)) ? SC_pkg_db_line : (MAX_DBLINENUM - 1);
            //CMD code类型
            SC_pkg_type = buf_data8;
            //数据包编号
            SC_pkg_index = (buf_data9<<8) + (buf_data10);
            //////////////////////////////////////////////////////////////
            //数据包头正确性判断
            //开始解析一个SC数据包
            if(0xAA55 != SC_pkg_lead_code)
            {
                //10000次解析错位打印一条记录
                m_nMissAA55Cnt[nMCB]++;
                if(1 == (m_nMissAA55Cnt[nMCB] % 10000))
                {
                    //m_nMissAA55Cnt = 0;
                    memset(tmp,0,512);
                    sprintf(tmp,"Not find SC_PKG_LEAD_CODE(0xAA55) when PM_GET_SC_PKG_HEADER with MCB%d.\n", nMCB);
                    sz_szInfo = tmp;
                    WriteLogFile(sz_szInfo);
                }
                if(m_nMissAA55Cnt[nMCB] > 65535*2)
                {
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
                    ReleaseMutex(hMutex);
                    //////////////////////////////////////////////////////////////////
                    LeaveCriticalSection(&ics);
                    //////////////////////////////////////////////////////////////////
#else

#endif
                    memset(tmp,0,512);
                    sprintf(tmp,"Not find SC_PKG_LEAD_CODE(0xAA55) more than 65535*2 times and Exit HMCB%d ETH_read_and_parse_dataEx_Thread.\n", nMCB);
                    sz_szInfo = tmp;
                    WriteLogFile(sz_szInfo);
                    //////////////////////////////////////////////////////////////////
                    //上一次数据采集是否成功完成标志：失败
                    m_bPreDAQSucessFlag[nMCB] = false;
                    //////////////////////////////////////////////////////////////////
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    //lock.unlock();
                    return 0;
                }
                //////////////////////////////////////////////////////////////
                //数据格式不对，数据指针前移一位
                glb_data_buf_vetor[nMCB].erase(glb_data_buf_vetor[nMCB].begin(), glb_data_buf_vetor[nMCB].begin()+1);
                buffDataLength = glb_data_buf_vetor[nMCB].size();
                //
                //gbl_data_buf_rdaddr[nMCB] = (rdAddr0 + 1) % GLB_DATA_BUF_SIZE;
                //glb_data_AccRdBytes[nMCB] += 1;
                //if(glb_data_AccWrBytes[nMCB] > glb_data_AccRdBytes[nMCB])
                //{
                //	buffDataLength = glb_data_AccWrBytes[nMCB] - glb_data_AccRdBytes[nMCB];
                //}
                //else
                //{
                //	buffDataLength = 0;
                //}
                //////////////////////////////////////////////////////////////
                continue;
            }
            //CMD code 与 数据包总长度长度判断
            else if((DB_DAQ_PKG != SC_pkg_type) || (SC_pkg_len != gbl_DBLine_Size[nMCB][0]))
            {
                memset(tmp,0,512);
                sprintf(tmp,"Get Error when parse PM_GET_SC_PKG_HEADER data of HMCB%d, return data length is : %d, need data length is %d. \n", nMCB, SC_pkg_len, gbl_DBLine_Size[nMCB][0]);
                sz_szInfo = tmp;
                WriteLogFile(sz_szInfo, gLogLevel2);
                //////////////////////////////////////////////////////////////
                //数据格式不对，数据指针前移一位
                glb_data_buf_vetor[nMCB].erase(glb_data_buf_vetor[nMCB].begin(), glb_data_buf_vetor[nMCB].begin()+1);
                buffDataLength = glb_data_buf_vetor[nMCB].size();
                //gbl_data_buf_rdaddr[nMCB] = (rdAddr0 + 1) % GLB_DATA_BUF_SIZE;
                //glb_data_AccRdBytes[nMCB] += 1;
                //if(glb_data_AccWrBytes[nMCB] > glb_data_AccRdBytes[nMCB])
                //{
                //	buffDataLength = glb_data_AccWrBytes[nMCB] - glb_data_AccRdBytes[nMCB];
                //}
                //else
                //{
                //	buffDataLength = 0;
                //}
                //////////////////////////////////////////////////////////////
                continue;
            }
            //正确找到数据包头
            else
            {
                //一帧数据头
                pDBDataPackage.nMCB = nMCB;
                pDBDataPackage.DB_line_i = SC_pkg_db_line;
                pDBDataPackage.rx_DB_line_i = SC_pkg_db_line;
                pDBDataPackage.SC_pkg_type = SC_pkg_type;
                pDBDataPackage.SC_pkg_index = SC_pkg_index;
                ////////////////////////////////////////////////////
                //读取整个SC包数据的载荷
                //正确解析出数据头，数据指针前移10字节
                nHeadBytes = 10;
                glb_data_buf_vetor[nMCB].erase(glb_data_buf_vetor[nMCB].begin(), glb_data_buf_vetor[nMCB].begin()+nHeadBytes);
                buffDataLength = glb_data_buf_vetor[nMCB].size();
                //gbl_data_buf_rdaddr[nMCB] = (rdAddr0 + nHeadBytes) % GLB_DATA_BUF_SIZE;
                //glb_data_AccRdBytes[nMCB] += nHeadBytes;
                //if(glb_data_AccWrBytes[nMCB] > glb_data_AccRdBytes[nMCB])
                //{
                //	buffDataLength = glb_data_AccWrBytes[nMCB] - glb_data_AccRdBytes[nMCB];
                //}
                //else
                //{
                //	buffDataLength = 0;
                //}
                //////////////////////////////////////////////////////////////
                //rdAddr0 = gbl_data_buf_rdaddr[nMCB];
                rdAddr0 = 0;
                nxB_DataPackBytes = 0;
                parse_mode = PM_GET_XB_DATA;
                //
                while(PM_GET_XB_DATA == parse_mode)
                {
                    //开始解析一个DB板 或 最后CB板 的Data包
                    pkg_parse_err = 0;
                    //
                    rdAddr1 = rdAddr0;
                    //xB_pkg_type  = glb_data_buf[nMCB][rdAddr1 % GLB_DATA_BUF_SIZE];//数据包类型
                    xB_pkg_type  = glb_data_buf_vetor[nMCB].at(rdAddr1);
                    //
                    rdAddr1++;
                    //xB_pkg_size = glb_data_buf[nMCB][rdAddr1 % GLB_DATA_BUF_SIZE];//数据包长度低8位
                    xB_pkg_size  = glb_data_buf_vetor[nMCB].at(rdAddr1);
                    //
                    rdAddr1++;
                    //buf_data = glb_data_buf[nMCB][rdAddr1 % GLB_DATA_BUF_SIZE];//(高4bit数据包编号+低4bit数据长度高4位)
                    buf_data  = glb_data_buf_vetor[nMCB].at(rdAddr1);
                    //
                    xB_pkg_size = ((buf_data&0x0F)<<8) + xB_pkg_size;//数据包长度低8位+高4位
                    xB_pkg_index  = (buf_data&0xF0)>>4;//数据包编号
                    //
                    rdAddr1++;
                    //xB_pkg_board_i = glb_data_buf[nMCB][rdAddr1 % GLB_DATA_BUF_SIZE];//探测器板编号(CB or DB板)
                    xB_pkg_board_i  = glb_data_buf_vetor[nMCB].at(rdAddr1);
					xB_pkg_board_i  = min((int)xB_pkg_board_i, (MAX_MCBADNUM-1));
                    //
                    rdAddr1++;
                    //xB_pkg_logic_err1 = glb_data_buf[nMCB][rdAddr1 % GLB_DATA_BUF_SIZE]; //控制接口逻辑状态
                    xB_pkg_logic_err1  = glb_data_buf_vetor[nMCB].at(rdAddr1);
                    //
                    rdAddr1++;
                    //xB_pkg_logic_err2 = glb_data_buf[nMCB][rdAddr1 % GLB_DATA_BUF_SIZE]; //其他逻辑状态
                    xB_pkg_logic_err2  = glb_data_buf_vetor[nMCB].at(rdAddr1);
                    /////////////////////////////////////////////////////////////////////
                    //包类型
                    if ((xB_pkg_type == DB_ERR_PKG) || (xB_pkg_type == SC_pkg_type))
                    {
                        xB_pkg_sel = DB_PKG_PARSE_SEL; //表示当前解析的是DB还是CB数据包
                        parse_mode = PM_GET_XB_DATA;
                        /////////////////////////////////////////
                        //保存最后一次解析的DB数据包前6个字节
                        /////////////////////////////////////////
                    }
                    else if (xB_pkg_type == CB_PKG)//CB包
                    {
                        xB_pkg_sel = CB_PKG_PARSE_SEL;//表示当前解析的是DB还是CB数据包
                        parse_mode = PM_GET_XB_DATA;
                        //

                    }
                    else
                    {
                        xB_pkg_sel = UNKNOWN_PKG_PARSE_SEL;
                        parse_mode = PM_GET_SUB_PKG_HEADER;
                        //////////////////////////////////////////////////////////////
                        //数据格式不对，数据指针前移一位
                        glb_data_buf_vetor[nMCB].erase(glb_data_buf_vetor[nMCB].begin(), glb_data_buf_vetor[nMCB].begin()+1);
                        buffDataLength = glb_data_buf_vetor[nMCB].size();
                        //gbl_data_buf_rdaddr[nMCB] = (rdAddr0 + 1) % GLB_DATA_BUF_SIZE;
                        //rdAddr0 = gbl_data_buf_rdaddr[nMCB];
                        //glb_data_AccRdBytes[nMCB] += 1;
                        ////
                        //if(glb_data_AccWrBytes[nMCB] > glb_data_AccRdBytes[nMCB])
                        //{
                        //	buffDataLength = glb_data_AccWrBytes[nMCB] - glb_data_AccRdBytes[nMCB];
                        //}
                        //else
                        //{
                        //	buffDataLength = 0;
                        //}
                        //
                        parse_mode = PM_START;
                        //////////////////////////////////////////////////////////////
                        break;
                    }
                    /////////////////////////////////////////////////////////////////////
                    //DB数据包(DB单板单命令执行返回的数据包)
                    if(DB_PKG_PARSE_SEL == xB_pkg_sel)//DB包
                    {
                        DB_board_index = xB_pkg_board_i;
                        pDBDataPackage.DB_pkg_type[DB_board_index] = xB_pkg_type;
                        pDBDataPackage.DB_pkg_index[DB_board_index] = xB_pkg_index;
                        pDBDataPackage.DB_board_index[DB_board_index] = DB_board_index;
                        Max_DB_board_index = (Max_DB_board_index > DB_board_index) ? Max_DB_board_index : DB_board_index;
                        pDBDataPackage.Max_DB_board = Max_DB_board_index + 1;//DB数量(1个起)
                        pDBDataPackage.DB_logic_err1[DB_board_index] = xB_pkg_logic_err1;
                        pDBDataPackage.DB_logic_err2[DB_board_index] = xB_pkg_logic_err2;
                        pDBDataPackage.DB_data_len[DB_board_index] = xB_pkg_size;
                        //
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
                        for(i = 0; i < __min(4096, xB_pkg_size); i++)
#else
                        for(i = 0; i < min(4096, (int)xB_pkg_size); i++)
#endif
                            {
                                rdAddr1++;
                                //pDBDataPackage.DB_data[DB_board_index][i] = glb_data_buf[nMCB][rdAddr1 % GLB_DATA_BUF_SIZE];
                                pDBDataPackage.DB_data[DB_board_index][i] = glb_data_buf_vetor[nMCB].at(rdAddr1);
                            }
                        //校验码：CRC-8，对包中所有数据进行校验
                        rdAddr1++;
                        //pDBDataPackage.DB_crc_code[DB_board_index] = glb_data_buf[nMCB][rdAddr1 % GLB_DATA_BUF_SIZE];
                        pDBDataPackage.DB_crc_code[DB_board_index] = glb_data_buf_vetor[nMCB].at(rdAddr1);
                        //一个DB包size = 6字节头+ xB_pkg_size个数据+1个CRC
                        nxB_DataPackBytes = 6 + xB_pkg_size + 1;
                        //////////////////////////////////////////////////////////////
                        //数据格式对，数据指针前移一个包字节数
                        glb_data_buf_vetor[nMCB].erase(glb_data_buf_vetor[nMCB].begin(), glb_data_buf_vetor[nMCB].begin() + nxB_DataPackBytes);
                        buffDataLength = glb_data_buf_vetor[nMCB].size();
                        //gbl_data_buf_rdaddr[nMCB] = (rdAddr0 + nxB_DataPackBytes) % GLB_DATA_BUF_SIZE;
                        //rdAddr0 = gbl_data_buf_rdaddr[nMCB];
                        //glb_data_AccRdBytes[nMCB] += nxB_DataPackBytes;
                        ////
                        //if(glb_data_AccWrBytes[nMCB] > glb_data_AccRdBytes[nMCB])
                        //{
                        //	buffDataLength = glb_data_AccWrBytes[nMCB] - glb_data_AccRdBytes[nMCB];
                        //}
                        //else
                        //{
                        //	buffDataLength = 0;
                        //}
                        //////////////////////////////////////////////////////////////
                    }
                    //CB数据包(CB单板单命令执行返回的数据包)
                    else if(CB_PKG_PARSE_SEL == xB_pkg_sel)
                    {
                        pDBDataPackage.CB_pkg_type = xB_pkg_type;
                        pDBDataPackage.CB_pkg_index = xB_pkg_index;
                        pDBDataPackage.CB_board_index = xB_pkg_board_i;
                        pDBDataPackage.CB_data_len = xB_pkg_size;
                        pDBDataPackage.CB_logic_err1 = xB_pkg_logic_err1;
                        pDBDataPackage.CB_logic_err2 = xB_pkg_logic_err2;
                        //CB包 拷贝CB Data数据（12字节）
                        //12个字节 CB Data数据
                        //Data 0（DB line独立）：高4位传输SDCLK采集错误类型，低4位传输SDCLK采集宽度。
                        //Data 1（DB line独立）：系统设计保留。
                        //Data 2：高4位已用于传输DDI；低4位保留，全为0。系统保留。
                        //Data 3：系统设计保留。
                        //Data 4～11: 留给用户使用。
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
                        for(i = 0; i < __min(16, xB_pkg_size); i++)
#else
                        for(i = 0; i < min(16, (int)xB_pkg_size); i++)
#endif
                        {
                            rdAddr1++;
                                //pDBDataPackage.CB_data[i] = glb_data_buf[nMCB][rdAddr1 % GLB_DATA_BUF_SIZE];
                                pDBDataPackage.CB_data[i] = glb_data_buf_vetor[nMCB].at(rdAddr1);
                        }
                        //校验码：CRC-8，对包中所有数据进行校验
                        rdAddr1++;
                        //pDBDataPackage.CB_crc_code = glb_data_buf[nMCB][rdAddr1 % GLB_DATA_BUF_SIZE];
                        pDBDataPackage.CB_crc_code = glb_data_buf_vetor[nMCB].at(rdAddr1);
                        //一个CB包size = 6字节头+ xB_pkg_size个数据+1个CRC
                        nxB_DataPackBytes = 6 + xB_pkg_size + 1;
                        //////////////////////////////////////////////////////////////
                        //数据格式对，数据指针前移一个包字节数
                        glb_data_buf_vetor[nMCB].erase(glb_data_buf_vetor[nMCB].begin(), glb_data_buf_vetor[nMCB].begin() + nxB_DataPackBytes);
                        buffDataLength = glb_data_buf_vetor[nMCB].size();
                        //gbl_data_buf_rdaddr[nMCB] = (rdAddr0 + nxB_DataPackBytes) % GLB_DATA_BUF_SIZE;
                        //rdAddr0 = gbl_data_buf_rdaddr[nMCB];
                        //glb_data_AccRdBytes[nMCB] += nxB_DataPackBytes;
                        ////
                        //////////////////////////////////////////////////////////////////////////////////////////////////////
                        //if(glb_data_AccWrBytes[nMCB] > glb_data_AccRdBytes[nMCB])
                        //{
                        //	buffDataLength = glb_data_AccWrBytes[nMCB] - glb_data_AccRdBytes[nMCB];
                        //}
                        //else
                        //{
                        //	buffDataLength = 0;
                        //}
                        ////////////////////////////////////////////////////////////////////////////////////////////////////
                        //待确认
                        //sz_szInfo.Format(_T("At end trigger_%d data analyse of HMCB%d, gbl_data_buf_rdaddr[nMCB] = %d, glb_data_AccWrBytes[nMCB]= %I64u, glb_data_AccRdBytes[nMCB]=%I64u,  buffDataLength = %I64u\n"),
                        //	pDBDataPackage.SC_pkg_index, nMCB, gbl_data_buf_rdaddr[nMCB], glb_data_AccWrBytes[nMCB], glb_data_AccRdBytes[nMCB], buffDataLength);
                        //WriteLogFile(sz_szInfo, gLogLevel2);
                        ////////////////////////////////////////////////////////////////////////////////////////////////////
                        //执行到这里，说明解析完一个数据包
                        parse_mode = PM_START;
                        ////////////////////////////////////////////////////////////////////////////////////////////////////
                        ////////////////////////////////////////////////////////////////////////////////////////////////////
                        parsed_pkg_n[nMCB]++;//已经正确解析的数据包个数递增1
                        //获取保存每一个HMCB返回的第一个数据包
                        if(1 == parsed_pkg_n[nMCB])
                        {
                            SC_1st_pkg_index[nMCB] = pDBDataPackage.SC_pkg_index;
                            ////每一个DB Line返回的第一个数据包内触发计数
                            SC_1st_pkg_TriggCnt[nMCB] = (pDBDataPackage.CB_data[8]<<24)+(pDBDataPackage.CB_data[9]<<16)+(pDBDataPackage.CB_data[10]<<8)+(pDBDataPackage.CB_data[11]);// CountPos & CountNeg(16bit + 16bit)
                        }
                        ////////////////////////////////////////////////////////////////////////////////////////////////////
                        //每一个HMCB返回的当前数据包索引(已考虑数据包溢出重新计数)
                        //控制盒返回数据包编号2个字节:0~65535
                        SC_current_pkg_index[nMCB] += (pDBDataPackage.SC_pkg_index + 65536 - SC_pre_pkg_index[nMCB]) % 65536;
                        //
                        SC_pre_pkg_index[nMCB] = pDBDataPackage.SC_pkg_index;
                        //控制板返回的硬件触发数
                        pDBDataPackage.SC_pkg_index = SC_current_pkg_index[nMCB] - SC_1st_pkg_index[nMCB] + 1;
                        ////////////////////////////////////////////////////////////////////////////////////
                        //判断解析后的前6个通道数据，若为0xA5则判断为本次采集的首个数据包
                        //首个数据包丢弃，后续数据包正常上传
                        if( (0xA5 == pDBDataPackage.DB_data[0][0]) &&
                            (0xA5 == pDBDataPackage.DB_data[0][1]) &&
                            (0xA5 == pDBDataPackage.DB_data[0][2]) &&
                            (0xA5 == pDBDataPackage.DB_data[0][3]) &&
                            (0xA5 == pDBDataPackage.DB_data[0][4]) &&
                            (0xA5 == pDBDataPackage.DB_data[0][5]))
                        {
                            parsed_pkg_n[nMCB] = 0;//已经解析的数据包个数
                            Update_pkg_n[nMCB] = 0;//已经解析且发送App更新的数据包个数
                            SC_1st_pkg_index[nMCB] = 0;//每一个DB Line返回的第一个数据包
                            //
                            SC_1st_pkg_TriggCnt[nMCB] = 0;//每一个DB Line返回的第一个数据包内触发计数
                            //SC_Current_pkg_TriggCnt[i] = 0;//每一个DB Line返回的当前数据包内触发计数
                            //
                            SC_pre_pkg_index[nMCB] = 0;//每一个DB Line返回的前一个数据包索引
                            SC_current_pkg_index[nMCB] = 0;//每一个DB Line返回的当前数据包索引
                            //
                            ////////////////////////////////////////////////////////////////////////////////////////////////////

                            memset(tmp,0,512);
                            sprintf(tmp,"Get 0xA5 data packet from HMCB%d. \n", nMCB);
                            sz_szInfo = tmp;
                            WriteLogFile(sz_szInfo);
                            //
                            //是否已获取到开始的0xA5数据包
                            m_bGet0xA5Packet[nMCB] = true;
                        }
                        if(!m_bGet0xA5Packet[nMCB])
                        {
                            //获取到0xA5数据包前已解析出的数据包
                            m_nMiss0XA5PacketCnt[nMCB]++;
                            if(1 == (m_nMiss0XA5PacketCnt[nMCB] % 100))
                            {
                                memset(tmp,0,512);
                                sprintf(tmp,"HMCB%d has received %d packets, but has not received 0xA5 packets.\n", nMCB, m_nMiss0XA5PacketCnt[nMCB]);
                                sz_szInfo = tmp;
                                WriteLogFile(sz_szInfo);
                            }
                        }
                        ////////////////////////////////////////////////////////////////////////////////////
                        //是否要求获取开始的0xA5数据包，默认需要获取
                        if(!m_bNeedGet0xA5)
                        {
                            m_bGet0xA5Packet[nMCB] = true;
                        }
                        //parsed_pkg_n : 1次取数中软件触发计数，从0开始
                        //前面(INVALID_DATA_PACKET_NUM - m_nDiscardNum[nMCB])个触发舍弃，不传数据给APP

						//前面(INVALID_DATA_PACKET_NUM - m_nDiscardNum[nMCB])个触发舍弃，不传数据给APP
						if((m_bGet0xA5Packet[nMCB]) && ((parsed_pkg_n[nMCB] + m_nDiscardNum[nMCB]) > INVALID_DATA_PACKET_NUM))
                        //if((m_bGet0xA5Packet[nMCB]) && (parsed_pkg_n[nMCB] > (INVALID_DATA_PACKET_NUM - m_nDiscardNum[nMCB])))
                        //if(parsed_pkg_n[nMCB] > INVALID_DATA_PACKET_NUM - m_nDiscardNum[NMCB])
                        {
                            pDBDataPackage.ADFormat_Bit = m_nADBit;
                            pDBDataPackage.nSocketCounter = parsed_pkg_n[nMCB] - INVALID_DATA_PACKET_NUM + m_nDiscardNum[nMCB];//软件包裹计数,1开始;
                            pDBDataPackage.overall_err = overall_err;
                            ////////////////////////////////////////////////////////////////////////////////////////////////////
                            //只有当DB存在且第一个DB有DAQ数据包，才往应用程序发送数据更新消息
                            if((pDBDataPackage.Max_DB_board > 0) && (DB_DAQ_PKG == pDBDataPackage.DB_pkg_type[0]))
                            {
                                ////////////////////////////////////////////////////////////////////////////////////////////////////
                                memset(tmp,0,512);
                                sprintf(tmp,"HMCB%d Update One Packet Data (No. %d) To App.\n", nMCB, Update_pkg_n[nMCB]);
                                sz_szInfo = tmp;
                                WriteLogFile(sz_szInfo, gLogLevel2);
								////////////////////////////////////////////////////////////////////////////////////////////////////
								//2024.05.14 前面几次采集，主控制盒的钩铛信号强制置0
								if((nMCB == 0) && (Update_pkg_n[nMCB]< 2))
								{
									//pDBDataPackage.CB_data[2] = pDBDataPackage.CB_data[2] & 0xBF;//CB_data[2]：高4位用于传输DDI（高位-低位依次为DI3,DI2DI1,DI0，对应备用-钩铛-高低能-触发
									//DI是反逻辑，置0需要写1
									pDBDataPackage.CB_data[2] = pDBDataPackage.CB_data[2] | 0x40;//CB_data[2]：高4位用于传输DDI（高位-低位依次为DI3,DI2DI1,DI0，对应备用-钩铛-高低能-触发
								}
								////////////////////////////////////////////////////////////////////////////////////////////////////
                                UpdateOnePacketDataToApp(&pDBDataPackage);
                                Update_pkg_n[nMCB]++;
                                // 正确解析一包数据后，m_nMissAA55Cnt计数清零
                                m_nMissAA55Cnt[nMCB]=0;
                                ////////////////////////////////////////////////////////////////////////////////////////////////////
                                //数据采集是否成功完成标志
                                m_bPreDAQSucessFlag[nMCB] = true;
                            }
                        }
                        //��������������m_bAdjAinDi��1������adjust AIN DI...
                        else
                        {
                            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                            //2024-01-08������������������������������������������������������������
                            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                            UINT nError_DI = ((~pDBDataPackage.CB_data[2])& 0xF0)<<4;//��������;//Data 2����4����������DDI
                            if(m_bGoudang_S4)
                            {
                                nError_DI = (nError_DI & 0x0B00) + ((nError_DI&0x0200)<< 1);//������������������������
                            }
                            //����������������������
                            long nPulseCounter = (pDBDataPackage.CB_data[8]<<24)+(pDBDataPackage.CB_data[9]<<16)+(pDBDataPackage.CB_data[10]<<8)+(pDBDataPackage.CB_data[11]);
                            nPulseCounter = nPulseCounter - SC_1st_pkg_TriggCnt[nMCB] - INVALID_DATA_PACKET_NUM + m_nDiscardNum[nMCB] + 1;
                            //
                            m_MCBDataHeadTmp[nMCB].head_use.scounter = pDBDataPackage.nSocketCounter;//nSocketCounter;	// socket packet serial number, self-add//������������,1����
                            m_MCBDataHeadTmp[nMCB].head_use.pcounter.l = nPulseCounter;//nPulseCounter;	// pulse trigger number//����������������������
                            m_MCBDataHeadTmp[nMCB].head_use.accEnergy = ((nError_DI&0x0200) ==0) ? 1 : 0;// acc energy sign
                            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                            //CB��������������12������������,12��������
                            //Data 0��DB line����������4������SDCLK����������������4������SDCLK����������
                            //Data 1��DB line����������������������
                            //Data 2����4������������DDI����4������������0������������
                            //Data 3����������������
                            //Data 4��11: ��������������
                            //m_MCBDataHeadTmp[nMCB].d_head.head_use.ain0.ul =		(pDBDataPackage.CB_data[4]<<24)+(pDBDataPackage.CB_data[5]<<16)+(pDBDataPackage.CB_data[6]<<8)+(pDBDataPackage.CB_data[7]);// 24bit ADC��(����24bit + 8Bit (0x07))
                            m_MCBDataHeadTmp[nMCB].head_use.ain0.ul =		(pDBDataPackage.CB_data[4]<<16)+(pDBDataPackage.CB_data[5]<<8)+(pDBDataPackage.CB_data[6]);//24bit ADC��(����24bit + 8Bit (0x07))
                            m_MCBDataHeadTmp[nMCB].head_use.ptsmdata_h =	(pDBDataPackage.CB_data[0]<<24)+(pDBDataPackage.CB_data[1]<<16)+(pDBDataPackage.CB_data[2]<<8)+(pDBDataPackage.CB_data[3]);// ����0��3����������
                            //m_MCBDataHeadTmp[nMCB].d_head.head_use.ptsmdata_l =	(pDBDataPackage.CB_data[8]<<24)+(pDBDataPackage.CB_data[9]<<16)+(pDBDataPackage.CB_data[10]<<8)+(pDBDataPackage.CB_data[11]);// CountPos(32bit)
                            m_MCBDataHeadTmp[nMCB].head_use.ptsmdata_l = nPulseCounter + INVALID_DATA_PACKET_NUM;
                            //
                            m_MCBDataHeadTmp[nMCB].head_use.ain1.ul = 0;				// ain1 data(UINT)
                            m_MCBDataHeadTmp[nMCB].head_use.sscnt = Update_pkg_n[nMCB];	//nPulseCounter + INVALID_DATA_PACKET_NUM;		// socket packet serial number, from packet
                            m_MCBDataHeadTmp[nMCB].head_use.regError_DI = nError_DI;
                        }
                    }
                    //std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
            //std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        ReleaseMutex(hMutex);
#else
    //lock.unlock();
    //std::this_thread::sleep_for(std::chrono::milliseconds(1));
#endif
    }
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    //////////////////////////////////////////////////////////////////
    LeaveCriticalSection(&ics);
    ////////////////////////////////////////////////////////////////////////////////////////////////////
#else
#endif
//    memset(tmp,0,512);
//    sprintf(tmp,"Exit HMCB%d ETH_read_and_parse_dataEx_Thread. \n", nMCB);
//    sz_szInfo = tmp;
//    WriteLogFile(sz_szInfo);

    return 0;
}


void CMCBCtrlC::UpdateOnePacketDataToApp(OneDBLineOnePackage *pDBDataPackage)
{

    ////////////////////////////////////////////////////
    static int num_call =0;
    if(num_call % 100 == 0)
    {
        WriteLogFile(" call function  UpdateOnePacketDataToApp  times : " + std::to_string(num_call));
    }
    num_call++;
    ////////////////////////////////////////////////////
    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    //unsigned short nMCB = 0;
    //INT16U i = 0;
    INT16U i = 0, j = 0, k = 0, l = 0;
    ///////////////////////////////////////////////
    ///////////////////////////////////////
    unsigned short nMCB = pDBDataPackage->nMCB;
    //unsigned short nMCB = pDBDataPackage->DB_line_i;
    nMCB = (nMCB < (MAX_MCBNUM - 1)) ? nMCB : (MAX_MCBNUM - 1);
    ///////////////////////////////////////////////
    ///////////////////////////////////////////////
    //拷贝数据状态到m_pOneDBLineOnePackageStatue
    m_pOneDBLineOnePackageStatue[nMCB].nMCB = pDBDataPackage->nMCB;
    m_pOneDBLineOnePackageStatue[nMCB].ADFormat_Bit = pDBDataPackage->ADFormat_Bit;//AD转换位数

    m_pOneDBLineOnePackageStatue[nMCB].nSocketCounter = pDBDataPackage->nSocketCounter;//软件包裹计数;
    m_pOneDBLineOnePackageStatue[nMCB].overall_err = pDBDataPackage->overall_err;	//总错误
    //
    m_pOneDBLineOnePackageStatue[nMCB].DB_line_i = pDBDataPackage->DB_line_i;//DB_line索引(0~3)
    m_pOneDBLineOnePackageStatue[nMCB].DB_line_num = pDBDataPackage->DB_line_num;//有效db_line(1,2,4)
    m_pOneDBLineOnePackageStatue[nMCB].cmd_code = pDBDataPackage->cmd_code;//是否 Check the package type
    m_pOneDBLineOnePackageStatue[nMCB].cmd_mismatched_pkg_skip_en = pDBDataPackage->cmd_mismatched_pkg_skip_en;//1 = 如果SC包头的cmd不符，会跳过本数据包检查下一个数据包，直到发现cmd匹配的数据包，并完成数据包解析
    //
    m_pOneDBLineOnePackageStatue[nMCB].SC_pkg_header = pDBDataPackage->SC_pkg_header;  //定位DWord （0x00FF55AA）
    m_pOneDBLineOnePackageStatue[nMCB].rx_DB_line_i = pDBDataPackage->rx_DB_line_i;	//当前所接收到数据的DB_line索引(0~3)
    m_pOneDBLineOnePackageStatue[nMCB].SC_pkg_type = pDBDataPackage->SC_pkg_type;	//
    //INT16U SC_pkg_index;
    m_pOneDBLineOnePackageStatue[nMCB].SC_pkg_index = pDBDataPackage->SC_pkg_index;
    //
    for(i = 0; i < MAX_MCBADNUM; i++)
    {
        m_pOneDBLineOnePackageStatue[nMCB].DB_pkg_type[i] = pDBDataPackage->DB_pkg_type[i];
        m_pOneDBLineOnePackageStatue[nMCB].DB_pkg_index[i] = pDBDataPackage->DB_pkg_index[i];
        m_pOneDBLineOnePackageStatue[nMCB].DB_board_index[i] = pDBDataPackage->DB_board_index[i];
        m_pOneDBLineOnePackageStatue[nMCB].DB_logic_err1[i] = pDBDataPackage->DB_logic_err1[i];
        m_pOneDBLineOnePackageStatue[nMCB].DB_logic_err2[i] = pDBDataPackage->DB_logic_err2[i];
        m_pOneDBLineOnePackageStatue[nMCB].DB_data_len[i] = pDBDataPackage->DB_data_len[i];
        m_pOneDBLineOnePackageStatue[nMCB].DB_crc_code[i] = pDBDataPackage->DB_crc_code[i];
    }
    //
    m_pOneDBLineOnePackageStatue[nMCB].Max_DB_board = pDBDataPackage->Max_DB_board;
    //
    m_pOneDBLineOnePackageStatue[nMCB].CB_pkg_type = pDBDataPackage->CB_pkg_type;
    m_pOneDBLineOnePackageStatue[nMCB].CB_pkg_index = pDBDataPackage->CB_pkg_index;
    m_pOneDBLineOnePackageStatue[nMCB].CB_board_index = pDBDataPackage->CB_board_index;
    m_pOneDBLineOnePackageStatue[nMCB].CB_logic_err1 = pDBDataPackage->CB_logic_err1;
    m_pOneDBLineOnePackageStatue[nMCB].CB_logic_err2 = pDBDataPackage->CB_logic_err2;
    m_pOneDBLineOnePackageStatue[nMCB].CB_data_len = pDBDataPackage->CB_data_len;
    for(i = 0; i < 16; i++)
    {
        m_pOneDBLineOnePackageStatue[nMCB].CB_data[i] = pDBDataPackage->CB_data[i];
    }
    m_pOneDBLineOnePackageStatue[nMCB].CB_crc_code = pDBDataPackage->CB_crc_code;
    ///////////////////////////////////////////////
	//增加debug节点
  //  if(num_call % 10 == 0)
  //  {
  //      WriteLogFile(" Debug : \t" + std::to_string(m_lDebugCnt));
		//m_lDebugCnt++;
  //  }
    ///////////////////////////////////////////////
    //time(&m_timehot[nMCB]); //update hot connect time
    m_timehot[nMCB] = NucTool::GetTickCount();
    m_timenodata[nMCB] = NucTool::GetTickCount();
    /////////////////////////////
    //无触发计时器清零
    //m_nWaitDataTrigger[nMCB] = 0;
    ///////////////////////////////////////////////
    unsigned short nMaxDBBoardi = pDBDataPackage->Max_DB_board;//DB数量(1个起)
    nMaxDBBoardi = (nMaxDBBoardi <= MAX_MCBADNUM) ? nMaxDBBoardi : MAX_MCBADNUM;
    ///////////////////////////////////////////////
    ///////////////////////////////////////////////
    //数据调整
    UINT nDMNum = m_DMInfo[nMCB].nDMNum;     //Detector Module number of the MCB
    UINT nRowNum = m_DMInfo[nMCB].nRowNum;    //Row of each Detector Module
    UINT nColNum = m_DMInfo[nMCB].nColNum;    //Column of each Detector Module
    //
    nDMNum = (nDMNum < MAX_MCBADNUM) ? nDMNum : MAX_MCBADNUM;
    nRowNum = (nRowNum < MAX_DMROWNUM) ? nRowNum : MAX_DMROWNUM;
    nColNum = (nColNum < MAX_DMCOLNUM) ? nColNum : MAX_DMCOLNUM;
    //
    UINT nPerColPens = nRowNum*nDMNum;//一个MCB(DBLine)带的一列总探测器通道数
    UINT nPerDMPens = nRowNum*nColNum;//一个模块的总探测器通道数
    UINT nPerMCBPens = nDMNum*nRowNum*nColNum;//一个MCB(DBLine)带的总探测器通道数
    ///////////////////////////////////////
    //是否需要
    //memset(pnDBlineDetectorData, 0, sizeof(pnDBlineDetectorData));
    ///////////////////////////////////////
    unsigned long nTriggerCnt = 0;
    unsigned long nADMChData1 = 0;
    unsigned long nADMChData2 = 0;
    unsigned char nTempCh[6] = { 0 };
    INT16U nADMChnumCnt = 0;//1次触发所得数据的总通道索引:从0开始
    //上传的数据低字节在前
    INT8U ADFormat_Bit = pDBDataPackage->ADFormat_Bit;
    if(16 == ADFormat_Bit)
    {
        for(j = 0; j < nMaxDBBoardi; j++)
        {
            for(i = 0; i < pDBDataPackage->DB_data_len[j] / 2; i++)
            {
                //2个字节数据表示一个通道数据，且低字节在前，高字节在后
                nADMChData1 = pDBDataPackage->DB_data[j][i*2] + (pDBDataPackage->DB_data[j][i*2+1]<<8);
                pnDBlineDetectorData[nMCB][j][i/nRowNum][i%nRowNum] = nADMChData1;//一次处理一个DBline带的一个模块的数据
                nADMChnumCnt += 1;//1次触发所得数据的总通道索引，从0开始
                ////////////////////////////////////////////////////////////////
                if((0 == i) && (j < DDataNUM ))
                {
                    m_uDBParameter[nMCB][j] = nADMChData1;//每个模块的第一个通道数据
                }
            }
        }
    }
    if(20 == ADFormat_Bit)
    {
        for(j = 0; j < nMaxDBBoardi; j++)
        {
            for(i = 0; i < pDBDataPackage->DB_data_len[j] / 5; i++)
            {
                //5个字节数据表示2个通道数据，且低字节在前，高字节在后
                //20bit  0x54321 + 0x98765 ==> 54 32 19 87 65//5个字节表示2个通道的数据
                nTempCh[0] =   pDBDataPackage->DB_data[j][i*5+0];
                nTempCh[1] =   pDBDataPackage->DB_data[j][i*5+1];
                nTempCh[2] =   pDBDataPackage->DB_data[j][i*5+2]&0x0F;
                nTempCh[3] = ((pDBDataPackage->DB_data[j][i*5+2]&0xF0)>>4) + ((pDBDataPackage->DB_data[j][i*5+3]&0x0F)<<4);
                nTempCh[4] = ((pDBDataPackage->DB_data[j][i*5+3]&0xF0)>>4) + ((pDBDataPackage->DB_data[j][i*5+4]&0x0F)<<4);
                nTempCh[5] = ((pDBDataPackage->DB_data[j][i*5+4]&0xF0)>>4);
                //
                nADMChData1 = nTempCh[0] + (nTempCh[1]<<8) + (nTempCh[2]<<16);
                nADMChData2 = nTempCh[3] + (nTempCh[4]<<8) + (nTempCh[5]<<16);
                //
                pnDBlineDetectorData[nMCB][j][i*2/nRowNum][i*2%nRowNum] = nADMChData1;//一次处理一个DBline带的一个模块的数据
                pnDBlineDetectorData[nMCB][j][(i*2+1)/nRowNum][(i*2+1)%nRowNum] = nADMChData2;//一次处理一个DBline带的一个模块的数据
                nADMChnumCnt += 2;//1次触发所得数据的总通道索引，从0开始
                ////////////////////////////////////////////////////////////////
                if((0 == i) && (j < DDataNUM) )
                {
                    m_uDBParameter[nMCB][j] = nADMChData1;
                }
                ////////////////////////////////////////////////////////////////
            }
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////
 	////增加debug节点
  //  if(num_call % 10 == 0)
  //  {
  //      WriteLogFile(" Debug : \t" + std::to_string(m_lDebugCnt));
		//m_lDebugCnt++;
  //  }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //某一个MCB(DBLine)上所有模块的数据处理完成，则往软件发一次MCB_MSG_DATA消息
    //探测器数据，每个通道占4 个字节。探测器数据排列顺序为：
    //所有探测器模块（从1 到DMINFO.nDMNum）第一列探测器（共DMINFO.nAllChnNum/DMINFO.nColNum）的数据
    //所有探测器模块第二列探测器的数据…
    //所有探测器模块第DMINFO.nColNum 列探测器的数据
    UINT nCh = 0;
    for(i = 0 ; i < nColNum; i++)
    {
        for(j = 0; j < nDMNum; j++)
        {
            for(k = 0; k < nRowNum; k++)
            {
                nCh = i*nDMNum*nRowNum + j*nRowNum + k;
                m_MCBData[nMCB].addata[nCh].ul = pnDBlineDetectorData[nMCB][j][i][k];
                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                if(m_nADBit > m_nAppADBit)
                {
                    m_MCBData[nMCB].addata[nCh].ul = (m_MCBData[nMCB].addata[nCh].ul) >>(m_nADBit - m_nAppADBit);
                }
                ////////////////////////////////////////////////////////////////////////////////////////////////////////////
                if( fabs(m_fProgrammeGainAmp - 1.0) > 0.0)
                {
                    m_MCBData[nMCB].addata[nCh].ul = (UINT)((double)(m_MCBData[nMCB].addata[nCh].ul)*m_fProgrammeGainAmp); //add 2000LSB@16BIT
                    if (m_MCBData[nMCB].addata[nCh].ul > UINT((1<<m_nADBit) -1))
                    {
                        m_MCBData[nMCB].addata[nCh].ul = UINT((1<<m_nADBit) -1);
                    }
                }
                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                //m_nALGValue:0--Org data; 1--Add 2000;
                if(m_nALGValue > 0)
                {
                    if (m_nADBit >= 16)
                    {
                        //m_ADOriDataUL[nIndex][i].ul += (2000<<(m_nADBit -16)); //add 2000LSB@16BIT
                        m_MCBData[nMCB].addata[nCh].ul += 2000; //add 2000LSB
                        if (m_MCBData[nMCB].addata[nCh].ul > UINT((1<<m_nADBit) -1))
                        {
                            m_MCBData[nMCB].addata[nCh].ul = UINT((1<<m_nADBit) -1);
                        }
                    }
                }
                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                //For 物品机专用: 如果配置了m_nAppADBit参数，且m_nAppADBit<m_nADBit，则认为是用于物品机，P值最大值为(65535)
                //if(m_nADBit > m_nAppADBit)
                //{
                //	if (m_MCBData[nMCB].addata[nCh].ul > UINT((1<<16) -1))
                //	{
                //		m_MCBData[nMCB].addata[nCh].ul = UINT((1<<16) -1);
                //	}
                //}
            }
        }
    }
    ////////////////////////////////////////////////////////////////////////
    int nIndex = nMCB;
    ////////////////////////////////////////////////////////////////////////
    //HMCB默认离控制盒最远的探测器模块为第一个探测器模块，与MCB-H定义相反，默认对所有HMCB或DB-Line按3-MODULE方案调整通道顺序。
    memcpy(m_MCBDataTmp.addata, m_MCBData[nIndex].addata, m_DMInfo[nIndex].nAllChnNum *sizeof(UINT));
    for (i =0; i<int(m_DMInfo[nIndex].nAllChnNum); i++)
    {
        m_MCBData[nIndex].addata[i].ul = m_MCBDataTmp.addata[i/(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)*m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum
                                                             +((m_DMInfo[nIndex].nDMNum-1)-(i/m_DMInfo[nIndex].nRowNum)%m_DMInfo[nIndex].nDMNum)*m_DMInfo[nIndex].nRowNum +i%m_DMInfo[nIndex].nRowNum].ul;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////
 	////增加debug节点
  //  if(num_call % 10 == 0)
  //  {
  //      WriteLogFile(" Debug : \t" + std::to_string(m_lDebugCnt));
		//m_lDebugCnt++;
  //  }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //channel order adjust for all MCB(DB Line)
    for (j =0; j< MAX_CHNADJNUM; j++)
    {
        if (m_nAdjChn[j])  //channel order adjust
        {
            memcpy(m_MCBDataTmp.addata, m_MCBData[nIndex].addata, m_DMInfo[nIndex].nAllChnNum *sizeof(UINT));
            switch(m_nAdjChn[j])  //adjust mode: 0-NONE, 1-ROW, 2-COLUMN, 3-MODULE, 4-ALL_ROW
            {
            case 1:
                for (i =0; i<int(m_DMInfo[nIndex].nAllChnNum); i++)
                {
                    m_MCBData[nIndex].addata[i].ul = m_MCBDataTmp.addata[i/m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nRowNum+
                                                                         m_DMInfo[nIndex].nRowNum-1-i%m_DMInfo[nIndex].nRowNum].ul;
                }
                break;
            case 2:
                for (i =0; i<int(m_DMInfo[nIndex].nAllChnNum); i++)
                {
                    m_MCBData[nIndex].addata[i].ul = m_MCBDataTmp.addata[m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum*(m_DMInfo[nIndex].nColNum-1-(i/(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)))
                                                                         +i%(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)].ul;
                }
                break;
            case 3:
                for (i =0; i<int(m_DMInfo[nIndex].nAllChnNum); i++)
                {
                    m_MCBData[nIndex].addata[i].ul = m_MCBDataTmp.addata[i/(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)*m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum
                                                                         +((m_DMInfo[nIndex].nDMNum-1)-(i/m_DMInfo[nIndex].nRowNum)%m_DMInfo[nIndex].nDMNum)*m_DMInfo[nIndex].nRowNum +i%m_DMInfo[nIndex].nRowNum].ul;
                }
                break;
            case 4:
                for (i =0; i<int(m_DMInfo[nIndex].nAllChnNum); i++)
                {
                    m_MCBData[nIndex].addata[i].ul = m_MCBDataTmp.addata[i/(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)*(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)+
                                                                         m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum-1-i%(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)].ul;
                }
                break;
            default:
                break;
            }
        }
        else
        {
            break;
        }
    }
    ////////////////////////////////////////////////////////////////////////
    //channel order adjust for one MCB(one DB Line)
    //int nIndex = nMCB;
    for (j =0; j<MAX_CHNADJNUM; j++)
    {
        if (m_nMCBxAdjChn[nIndex][j])  //channel order adjust
        {
            memcpy(m_MCBDataTmp.addata, m_MCBData[nIndex].addata, m_DMInfo[nIndex].nAllChnNum *sizeof(UINT));
            switch(m_nMCBxAdjChn[nIndex][j])  //adjust mode: 0-NONE, 1-ROW, 2-COLUMN, 3-MODULE, 4-ALL_ROW
            {
            case 1:
                for (i =0; i<int(m_DMInfo[nIndex].nAllChnNum); i++)
                {
                    m_MCBData[nIndex].addata[i].ul = m_MCBDataTmp.addata[i/m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nRowNum+
                                                                         m_DMInfo[nIndex].nRowNum-1-i%m_DMInfo[nIndex].nRowNum].ul;
                }
                break;
            case 2:
                for (i =0; i<int(m_DMInfo[nIndex].nAllChnNum); i++)
                {
                    m_MCBData[nIndex].addata[i].ul = m_MCBDataTmp.addata[m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum*(m_DMInfo[nIndex].nColNum-1-(i/(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)))
                                                                         +i%(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)].ul;
                }
                break;
            case 3:
                for (i =0; i<int(m_DMInfo[nIndex].nAllChnNum); i++)
                {
                    m_MCBData[nIndex].addata[i].ul = m_MCBDataTmp.addata[i/(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)*m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum
                                                                         +((m_DMInfo[nIndex].nDMNum-1)-(i/m_DMInfo[nIndex].nRowNum)%m_DMInfo[nIndex].nDMNum)*m_DMInfo[nIndex].nRowNum +i%m_DMInfo[nIndex].nRowNum].ul;
                }
                break;
            case 4:
                for (i =0; i<int(m_DMInfo[nIndex].nAllChnNum); i++)
                {
                    m_MCBData[nIndex].addata[i].ul = m_MCBDataTmp.addata[i/(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)*(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)+
                                                                         m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum-1-i%(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)].ul;
                }
                break;
            default:
                break;
            }
        }
        else
        {
            break;
        }
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    //多列合一，多列合并求算术均值
    if (m_bColCombine) // combine column
    {
        for (i =0; i<int(m_DMInfo[nIndex].nAllChnNum/m_DMInfo[nIndex].nColNum); i++)
        {
            if (m_MCBData[nIndex].addata[i].ul == UINT((1<<m_nADBit) -1))  // data saturation
            {
                continue;
            }
            for (j=1; j<int(m_DMInfo[nIndex].nColNum); j++)
            {
                if (m_MCBData[nIndex].addata[i +j*(m_DMInfo[nIndex].nAllChnNum/m_DMInfo[nIndex].nColNum)].ul ==UINT((1<<m_nADBit) -1)) // data saturation
                {
                    m_MCBData[nIndex].addata[i].ul = ((1<<m_nADBit) -1)*m_DMInfo[nIndex].nColNum;
                    break;
                }
                m_MCBData[nIndex].addata[i].ul += m_MCBData[nIndex].addata[i +j*(m_DMInfo[nIndex].nAllChnNum/m_DMInfo[nIndex].nColNum)].ul;
            }
            m_MCBData[nIndex].addata[i].ul /=m_DMInfo[nIndex].nColNum;
        }
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    int nPreSectionADNum = 0;
    int nPreSectionEndRowNum = 0;
    //
    int nSectionADNum = 0;
    int nSectionRowNum = 0;
    int nSectionFirstRowNo = 0;
    //section order adjust for all MCB(DB Line)
    if((m_nSectionFirstADNo[0] > 0) && (m_nSectionLastADNo[0] > 0) && (m_nSectionLastADNo[0] >= m_nSectionFirstADNo[0]))
    {
        //
        memcpy(m_MCBDataTmp.addata, m_MCBData[nIndex].addata, m_DMInfo[nIndex].nAllChnNum *sizeof(UINT));
        for (j = 0; j < MAX_SECTIONADJNUM; j++)
        {
            if((m_nSectionFirstADNo[j] > 0) && (m_nSectionLastADNo[j] > 0) && (m_nSectionLastADNo[j] >= m_nSectionFirstADNo[j]))
            {
                nSectionADNum = m_nSectionLastADNo[j] - m_nSectionFirstADNo[j] + 1;
                nSectionRowNum = nSectionADNum*m_DMInfo[nIndex].nRowNum;
                //每排都需要调整
                for(k = 0; k < m_DMInfo[nIndex].nColNum; k++)
                {
                    nSectionFirstRowNo = (m_nSectionFirstADNo[j] - 1)*m_DMInfo[nIndex].nRowNum + k*m_DMInfo[nIndex].nDMNum*m_DMInfo[nIndex].nRowNum;
                    if(0 == j)
                    {
                        nPreSectionADNum = 0;
                    }
                    else
                    {
                        nPreSectionADNum = m_nSectionLastADNo[j-1] - m_nSectionFirstADNo[j-1] + 1;
                    }
                    nPreSectionEndRowNum = nPreSectionADNum*m_DMInfo[nIndex].nRowNum + k*m_DMInfo[nIndex].nDMNum*m_DMInfo[nIndex].nRowNum;
                    for(i = 0; i < nSectionRowNum; i++)
                    {
                        m_MCBData[nIndex].addata[nPreSectionEndRowNum + i].ul = m_MCBDataTmp.addata[nSectionFirstRowNo+i].ul;
                    }
                }
            }
        }
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    //2019.09.18 Add by zx
    //int nDMInfo_nHalfChannel = m_DMInfo[nIndex].nRowNum * m_DMInfo[nIndex].nColNum / 2;
    //int nDMy = 0;
    //int nPGz = 0;
    ////先高能 后低能（先第一排 再第二排）//
    //for (i =0; i < int(m_DMInfo[nIndex].nAllChnNum/2); i++)
    //{
    //	nDMy = i / nDMInfo_nHalfChannel;
    //	nPGz = (i % nDMInfo_nHalfChannel) / 16;
    //	m_MCBData[nIndex].addata[i].ul = m_MCBData[nIndex].addata[i].ul*m_fMCBxDMyPGz[nIndex][nDMy][nPGz];
    //	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //	//for 物品机16位专用
    //	if (m_MCBData[nIndex].addata[i].ul > UINT((1<<16) -1))
    //	{
    //		m_MCBData[nIndex].addata[i].ul = UINT((1<<16) -1);
    //	}
    //	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //	//}
    //	////后低能
    //	//for (i =0; i < int(m_DMInfo[nIndex].nAllChnNum/2); i++)
    //	//{
    //	//	nDMy = i / nDMInfo_nHalfChannel;
    //	//	nPGz = nDMInfo_nHalfChannel / 16 + (i % nDMInfo_nHalfChannel) / 16;
    //	nPGz += nDMInfo_nHalfChannel / 16;
    //	m_MCBData[nIndex].addata[m_DMInfo[nIndex].nAllChnNum/2 + i].ul = m_MCBData[nIndex].addata[m_DMInfo[nIndex].nAllChnNum/2 + i].ul*m_fMCBxDMyPGz[nIndex][nDMy][nPGz];
    //	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //	//for 物品机16位专用
    //	if (m_MCBData[nIndex].addata[m_DMInfo[nIndex].nAllChnNum/2 + i].ul > UINT((1<<16) -1))
    //	{
    //		m_MCBData[nIndex].addata[m_DMInfo[nIndex].nAllChnNum/2 + i].ul = UINT((1<<16) -1);
    //	}
    //	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //}
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    //UINT k = 0;
    ////
    //int nPreSectionADNum = 0;
    //int nPreSectionEndRowNum = 0;
    ////
    //int nSectionADNum = 0;
    //int nSectionRowNum = 0;
    //int nSectionFirstRowNo = 0;
    //section order adjust for every one MCB(DB Line)
    if((m_nMCBxSectionFirstADNo[nIndex][0] > 0) && (m_nMCBxSectionLastADNo[nIndex][0] > 0) && (m_nMCBxSectionLastADNo[nIndex][0] >= m_nMCBxSectionFirstADNo[nIndex][0]))
    {
        //
        memcpy(m_MCBDataTmp.addata, m_MCBData[nIndex].addata, m_DMInfo[nIndex].nAllChnNum *sizeof(UINT));
        for (j = 0; j < MAX_SECTIONADJNUM; j++)
        {
            if((m_nMCBxSectionFirstADNo[nIndex][j] > 0) && (m_nMCBxSectionLastADNo[nIndex][j] > 0) && (m_nMCBxSectionLastADNo[nIndex][j] >= m_nMCBxSectionFirstADNo[nIndex][j]))
            {
                nSectionADNum = m_nMCBxSectionLastADNo[nIndex][j] - m_nMCBxSectionFirstADNo[nIndex][j] + 1;
                nSectionRowNum = nSectionADNum*m_DMInfo[nIndex].nRowNum;
                //每排都需要调整
                for(k = 0; k < m_DMInfo[nIndex].nColNum; k++)
                {
                    nSectionFirstRowNo = (m_nMCBxSectionFirstADNo[nIndex][j] - 1)*m_DMInfo[nIndex].nRowNum + k*m_DMInfo[nIndex].nDMNum*m_DMInfo[nIndex].nRowNum;
                    if(0 == j)
                    {
                        nPreSectionADNum = 0;
                    }
                    else
                    {
                        nPreSectionADNum = m_nMCBxSectionLastADNo[nIndex][j-1] - m_nMCBxSectionFirstADNo[nIndex][j-1] + 1;
                    }
                    nPreSectionEndRowNum = nPreSectionADNum*m_DMInfo[nIndex].nRowNum + k*m_DMInfo[nIndex].nDMNum*m_DMInfo[nIndex].nRowNum;
                    for(i = 0; i < nSectionRowNum; i++)
                    {
                        m_MCBData[nIndex].addata[nPreSectionEndRowNum + i].ul = m_MCBDataTmp.addata[nSectionFirstRowNo+i].ul;
                    }
                }
            }
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT nError_DI = ((~pDBDataPackage->CB_data[2])& 0xF0)<<4;//按位取反;//Data 2：高4位用于传输DDI
    if(m_bGoudang_S4)
    {
        nError_DI = (nError_DI & 0x0B00) + ((nError_DI&0x0200)<< 1);//������������������������
    }
    //控制板返回的硬件触发数
    //long nPulseCounter = pDBDataPackage->SC_pkg_index;
    //nPulseCounter = nPulseCounter - INVALID_DATA_PACKET_NUM;

    long nPulseCounter = (pDBDataPackage->CB_data[8]<<24)+(pDBDataPackage->CB_data[9]<<16)+(pDBDataPackage->CB_data[10]<<8)+(pDBDataPackage->CB_data[11]);
    nPulseCounter = nPulseCounter - SC_1st_pkg_TriggCnt[nMCB] - INVALID_DATA_PACKET_NUM + m_nDiscardNum[nMCB] + 1;
    //
    m_MCBData[nMCB].d_head.head_use.scounter = pDBDataPackage->nSocketCounter;//nSocketCounter;	// socket packet serial number, self-add//软件包裹计数,1开始
 	//
	//硬件序列号取值模式
   //m_MCBData[nMCB].d_head.head_use.pcounter.l = nPulseCounter;//nPulseCounter;	// pulse trigger number//控制板返回的硬件触发数
	//默认为1，取实际返回包裹数序列号做硬件序列号
	//改为0，取软件包序列号
	//（临时解决存在硬件序列号有重复的现象（换控制盒仍旧存在），导致多个盒级联时存在数据对不齐的问题）（软件端按回调数据里面的硬件触发数存图）
	if(1 == m_bHWSerialNumberMode)
	{
		m_MCBData[nMCB].d_head.head_use.pcounter.l = nPulseCounter;//nPulseCounter;	// pulse trigger number//控制板返回的硬件触发数
	}
	else
	{
		m_MCBData[nMCB].d_head.head_use.pcounter.l = pDBDataPackage->nSocketCounter;//取软件包序列号
	}

    m_MCBData[nMCB].d_head.head_use.accEnergy = ((nError_DI&0x0200) ==0) ? 1 : 0;// acc energy sign
    //m_MCBData[nMCB].d_head.head_use.ain0.ul = 0x12345678;							// ain0 data(UINT)
    //m_MCBData[nMCB].d_head.head_use.ptsmdata_h = 0x87654321;							// exdata lower(ptsm)
    //m_MCBData[nMCB].d_head.head_use.ptsmdata_l = 0x9ABCDEF0;							// exdata upper(ptsm)
    //m_MCBData[nMCB].d_head.head_use.ain0.ul =		(pDBDataPackage->CB_data[0]<<24)+(pDBDataPackage->CB_data[1]<<16)+(pDBDataPackage->CB_data[2]<<8)+(pDBDataPackage->CB_data[3]);// ain0 data(UINT)
    //m_MCBData[nMCB].d_head.head_use.ptsmdata_h =	(pDBDataPackage->CB_data[4]<<24)+(pDBDataPackage->CB_data[5]<<16)+(pDBDataPackage->CB_data[6]<<8)+(pDBDataPackage->CB_data[7]);// exdata lower(ptsm)
    //m_MCBData[nMCB].d_head.head_use.ptsmdata_l =	(pDBDataPackage->CB_data[8]<<24)+(pDBDataPackage->CB_data[9]<<16)+(pDBDataPackage->CB_data[10]<<8)+(pDBDataPackage->CB_data[11]);// exdata upper(ptsm)
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //CB数据包固定包含12个字节的数据,12个字节。
    //Data 0（DB line独立）：高4位传输SDCLK采集错误类型，低4位传输SDCLK采集宽度。
    //Data 1（DB line独立）：系统设计保留。
    //Data 2：高4位已用于传输DDI；低4位保留，全为0。系统保留。
    //Data 3：系统设计保留。
    //Data 4～11: 留给用户使用。
    //m_MCBData[nMCB].d_head.head_use.ain0.ul =		(pDBDataPackage->CB_data[4]<<24)+(pDBDataPackage->CB_data[5]<<16)+(pDBDataPackage->CB_data[6]<<8)+(pDBDataPackage->CB_data[7]);// 24bit ADC值(取高24bit + 8Bit (0x07))
    m_MCBData[nMCB].d_head.head_use.ain0.ul =		(pDBDataPackage->CB_data[4]<<16)+(pDBDataPackage->CB_data[5]<<8)+(pDBDataPackage->CB_data[6]);//24bit ADC值(取高24bit + 8Bit (0x07))
	//
	if(pDBDataPackage->CB_data[4] & 0x80)
	{
		//最高位符号位为1，则直接取除符号位外的数据
		m_MCBData[nMCB].d_head.head_use.ain0.ul &= 0x7FFFFF;
		//
	}
	else
	{
		//最高位符号位为0，则增加0x800000
		m_MCBData[nMCB].d_head.head_use.ain0.ul += 0x800000;
	}
	//适配m_nADBit
	if(m_nADBit < 24)
	{
		m_MCBData[nMCB].d_head.head_use.ain0.ul = (m_MCBData[nMCB].d_head.head_use.ain0.ul)>>(24 - m_nADBit);
	}
	else
	{
		m_MCBData[nMCB].d_head.head_use.ain0.ul = (m_MCBData[nMCB].d_head.head_use.ain0.ul)<<(m_nADBit - 24);
	}
	//
    m_MCBData[nMCB].d_head.head_use.ptsmdata_h =	(pDBDataPackage->CB_data[0]<<24)+(pDBDataPackage->CB_data[1]<<16)+(pDBDataPackage->CB_data[2]<<8)+(pDBDataPackage->CB_data[3]);// 字节0～3：系统保留
    //m_MCBData[nMCB].d_head.head_use.ptsmdata_l =	(pDBDataPackage->CB_data[8]<<24)+(pDBDataPackage->CB_data[9]<<16)+(pDBDataPackage->CB_data[10]<<8)+(pDBDataPackage->CB_data[11]);// CountPos(32bit)
    m_MCBData[nMCB].d_head.head_use.ptsmdata_l = nPulseCounter + INVALID_DATA_PACKET_NUM;
    //
    m_MCBData[nMCB].d_head.head_use.ain1.ul = 0;				// ain1 data(UINT)
    m_MCBData[nMCB].d_head.head_use.sscnt = Update_pkg_n[nMCB];	//nPulseCounter + INVALID_DATA_PACKET_NUM;		// socket packet serial number, from packet
    m_MCBData[nMCB].d_head.head_use.regError_DI = nError_DI;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //调整探测器模块数据与控制盒Ain、Di 数据错位1 个触发的问题
    if (m_bAdjAinDi)  //adjust AIN DI...
    {
        MCBData_H MCBDataHTmp;
        memcpy(&MCBDataHTmp, &m_MCBData[nIndex].d_head, sizeof(MCBData_H));   //temporarily save current MCBData_H
        {  // copy last accEnergy, AIN, DI...
            m_MCBData[nIndex].d_head.head_use.accEnergy = m_MCBDataHeadTmp[nIndex].head_use.accEnergy;
            m_MCBData[nIndex].d_head.head_use.ain0 = m_MCBDataHeadTmp[nIndex].head_use.ain0;
            m_MCBData[nIndex].d_head.head_use.ain1 = m_MCBDataHeadTmp[nIndex].head_use.ain1;
            m_MCBData[nIndex].d_head.head_use.ptsmdata_h = m_MCBDataHeadTmp[nIndex].head_use.ptsmdata_h;
            m_MCBData[nIndex].d_head.head_use.ptsmdata_l = m_MCBDataHeadTmp[nIndex].head_use.ptsmdata_l;
            m_MCBData[nIndex].d_head.head_use.regError_DI = m_MCBDataHeadTmp[nIndex].head_use.regError_DI;
        }
        memcpy(&m_MCBDataHeadTmp[nIndex], &MCBDataHTmp, sizeof(MCBData_H));   //update current MCBData_H
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    m_scounter[nIndex]++;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (0 == nIndex && m_MCBData[nIndex].d_head.head_use.scounter <= GODOWNNUM)
    {
        m_godown[m_MCBData[nIndex].d_head.head_use.scounter-1] =(((m_MCBData[nIndex].d_head.head_use.regError_DI>>8)&0x04)>>2);//DI2
    }
    if (m_MCBData[nIndex].d_head.head_use.scounter <= DDataNUM)
    {
        m_uDetData[nIndex][m_MCBData[nIndex].d_head.head_use.scounter-1] =m_MCBData[nIndex].addata[0].ul;
    }
    if (nIndex ==0 && m_MCBData[nIndex].d_head.head_use.scounter <= MAXEXDATANUM)// && m_bSaveExdata)
    {
        m_InfoExdata[0][m_MCBData[nIndex].d_head.head_use.scounter-1] = m_MCBData[nIndex].d_head.head_use.scounter;
        m_InfoExdata[1][m_MCBData[nIndex].d_head.head_use.scounter-1] = m_MCBData[nIndex].d_head.head_use.pcounter.l;
        m_InfoExdata[2][m_MCBData[nIndex].d_head.head_use.scounter-1] = (m_MCBData[nIndex].d_head.head_use.regError_DI>>8)&0xff;
        m_InfoExdata[3][m_MCBData[nIndex].d_head.head_use.scounter-1] = m_MCBData[nIndex].d_head.head_use.ptsmdata_h;
        m_InfoExdata[4][m_MCBData[nIndex].d_head.head_use.scounter-1] = m_MCBData[nIndex].d_head.head_use.ptsmdata_l;
    }
    //2023.06.16 更新最新收到数据的包裹序号
    m_nNeedSendData[nIndex] = m_MCBData[nIndex].d_head.head_use.scounter;
    ////////////////////////////////////////////////////////////////////////////////////////////////
 	////增加debug节点
  //  if(num_call % 10 == 0)
  //  {
  //      WriteLogFile(" Debug : \t" + std::to_string(m_lDebugCnt));
		//m_lDebugCnt++;
  //  }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if(m_bZeroPadding)//������������������������
    {
        long nIncPulseCnt = 0;
        if(0 == m_lPrePulseCnt[nMCB])
        {
            m_lPrePulseCnt[nMCB] = m_MCBData[nMCB].d_head.head_use.pcounter.l;//��������������������������������������
        }
        else
        {
            nIncPulseCnt = m_MCBData[nMCB].d_head.head_use.pcounter.l - m_lPrePulseCnt[nMCB] - 1;
            if(nIncPulseCnt > 0)//��������������
            {
				if(nIncPulseCnt > 100)
				{
                    memset(tmp, 0, 512);
                    sprintf(tmp,"HMCB%d data is abnormal and the difference in package numbers is %d, which is greater than 100. Just zeroPadding once time.\n", nMCB, nIncPulseCnt);
                    sz_szInfo = tmp;
                    WriteLogFile(sz_szInfo);
					///////////////////////////////////////////////////////////////////////////////////////////////////
					nIncPulseCnt = 1;
				}
				///////////////////////////////////////////////////////////////////////////////////////////////////
                for(i = 0; i < nIncPulseCnt; i++)
                {
                    //set m_MCBDataTmp as 0数据清零
                    //memset(&m_MCBDataTmp, 0, sizeof(MCBData));
                    // 拷贝前次数据备份数据
                    memcpy(&m_MCBDataTmp, &m_MCBDataBackup[nMCB], sizeof(MCBData));//拷贝前次数据备份
                    //copy current packet head to m_MCBDataTmp����������������
                    memcpy(&m_MCBDataTmp.d_head, &m_MCBData[nMCB].d_head, sizeof(MCBData_H));   //temporarily save current MCBData_H
                    ////////////////////////////////////////
                    //��������������������������
                    m_MCBDataTmp.d_head.head_use.scounter = m_MCBDataTmp.d_head.head_use.scounter + i;//nSocketCounter;	// socket packet serial number, self-add//������������,1����
                    m_MCBDataTmp.d_head.head_use.pcounter.l = m_lPrePulseCnt[nMCB] + 1 + i;//nPulseCounter;	// pulse trigger number//����������������������
                    m_MCBDataTmp.d_head.head_use.accEnergy = -1;//������������������
                    ////////////////////////////////////////
                    parsed_pkg_n[nMCB]++;//����������������������������1
                    //����������������
					gCallBack(MCB_MSG_DATA, WPARAM(&m_MCBDataTmp), nMCB);
                    //
                    memset(tmp,0,512);
                    sprintf(tmp,"HMCB%d ZeroPadding one frame data at %d(socket)/%d(pluse).\n", nMCB, m_MCBDataTmp.d_head.head_use.scounter, m_MCBDataTmp.d_head.head_use.pcounter.l);
                    sz_szInfo = tmp;
                    WriteLogFile(sz_szInfo);
                    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                    if(1 == ((m_MCBDataTmp.d_head.head_use.scounter) % 100))
                    {
                        memset(tmp,0,512);
                        sprintf(tmp,"HMCB%d software socket data counts: %d, hardware pluse counts: %d, differ:%d\n",
                                         nMCB, m_MCBDataTmp.d_head.head_use.scounter, m_MCBDataTmp.d_head.head_use.pcounter.l,
                                         m_MCBDataTmp.d_head.head_use.pcounter.l - m_MCBDataTmp.d_head.head_use.scounter);
                        WriteLogFile(sz_szInfo);
                        //����X����������������
                    }
                }
                //������������������������������
                m_MCBData[nMCB].d_head.head_use.scounter = pDBDataPackage->nSocketCounter + nIncPulseCnt;//nSocketCounter;	// socket packet serial number, self-add//������������,1����
                //m_MCBData[nMCB].d_head.head_use.pcounter.l = m_lPrePulseCnt + nIncPulseCnt + 1;//nPulseCounter;	// pulse trigger number//����������������������
            }
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    gCallBack(MCB_MSG_DATA, WPARAM(&m_MCBData[nMCB]), nMCB);
    //
    memcpy(&m_MCBDataBackup[nMCB], &m_MCBData[nMCB], sizeof(MCBData));//备份数据
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    m_lPrePulseCnt[nMCB] = m_MCBData[nMCB].d_head.head_use.pcounter.l;//��������������������
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
 	////增加debug节点
  //  if(num_call % 10 == 0)
  //  {
  //      WriteLogFile(" Debug : \t" + std::to_string(m_lDebugCnt));
		//m_lDebugCnt++;
  //  }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    if(1 == ((m_MCBData[nMCB].d_head.head_use.scounter) % 100))
    {
        char tmp[512] = {0};
        sprintf(tmp,"HMCB%d software socket data counts: %d, hardware pluse counts: %d, differ:%d\n",
                nMCB, m_MCBData[nMCB].d_head.head_use.scounter, m_MCBData[nMCB].d_head.head_use.pcounter.l,
                m_MCBData[nMCB].d_head.head_use.pcounter.l - m_MCBData[nMCB].d_head.head_use.scounter);
        sz_szInfo = tmp;
        //std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
        //执行X命令读取寄存器值
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //gCallBack(MCB_MSG_DATA, WPARAM(&m_MCBData[nMCB]), nMCB);
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////




