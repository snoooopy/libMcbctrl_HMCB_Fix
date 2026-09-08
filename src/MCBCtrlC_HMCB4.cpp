// MCBCtrlC_HMCB4.cpp: implementation of the CMCBCtrlC class.
//////////////////////////////////////////////////////////////////////
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include "stdafx.h"
#include "Shlwapi.h"
#include<windows.h>
#else
#include <unistd.h>
#include <ctime>
#include <thread>
#include <sys/prctl.h>
#include <unistd.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "nuctool.h"
#endif
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

#include "MCBThread.h"

#define gCmdReptN	(1)
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
//////////////////////////////////////////////////////////////////////
//#define GLB_DATA_BUF_SIZE	(100000)	//不能超过32位数的表示范围，必须大于1个SC数据包长度
#define GLB_DATA_BUF_SIZE	(64*1024*1024)	//不能超过32位数的表示范围，必须大于1个SC数据包长度
//
#define SOCKETBUFSIZE (1440*4)
//extern INT8U glb_data_buf[MAX_MCBNUM][GLB_DATA_BUF_SIZE];
extern std::vector<unsigned char> glb_data_buf_vetor[MAX_MCBNUM];
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
extern HANDLE hMutex;
#else
/*extern*/ //boost::shared_mutex mutex1;
mutex _mutex;
#endif

extern ULONGLONG glb_data_AccWrBytes[MAX_MCBNUM];//记录总接收数据字节数
extern ULONGLONG glb_data_AccRdBytes[MAX_MCBNUM];//记录总读出处理字节数

extern INT32U gbl_data_buf_wraddr[MAX_MCBNUM];			//记录数据缓存区下一个数据的写入位置
extern INT32U gbl_data_buf_rdaddr[MAX_MCBNUM];			//记录下一个数据的读取地址
extern INT32S gbl_sc_pkg_index[MAX_MCBNUM];			//记录各个DB Line上一个sc pkg的包编号

//一个DBLine的size= 10字节逻辑 + DB数*一个DB包size + 1个CB包字节
//MAX_MCBNUM个MCB，MAX_DBLINENUM个DBline每个DBline需要接收的字节个数
extern INT32U gbl_DBLine_Size[MAX_MCBNUM][MAX_DBLINENUM];
//一个MCB的size= 该MCB的所有DBLine的size之和
extern INT32U gbl_MCBTotal_Size[MAX_MCBNUM];
//
//INT32U xB_pkg_index_Temp1[MAX_MCBNUM] = { 0 };
//INT32U xB_pkg_index_Temp2[MAX_MCBNUM] = { 0 };
//INT32U xB_pkg_index_Temp3[MAX_MCBNUM] = { 0 };
//--------------------------------------------------------------
//      全局数据读取缓存区
//--------------------------------------------------------------
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
//////////////////////////////////////////////////////////////////////
extern CRITICAL_SECTION ics;
#endif
//////////////////////////////////////////////////////////////////////
void CMCBCtrlC::InitialMCB_HMCB4(unsigned short nMCB)
{
    m_bMCBxInInitialing = true;
    //
    int i = 0;
    std::string sz_szInfo = "";
    // 把这一部分放在单个HMCB初始化里面
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
    char tmp[512] = {0};
    sprintf(tmp,"Initiall HMCB4 No.%d ...\n", nMCB);
    sz_szInfo = tmp;
    std::cout <<  (sz_szInfo);
    WriteLogFile(sz_szInfo);
    
    m_timeInitSpan = NucTool::GetTickCount();
    /////////////////////////////////////////////////////////////
    INT8U ret = 0;
    INT8U chRetFlag = 0;
    /////////////////////////////////////////////////////////////
    m_MCBReadySts[nMCB] = 0;
    /////////////////////////////////////////////////////////////
    ConnectClose(nMCB, MCB_CMD_INITIAL);
    /////////////////////////////////////////////////////////////
    // 建立控制和数据链接socket ，成功返回true
    bool bFuncRet = ConnectOpen (nMCB);//Initial MCBx
    if(!bFuncRet)
    {
	m_MCBReadySts[nMCB] |=0x10;//CTRL socket failed
	//
    m_bMCBxInInitialing = false;
    m_timehot[i] = NucTool::GetTickCount();
	return;
    }
    /////////////////////////////////////////////////////////////
    
    if (bFuncRet) //open connection, configure fpga
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
            sz_szInfo= tmp;
            std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"No error when calling EDAQ_daq_mode_enable(0) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo= tmp;
            std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo,gLogLevel1);
        }
        ///////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
        ETH_CTRL_read_buffer_residual_data(nMCB);
        ret = EDAQ_TEST_DATA_enable(0, nMCB);	//使能(置一)或禁用（清零）FPGA内部计数型测试数据产生模块
        if(ret > 0)
        {
            chRetFlag += (0x01<<1);
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_TEST_DATA_enable(0) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            std::cout <<  sz_szInfo;
            WriteLogFile(sz_szInfo);
        }
	else
	{
		memset(tmp,0,512);
		sprintf(tmp,"No error when calling EDAQ_TEST_DATA_enable(0) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
		sz_szInfo = tmp;
            	std::cout <<  sz_szInfo;
		WriteLogFile(sz_szInfo, gLogLevel1);
	}
        ///////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
        ETH_CTRL_read_buffer_residual_data(nMCB);
        ret = EDAQ_SC_data_pkg_monitor_enable(0, nMCB);	//使能(置一)或禁用（清零）FPGA内嵌的SC数据包检查模块
        if(ret > 0)
        {
	    chRetFlag += (0x01<<2);
	    //
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_SC_data_pkg_monitor_enable(0) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"No Error when calling EDAQ_SC_data_pkg_monitor_enable(0) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo,gLogLevel1);
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
        usleep(5*1000);
#endif
        if(ret > 0)
        {
            chRetFlag += (0x01<<3);
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_CB_logic_reset(7) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"No error when calling EDAQ_CB_logic_reset(7) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo,gLogLevel1);
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
        std::cout << sz_szInfo;
		WriteLogFile(sz_szInfo);
		//
        m_bMCBxInInitialing = false;
        m_timehot[i] = NucTool::GetTickCount();
		return;
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
            	chRetFlag += (0x01<<4);
            	//
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling EDAQ_HMCB_work_mode_set(1) by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                std::cout << sz_szInfo;
                WriteLogFile(sz_szInfo);
            }
            else
            {
            	 memset(tmp,0,512);
                sprintf(tmp,"No error when calling EDAQ_HMCB_work_mode_set(1) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                std::cout << sz_szInfo;
                WriteLogFile(sz_szInfo,gLogLevel1);
            }
        }
        else
        {
            //设置HMCB(4)级联工作模块
            ret = EDAQ_HMCB_work_mode_set(0, nMCB);	//1：主模式；0：从模式
            if(ret > 0)
            {
            	chRetFlag += (0x01<<4);
            	//
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling EDAQ_HMCB_work_mode_set(0) by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                std::cout <<  sz_szInfo;
                WriteLogFile(sz_szInfo);
            }
            else
            {
                memset(tmp,0,512);
                sprintf(tmp,"No error when calling EDAQ_HMCB_work_mode_set(0) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                std::cout << sz_szInfo;
                WriteLogFile(sz_szInfo,gLogLevel1);
            }
        }
        ///////////////////////////////////////////////////////////////
        // 持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
	ETH_CTRL_read_buffer_residual_data(nMCB);
	INT8U DB_line_num = m_DMInfo[nMCB].nDBLineEnNum;
        //功能：设置使能的DB Line数目
        ret = EDAQ_DB_line_num_set(DB_line_num, nMCB);	//设置DB Line数目。
        if(ret > 0)
        {
            chRetFlag += (0x01<<5);
            //
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_DB_line_num_set(1) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            std::cout << (sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"No error when calling EDAQ_DB_line_num_set(1) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            std::cout << (sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据
        ETH_CTRL_read_buffer_residual_data(nMCB);
        //功能：设置DB板子数目
        ret =	EDAQ_DB_num_set(m_DMInfo[nMCB].nDBDMNUM[3], m_DMInfo[nMCB].nDBDMNUM[2], m_DMInfo[nMCB].nDBDMNUM[1], m_DMInfo[nMCB].nDBDMNUM[0], nMCB);//设置4个DB Line各自连接DB板数目。第一个参数，对应DB Line 3的DB num。
        if(ret > 0)
        {
            chRetFlag += (0x01<<6);
	    //
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_DB_num_set(%d, %d, %d, %d) by HMCB(4)%d. The return value is %d.\n", m_DMInfo[nMCB].nDBDMNUM[3], m_DMInfo[nMCB].nDBDMNUM[2], m_DMInfo[nMCB].nDBDMNUM[1], m_DMInfo[nMCB].nDBDMNUM[0], nMCB, ret);
            sz_szInfo = tmp;
            std::cout << (sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
	else
	{
	    memset(tmp,0,512);
        sprintf(tmp,"No error when calling EDAQ_DB_num_set(%d, %d, %d, %d) by HMCB(4)%d. The return value is %d.\n", m_DMInfo[nMCB].nDBDMNUM[3], m_DMInfo[nMCB].nDBDMNUM[2], m_DMInfo[nMCB].nDBDMNUM[1], m_DMInfo[nMCB].nDBDMNUM[0], nMCB, ret);
	    sz_szInfo = tmp;
            std::cout << (sz_szInfo); 
	    WriteLogFile(sz_szInfo, gLogLevel1);
	}
        ///////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
         ETH_CTRL_read_buffer_residual_data(nMCB);
        ///////////////////////////////////////////////////////////////
        INT8U nMaxDBLine = EDAQ_read_DB_line_max_num(nMCB);
        memset(tmp,0,512);
        sprintf(tmp,"HMCB(4)%d is ECM1GA-%d and support the maximum value of DB line number is : %d.\n", nMCB, nMaxDBLine, nMaxDBLine);
        sz_szInfo = tmp;
        std::cout << (sz_szInfo);
        WriteLogFile(sz_szInfo);
        ///////////////////////////////////////////////////////////////
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
         ETH_CTRL_read_buffer_residual_data(nMCB);
        //////////////////////////////////////////////////
        //HMCB(4) FPGA固件核心版本号和外围版本号生成时间
        EDAQ_FIRMWARE_VERSION_REG reg1;
        ret = ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR, &reg1.All, nMCB);
        if(ret > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR,...) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            std::cout << (sz_szInfo);
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
            std::cout << (sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////
        //EEPROM ID
        //INT64U rd_ID;
        //rd_ID = EDAQ_EEPROM_ID_read(nMCB);
	INT16U Int16PIDdata[64] = { 0 };
	ret = EDAQ_EEPROM_ID_read(nMCB, Int16PIDdata);
        //EEPROM中存储的板子生产时间
        std::string strProductionTime = ("");
        char time_str[30] = { 0x00 };
        std::string str_time;
        struct tm *info;
        time_t t;
        t = (time_t) EDAQ_production_time_read(nMCB);
        info = localtime(&t);
	if(NULL == info)
	{
         WriteLogFile("Failed to get the board production time.\n");
         str_time="Failed to get";
	}
	else
    {
         memset(tmp,0,512);
         sprintf(tmp,"%04d/%02d/%02d %02d:%02d:%02d", 1900+info->tm_year, info->tm_mon, info->tm_mday, info->tm_hour, info->tm_min, info->tm_sec);
        str_time = tmp;
	}
	strProductionTime += str_time;
        
        strftime(time_str, sizeof(time_str), "%Y/%m/%d %H:%M:%S",localtime(&t));
        for(int i = 0; i < sizeof(time_str); i++)
        {
            strProductionTime += time_str[i];
        }
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
        m_strMCBCoreFirmwareVer[nMCB] = tmp;
        memset(tmp,0,512);
        sprintf(tmp,"HMCB(4)%d core firmware version: %s; HMCB(4) other firmware version: %d.%d.%d.\n",nMCB, m_strMCBCoreFirmwareVer[nMCB].c_str(), reg2.Sepe.major_ver,reg2.Sepe.minor_ver,reg2.Sepe.revision);
        sz_szInfo = tmp;
        std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
        //
        memset(tmp,0,512);
        sprintf(tmp,"HMCB(4)%d Board ID: 0x%04X%04X%04X%04X; Board production time(Y/m/d H:M:S): %s.\n", nMCB, Int16PIDdata[3], Int16PIDdata[2], Int16PIDdata[1], Int16PIDdata[0], strProductionTime.c_str());
        sz_szInfo = tmp;
        std::cout << (sz_szInfo);
        WriteLogFile(sz_szInfo);
        ///////////////////////////////////////////////////////////////
        m_timeInitSpan = NucTool::GetTickCount();
        if (!ConfigFpga_HMCB4(nMCB))
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling ConfigFpga_HMCB4() by HMCB%d. FPAG configuration failed.\n", nMCB);
            sz_szInfo = tmp;
            std::cout << (sz_szInfo);
            WriteLogFile(sz_szInfo);
            SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_FPGAFILE_ERR);
            ConnectClose(nMCB, 0);
	    //
	    m_MCBReadySts[nMCB] |=0x30;//communicate and send command to HMCB failed
            //
        m_bMCBxInInitialing = false;
        m_timehot[i] = NucTool::GetTickCount();
	    return;
        }
        else
        {
            //time(&m_timeuncheck);
            m_timeuncheck = NucTool::GetTickCount();
            //SetReg(nMCB);
        }
        m_timeInitSpan = NucTool::GetTickCount();
        ///////////////////////////////////////////////////////////////
        //下载FPGA后重新读取固件核心版本号等信息
        //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
        ETH_CTRL_read_buffer_residual_data(nMCB);
        ///////////////////////////////////////////////////////////////
        //HMCB FPGA固件核心版本号和外围版本号生成时间
        EDAQ_FIRMWARE_VERSION_REG reg3;
        ret = ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR, &reg3.All, nMCB);
        if(ret > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR,...) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            std::cout << (sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////
        //用户可自定义的状态寄存器(REV_STATUS_REG1,地址为32*4)用于表示HMCB FPGA固件外围版本号
        EDAQ_FIRMWARE_VERSION_REG reg4;
        ret = ECS_A32D32_read(CB_PERI_CIRCUITS_VERSION_REG_ADDR, &reg4.All, nMCB);
        if(ret > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling ECS_A32D32_read(CB_PERI_CIRCUITS_VERSION_REG_ADDR,...) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            std::cout << (sz_szInfo);
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////
        //EEPROM ID
        //INT64U rd_ID;
        //rd_ID = EDAQ_EEPROM_ID_read(nMCB);
        ret = EDAQ_EEPROM_ID_read(nMCB, Int16PIDdata);
        //EEPROM中存储的板子生产时间
        //CString strProductionTime = _T("");
  /*      strProductionTime = ("");
        //char time_str[30] = { 0x00 };
        //time_t t;
        t = (time_t) EDAQ_production_time_read(nMCB);
        strftime(time_str, sizeof(time_str), "%Y/%m/%d %H:%M:%S",localtime(&t));
        for(int i = 0; i < sizeof(time_str); i++)
        {
            strProductionTime += time_str[i];
        }
        */////////////////////////////////////////////////////////////////////
        nMCB_Version[nMCB]  = reg3.Sepe.build_num;
        nMCB_Version[nMCB] += (reg3.Sepe.revision&0x0F)<<4;
        nMCB_Version[nMCB] += (reg3.Sepe.minor_ver&0x0F)<<8;
        nMCB_Version[nMCB] += (reg3.Sepe.major_ver&0x0F)<<12;
        /////////////////////////////////////////////////////////////////////
        memset(tmp,0,512);
        sprintf(tmp,"HMCB(4)%d core firmware version: %d.%d.%d; HMCB(4) other firmware version: %d.%d.%d.\n",nMCB, reg3.Sepe.major_ver,reg3.Sepe.minor_ver,reg3.Sepe.revision, reg4.Sepe.major_ver,reg4.Sepe.minor_ver,reg4.Sepe.revision);
        sz_szInfo = tmp;
        std::cout << (sz_szInfo);
        WriteLogFile(sz_szInfo);
        /////////////////////////////////////////////////////////////////////
        //设置CB工作参数
        SetReg(nMCB);
        /////////////////////////////////////////////////////////////////////
        int nRet = 0;
        //设置各DBline的DM配置参数
        SetDMCmdReg_HMCB4(nMCB);
	///////////////////////////////////////////////////
	//	int nLoop = 0;
	//	do{
	//		//Dump all the ECS registers
	//DumpAllTheECSRegistersRelease(nMCB);
	//		if(nRet != 0)
	//		{
	//			nLoop++;
	//		}
	//	}while((nRet != 0) && (nLoop < gCmdReptN));
	///////////////////////////////////////////////////
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
        std::cout << sz_szInfo;
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
    //time(&m_timeuncheck);
    m_timeInitSpan = NucTool::GetTickCount();
    m_timeuncheck = NucTool::GetTickCount();
    m_bMCBxInInitialing = false;
    m_timehot[i] = NucTool::GetTickCount();
}

//////////////////////////////////////////////////////////////////////
void CMCBCtrlC::Start_HMCB4(long lcounter)
{
	std::string sz_szInfo = "";
	char tmp[512] = {0};
	unsigned short i = 0;
	unsigned short j = 0;
	INT8U ret = 0;
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//收到[Start]命令次数
	m_nRecvStartCmdCnt++;
	//
	sprintf(tmp,"HMCB(4).dll received [Start] CMD(%d), packet-number:%d...\n", m_nRecvStartCmdCnt, lcounter);
	sz_szInfo = tmp;
	std::cout << sz_szInfo;
	WriteLogFile(sz_szInfo);
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if(m_bMCBxInInitialing)
	{
		sz_szInfo= "Warning: HMCB(4) is Initialing, can't start now!\n";
		std::cout << sz_szInfo;
		WriteLogFile(sz_szInfo);
		SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, MCB_STSCD_STARTRUN);
		return;
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if(m_bStart)
	{
		sz_szInfo = "Warning: HMCB(4) is running, can't start again!\n";
		std::cout << sz_szInfo;
		WriteLogFile(sz_szInfo);
		SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, MCB_STSCD_STARTRUN);
		return;
	}
	///////////////////////////////////////////////////////////////
	//主HMCB(4)模块，调用EDAQ_daq_mode_enable(0)，确保不处于数据采集模式。
	for(i = 0; i < m_nMCBNum; i++)
	{
		////////////////////////////////////////////////////////////////
		//持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
		ETH_CTRL_read_buffer_residual_data(i);
		////////////////////////////////////////////////////////////////
		ret = EDAQ_daq_mode_enable(0, i);
		if(ret != 0)
		{
            memset(tmp,0,512);
			sprintf(tmp,"HMCB(4)%d Received [start] CMD, error calling EDAQ_daq_mode_enable(0).The return value is %d.\n", i, ret);
			sz_szInfo = tmp;
			std::cout << sz_szInfo;
			WriteLogFile(sz_szInfo);
			//
			m_bPreDAQSucessFlag[i] = false;
			//
		}
		else
		{
			memset(tmp,0,512);
			sprintf(tmp,"HMCB(4)%d Received [start] CMD, no error calling EDAQ_daq_mode_enable(0).The return value is %d.\n", i, ret);
			sz_szInfo = tmp;
			std::cout << sz_szInfo;
			WriteLogFile(sz_szInfo, gLogLevel1);
		}
        usleep(10 * 1000);
	}
	///////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////
	//复位上一次数据采集是否成功完成标志
	for(i = 0; i < m_nMCBNum; i++)
	{
		m_bPreDAQSucessFlag[i] = false;
	}
	////////////////////////////////////////////////////////////
	//记录data socket原始数据文件
/*	if( m_nLogLevel >= 5)
	{
		//////////////////////////////////////////////////////
		SYSTEMTIME t_st;
		GetLocalTime(&t_st);
		//关闭前面打开的文件
		if(m_bSocketRawDatafile)
		{
			m_SocketRawDatafile.Close();
			m_bSocketRawDatafile = FALSE;
		}
		//
		m_unSocketRawWrCnt = 0;//写入记录的总数据字节数
		//随时间形成新的文件名
		m_SocketRawDatafilename.Format(_T("%s\\SocketRawData_%04d%02d%02d%02d%02d%02d.txt"), m_szPath, t_st.wYear, t_st.wMonth, t_st.wDay, t_st.wHour, t_st.wMinute, t_st.wSecond);
		//创建打开新文件
		locale loc = locale::global(locale("")); //要打开的文件路径含中文，设置全局locale为本地环境  
		//
		if (m_SocketRawDatafile.Open(m_SocketRawDatafilename, CFile::modeWrite |CFile::modeCreate |CFile::modeNoTruncate |CFile::typeText |CFile::shareDenyWrite ) ==0) //
		{
			TRACE(_T("open Raw data file error!\n"));
			m_bSocketRawDatafile = FALSE;
		}
		else
		{
			m_SocketRawDatafile.SeekToEnd();
			m_bSocketRawDatafile = TRUE;
		}
		//打开文件
		locale::global(loc);//恢复全局locale</span>  
	}
    */ //记得放开 ssx 2023年12月08日10:21:14
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	m_bStart = true;
	m_bPreStop = false;
	m_bPreDataEnd = false;
	//已收到停止采集命令
	m_bRecStopCmd = false;
	//
	m_nSendAllRecorded = 0;//进入send all 模式日志是否已记录 0 初始 1需要记录 2已记录
	
    //m_nMissAA55Cnt = 0;
    for(i = 0; i < m_nMCBNum; i++)
    {
        m_nMissAA55Cnt[i]=0;
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
        m_MCBRunSts[i] &= 0xfb;
		m_scounter[i] = 0;  //scounter reset
		m_nNeedSendData[i] =0;
		m_MCBData[i].d_head.head_use.scounter = 0;
		memset(m_MCBData, 0x00, sizeof(m_MCBData));
        //
        memset(m_MCBDataBackup, 0x00, sizeof(m_MCBDataBackup));//前次数据备份
        m_lPrePulseCnt[i] = 0;//前次正常解析数据
        //
		if (!m_pDataSocket[i]) // check connection status
		{
            m_bStart =false;
            memset(tmp,0,512);
            sprintf(tmp,"HMCB(4)%d started data collection, but the data socket was not established properly! FAIL TO START!\n", i);
            sz_szInfo = tmp;
            std::cout << sz_szInfo;
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
	//UINT nDMNum[MAX_MCBNUM] = { 0 };//每个DBLine带的有效模块数
	for(i = 0; i < MAX_MCBNUM; i++)
	{
		nRowNum[i] = m_DMInfo[i].nRowNum;//每行/列笔数
		nColNum[i] = m_DMInfo[i].nColNum;//行列数
		//nDMNum[i] = m_DMInfo[i].nDMNum;//每个DBLine带的有效模块
	}
	///////////////////////////////////////////////////////////
	//
	INT32U OneDBSize = 0;
	//一个CBDataPackage的size = 6字节逻辑+12字节数据+1字节CRC
	INT32 CBDataPackSize = 6 + 12 + 1;
	//
	for(i = 0; i < m_nMCBNum; i++)
	{
		//////////////////////////////////////////////////////////////////////
		//数据解析按智能（=0）还是高能（=1）模式解析处理
		if(m_bHEorCXMode)
		{
			//一个DB包size = 6字节头+（2+通道数）个通道数据+1个CRC
			//2022-08-09改为一个DB包size = 6字节头+（模块通道数）个通道数据+1个CRC
			OneDBSize = 6 + (nRowNum[i]*nColNum[i])*ADFormat_Bit/8 + 1;
		}
		else
		{
			//for 智能：一个DB包size = 6字节头+（2(2个特殊通道)+通道数）个通道数据+1个CRC
			OneDBSize = 6 + (2 + nRowNum[i]*nColNum[i])*ADFormat_Bit/8 + 1;
		}
        //////////////////////////////////////////////////////////////////////
        gbl_MCBTotal_Size[i] = 0;
        for(j = 0; j < m_DMInfo[i].nDBLineEnNum; j++)
		{
			//一个DBline的数据size = 10个包头 + DB数量*一个DB包size + 一个CBDataPackage的size
			gbl_DBLine_Size[i][j] = 10 + m_DMInfo[i].nDBDMNUM[j]*OneDBSize + CBDataPackSize;
            gbl_MCBTotal_Size[i] += gbl_DBLine_Size[i][j];
		}
		//
		glb_data_AccWrBytes[i] = 0;		//记录总接收数据字节数
		glb_data_AccRdBytes[i] = 0;		//记录总读出处理字节数
		gbl_data_buf_wraddr[i] = 0;		//记录数据缓存区下一个数据的写入位置
		gbl_data_buf_rdaddr[i] = 0;		//记录下一个数据的读取地址	
		gbl_sc_pkg_index[i] = -1;		//记录各个DB Line上一个sc pkg的包编号
	}
	////////////////////////////////////////////////////////////
	//hMutex = CreateMutex(NULL, FALSE, NULL);
	////////////////////////////////////////////////////////////
    //启动多线程准备数据解析
	//ReadParseData_HMCB4(m_lcounter);
	//屏蔽多线程，改用单线程解析数据
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
		std::cout << "Master HMCB(4) start OK, begin running...\n";
		WriteLogFile("Master HMCB(4) start OK, begin running...\n");
		SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_STARTOK);
		///////////////////////////////////////////////////////////////
		ProcStartDaqCmd(nMCB);
		///////////////////////////////////////////////////////////////
		memset(tmp,0,512);
		sprintf(tmp,"HMCB(4) No.%d start receive data.\n", nMCB);
		sz_szInfo = tmp;
		std::cout << sz_szInfo;
		WriteLogFile(sz_szInfo);
		m_MCBRunSts[nMCB] |=0x01;
		/////////////////////////////////////////////////////////////////
		////持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
		//ETH_CTRL_read_buffer_residual_data(nMCB);
		///////////////////////////////////////////////////////////////////
		////启动数据采集
		////功能：发出DB DAQ命令传输请求
		//ret = EDAQ_DB_daq_cmd_tx_req(nMCB);
		//if(0 != ret)
		//{
		//	memset(tmp,0,512);
		//	sprintf(tmp,"Error when calling EDAQ_DB_daq_cmd_tx_req(%d) by HMCB(4)%d. The return value is %d.\n", nMCB, nMCB, ret);
		//	sz_szInfo = tmp;
		//	std::cout << sz_szInfo;
		//	WriteLogFile(sz_szInfo);
		//}
		//else
		//{
		//	memset(tmp,0,512);
		//	sprintf(tmp,"No error when calling EDAQ_DB_daq_cmd_tx_req(%d) by HMCB(4)%d. The return value is %d.\n", nMCB, nMCB, ret);
		//	sz_szInfo = tmp;
		//	std::cout << sz_szInfo;
		//	WriteLogFile(sz_szInfo, gLogLevel1);
		//}
		///////////////////////////////////////////////////////////////////
		////持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
		//ETH_CTRL_read_buffer_residual_data(nMCB);
		///////////////////////////////////////////////////////////////////
		////持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
		////选择触发方式
		//ret = SetTrigCtrlReg(nMCB);
		//if(0 != ret)
		//{
		//	memset(tmp,0,512);
		//	sprintf(tmp,"Error when calling SetTrigCtrlReg() by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
		//	sz_szInfo = tmp;
		//	std::cout << sz_szInfo;
		//	WriteLogFile(sz_szInfo);
		//}
		//else
		//{
		//	memset(tmp,0,512);
		//	sprintf(tmp,"No error when calling SetTrigCtrlReg() by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
		//	sz_szInfo = tmp;
		//	std::cout << sz_szInfo;
		//	WriteLogFile(sz_szInfo, gLogLevel1);
		//}
		///////////////////////////////////////////////////////////////////
		////持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
		//ETH_CTRL_read_buffer_residual_data(nMCB);
		///////////////////////////////////////////////////////////////////
		////进入取数模式//CB进入到数据采集模式
		//ret = EDAQ_daq_mode_enable(1, nMCB);//进入取数模式//CB进入到数据采集模式
		//if(0 != ret)
		//{
		//	memset(tmp,0,512);
		//	sprintf(tmp,"Error when calling EDAQ_daq_mode_enable(1) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
		//	sz_szInfo = tmp;
		//	std::cout << sz_szInfo;
		//	WriteLogFile(sz_szInfo);
		//}
		//else
		//{
		//	memset(tmp,0,512);
		//	sprintf(tmp,"No error when calling EDAQ_daq_mode_enable(1) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
		//	sz_szInfo = tmp;
		//	std::cout << sz_szInfo;
		//	WriteLogFile(sz_szInfo, gLogLevel1);
		//}
		/////////////////////////////////////////////////////////////////
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
		for(i = 1; i < m_nMCBNum; i++)
		{
			//nMCB = i - 1;
			nMCB = i;
			//
			m_MCBRunSts[nMCB] &=0xfe;
			///////////////////////////////////////////////////////////////
			memset(tmp,0,512);
			sprintf(tmp,"Slave HMCB(4)%d start OK, begin running...\n", nMCB);
			sz_szInfo = tmp;
			std::cout << sz_szInfo;
			WriteLogFile(sz_szInfo);
			SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_STARTOK);
			///////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////
			ProcStartDaqCmd(nMCB);
			///////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////
			memset(tmp,0,512);
			sprintf(tmp,"Slave HMCB(4)%d start receive data.\n", nMCB);
			sz_szInfo = tmp;
			std::cout << sz_szInfo;
			WriteLogFile(sz_szInfo);
            m_MCBRunSts[nMCB] |=0x01;
			/////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////
			////持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
			//ETH_CTRL_read_buffer_residual_data(nMCB);
			//usleep(10*1000);
			////启动数据采集
			//ret = EDAQ_DB_daq_cmd_tx_req(nMCB);
			//if(0 != ret)
			//{
			//	memset(tmp,0,512);
			//	sprintf(tmp,"Error when calling EDAQ_DB_daq_cmd_tx_req() by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
			//	sz_szInfo = tmp;
			//	std::cout << (sz_szInfo);
			//	WriteLogFile(sz_szInfo);
			//}
			//else
			//{
			//	memset(tmp,0,512);
			//	sprintf(tmp,"No error when calling EDAQ_DB_daq_cmd_tx_req() by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
			//	sz_szInfo = tmp;
			//	std::cout << sz_szInfo;
			//	WriteLogFile(sz_szInfo, gLogLevel1);
			//}
			///////////////////////////////////////////////////////////////////
			////持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
			////选择触发方式
			//ret = SetTrigCtrlReg(nMCB);
			//if(0 != ret)
			//{
			//	memset(tmp,0,512);
			//	sprintf(tmp,"Error when calling SetTrigCtrlReg() by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
			//	sz_szInfo = tmp;
			//	std::cout << sz_szInfo;
			//	WriteLogFile(sz_szInfo);
			//}
			//else
			//{
			//	memset(tmp,0,512);
			//	sprintf(tmp,"No error when calling SetTrigCtrlReg() by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
			//	sz_szInfo = tmp;
			//	std::cout << sz_szInfo;
			//	WriteLogFile(sz_szInfo, gLogLevel1);
			//}
			///////////////////////////////////////////////////////////////////
			////持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
			//ETH_CTRL_read_buffer_residual_data(nMCB);
			////进入取数模式//CB进入到数据采集模式
			//ret = EDAQ_daq_mode_enable(1, nMCB);
			//if(0 != ret)
			//{
			//	memset(tmp,0,512);
			//	sprintf(tmp,"Error when calling EDAQ_daq_mode_enable(1) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
			//	sz_szInfo = tmp;
			//	std::cout << sz_szInfo;
			//	WriteLogFile(sz_szInfo);
			//}
			//else
			//{
			//	memset(tmp,0,512);
			//	sprintf(tmp,"No error when calling EDAQ_daq_mode_enable(1) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
			//	sz_szInfo = tmp;
			//	std::cout << sz_szInfo;
			//	WriteLogFile(sz_szInfo, gLogLevel1);
			//}
			///////////////////////////////////////////////////////////////////
			//usleep(10*1000);
			///////////////////////////////////////////////////////////////////
			////sz_szInfo.Format(_T("After EDAQ_daq_mode_enable(1, %d) and DumpAllTheECSRegistersRelease(%d, 1) .\n"), nMCB, nMCB);
			////WriteLogFile(sz_szInfo, gLogLevel1);
			////DumpAllTheECSRegistersRelease(nMCB, 1);
			///////////////////////////////////////////////////////////////////
   //         m_MCBRunSts[nMCB] |=0x01;
		}
	}
	//
	for(i =0; i< m_nMCBNum; i++)
    {
        m_timenodata[i] = NucTool::GetTickCount();
	}
}
//////////////////////////////////////////////////////////////////////
INT8U CMCBCtrlC::ResetCBLogicBeforeStart_HMCB4(unsigned short nMCB)
{
    int i=0;
    INT8U ret = 0;
    string sz_szInfo = "";
    /////////////////////////////////////////////////////////////
    char tmp[512] = {0};
    sprintf(tmp,"HMCB0-DB%d resets CB logic before starting a new acquisition.\n", nMCB);
    sz_szInfo = tmp;
    std::cout << sz_szInfo;
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
    bool bFuncRet = ConnectOpen (nMCB);
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
            sprintf(tmp,"Error when calling EDAQ_daq_mode_enable(0) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
            return ret;
        }
        ///////////////////////////////////////////////////////////////
        ret = EDAQ_TEST_DATA_enable(0, nMCB);	//使能(置一)或禁用（清零）FPGA内部计数型测试数据产生模块
        if(ret > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_TEST_DATA_enable(0) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////
        ret = EDAQ_SC_data_pkg_monitor_enable(0, nMCB);	//使能(置一)或禁用（清零）FPGA内嵌的SC数据包检查模块
        if(ret > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_SC_data_pkg_monitor_enable(0) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo= tmp;
            std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////
        //逻辑复位FPGA逻辑//The parsed result of DB num of DB-Line[0..3]对应寄存器值清0
        ret = EDAQ_CB_logic_reset(nMCB);

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        Sleep(5);
#else
        usleep(5*1000);
#endif
        if(ret > 0)
        {
            char tmp[512] = {0};
            sprintf(tmp,"Error when calling EDAQ_CB_logic_reset(8) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo= tmp;
            std::cout <<sz_szInfo;
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////
        //持续从ETH(_ETH_Data_socket)读取数据，直到清空缓存中的数据。
        //ETH_read_buffer_residual_data();
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
                char tmp[512] = {0};
                sprintf(tmp,"Error when calling EDAQ_HMCB_work_mode_set(1) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                std::cout << sz_szInfo;
                WriteLogFile(sz_szInfo);
            }
        }
        else
        {
            //设置HMCB(4)级联工作模块
            ret = EDAQ_HMCB_work_mode_set(0, nMCB);	//1：主模式；0：从模式
            if(ret > 0)
            {
                char tmp[512] = {0};
                sprintf(tmp,"Error when calling EDAQ_HMCB_work_mode_set(0) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                std::cout << sz_szInfo;
                WriteLogFile(sz_szInfo);
            }
        }
        ///////////////////////////////////////////////////////////////
        INT8U DB_line_num = m_DMInfo[nMCB].nDBLineEnNum;
        //功能：设置DB Line数目//每个HMCB(4)对应一个DB Line
        ret = EDAQ_DB_line_num_set(DB_line_num, nMCB);	//设置DB Line数目。
        if(ret > 0)
        {
            char tmp[512] = {0};
            sprintf(tmp,"Error when calling EDAQ_DB_line_num_set(1) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////
        //功能：设置DB板子数目
        ret =	EDAQ_DB_num_set(m_DMInfo[nMCB].nDBDMNUM[3], m_DMInfo[nMCB].nDBDMNUM[2], m_DMInfo[nMCB].nDBDMNUM[1], m_DMInfo[nMCB].nDBDMNUM[0], nMCB);//设置4个DB Line各自连接DB板数目。第一个参数，对应DB Line 3的DB num。
        if(ret > 0)
        {
            char tmp[512] = {0};
            sprintf(tmp,"Error when calling EDAQ_DB_num_set(%d, %d, %d, %d) by HMCB(4)%d. The return value is %d.\n", m_DMInfo[3].nDMNum, m_DMInfo[2].nDMNum, m_DMInfo[1].nDMNum, m_DMInfo[0].nDMNum, nMCB, ret);
            sz_szInfo= tmp;
            std::cout <<sz_szInfo;
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////
        if(m_nLogLevel >= 3)
        {
            INT8U nMaxDBLine = EDAQ_read_DB_line_max_num(nMCB);
            memset(tmp,0,512);
            sprintf(tmp,"HMCB(4)%d is ECM1GA-%d and support the maximum value of DB line number is : %d.\n", nMCB, nMaxDBLine, nMaxDBLine);
            sz_szInfo = tmp;
            std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
            ///////////////////////////////////////////////////////////////
            //HMCB FPGA固件核心版本号和外围版本号生成时间
            EDAQ_FIRMWARE_VERSION_REG reg1;
            ret = ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR, &reg1.All, nMCB);
            if(ret > 0)
            {
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR,...) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);

                sz_szInfo = tmp;
                std::cout << sz_szInfo;
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
                std::cout << (sz_szInfo);
                WriteLogFile(sz_szInfo);
            }
            ///////////////////////////////////////////////////////////////
            //EEPROM ID
            //INT64U rd_ID;
            //rd_ID = EDAQ_EEPROM_ID_read(nMCB);
            INT16U Int16PIDdata[64] = { 0 };
	    ret = EDAQ_EEPROM_ID_read(nMCB, Int16PIDdata);
            //EEPROM中存储的板子生产时间
            std::string strProductionTime = "";
            char time_str[30] = { 0x00 };
             std::string str_time;
            time_t t;
            struct tm * info;
            t = (time_t) EDAQ_production_time_read(nMCB);
            info = localtime(&t);
	    if(NULL == info)
	    {
            WriteLogFile(("Failed to get the board production time.\n"));
            str_time="Failed to get";
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
            m_strMCBCoreFirmwareVer[nMCB]=tmp;
            memset(tmp,0,512);
            sprintf(tmp,"HMCB(4)%d core firmware version: %s; HMCB(4) other firmware version: %d.%d.%d.\n",nMCB, m_strMCBCoreFirmwareVer[nMCB].c_str(), reg2.Sepe.major_ver,reg2.Sepe.minor_ver,reg2.Sepe.revision);
            sz_szInfo = tmp;;
            std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
            //
            memset(tmp,0,512);
            sprintf(tmp,"HMCB(4)%d Board ID: 0x%04X%04X%04X%04X; Board production time(Y/m/d H:M:S): %s.\n", nMCB, Int16PIDdata[3], Int16PIDdata[2], Int16PIDdata[1], Int16PIDdata[0], strProductionTime.c_str());
            sz_szInfo = tmp;
            std::cout << (sz_szInfo);
            WriteLogFile(sz_szInfo);
            ///////////////////////////////////////////////////////////////
            memset(tmp,0,512);
            sprintf(tmp,"MCB No.%d Re-Configure FPAG bin file.\n", nMCB);
            sz_szInfo = tmp;
            std::cout << (sz_szInfo);
            WriteLogFile(sz_szInfo);
            if (!ConfigFpga_HMCB4())
            {
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling ConfigFpga_HMCB4() by HMCB(4)%d. FPAG configuration failed.\n", nMCB);
                sz_szInfo = tmp;
                std::cout << (sz_szInfo);
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
            //HMCB(4) FPGA固件核心版本号和外围版本号生成时间
            EDAQ_FIRMWARE_VERSION_REG reg3;
            ret = ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR, &reg3.All, nMCB);
            if(ret > 0)
            {
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR,...) by HMCB%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                std::cout << (sz_szInfo);
                WriteLogFile(sz_szInfo);
            }
            ///////////////////////////////////////////////////////////////
            //用户可自定义的状态寄存器(REV_STATUS_REG1,地址为32*4)用于表示HMCB FPGA固件外围版本号
            EDAQ_FIRMWARE_VERSION_REG reg4;
            ret = ECS_A32D32_read(CB_PERI_CIRCUITS_VERSION_REG_ADDR, &reg4.All, nMCB);
            if(ret > 0)
            {
                memset(tmp,0,512);
                sprintf(tmp,"Error when calling ECS_A32D32_read(CB_PERI_CIRCUITS_VERSION_REG_ADDR,...) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
                sz_szInfo = tmp;
                std::cout << (sz_szInfo);
                WriteLogFile(sz_szInfo);
            }
            ///////////////////////////////////////////////////////////////
            //EEPROM ID
            //INT64U rd_ID;
            //rd_ID = EDAQ_EEPROM_ID_read(nMCB);
            ret = EDAQ_EEPROM_ID_read(nMCB, Int16PIDdata);
            //EEPROM中存储的板子生产时间
            //CString strProductionTime = _T("");
            strProductionTime = ("");
            //char time_str[30] = { 0x00 };
            //time_t t;
            t = (time_t) EDAQ_production_time_read(nMCB);
            strftime(time_str, sizeof(time_str), "%Y/%m/%d %H:%M:%S",localtime(&t));
            for(int i = 0; i < sizeof(time_str); i++)
            {
                strProductionTime += time_str[i];
            }
            /////////////////////////////////////////////////////////////////////
            nMCB_Version[nMCB]  = reg3.Sepe.build_num;
            nMCB_Version[nMCB] += (reg3.Sepe.revision&0x0F)<<4;
            nMCB_Version[nMCB] += (reg3.Sepe.minor_ver&0x0F)<<8;
            nMCB_Version[nMCB] += (reg3.Sepe.major_ver&0x0F)<<12;
            /////////////////////////////////////////////////////////////////////
            char tmp[512] = {0};
            sprintf(tmp,"HMCB(4)%d core firmware version: %d.%d.%d; HMCB(4) other firmware version: %d.%d.%d.\n",nMCB, reg3.Sepe.major_ver,reg3.Sepe.minor_ver,reg3.Sepe.revision, reg4.Sepe.major_ver,reg4.Sepe.minor_ver,reg4.Sepe.revision);
            sz_szInfo = tmp;
            std::cout << sz_szInfo;
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
    sprintf(tmp,"Exit HMCB0-DB%d resets CB logic flow before starting a new acquisition.\n", nMCB);
    sz_szInfo = tmp;
    std::cout << sz_szInfo;
    WriteLogFile(sz_szInfo);
    /////////////////////////////////////////////////////////////
    return ret;
}
/////////////////////////////////////////////////////////////////////////
bool CMCBCtrlC::ConfigFpga_HMCB4(unsigned short nMCB /*=0*/)
{
	/////////////////////////////////////////////////////////////////////////
	int i = 0;
	int j = 0;
	/////////////////////////////////////////////////////////////////////////
	std::string sz_szInfo = "";
	std::string filename = "";
	char tmp[512] = {0};
	////////////////////////////////////////////////////////////////
	std::string strBinfilePath = "";
	//判断是否存在 D:\\XRayV3\\Config\\Setting.ini 或 D:\\XRayV4\\Config\\Setting.ini文件且设置了_T("MainConfig"), _T("DeviceType"),参数
	DWORD nXRayV = IsExistXRayVxSetting();
	//
	if(3 == nXRayV)
	{
		strBinfilePath = ("D:\\XRayV3\\Config\\Binfile");
	}
	else if(4 == nXRayV)
	{
		strBinfilePath = ("D:\\XRayV4\\Config\\Binfile");
	}
	else
	{
		if(m_bPath)
		{
			
			strBinfilePath=m_szPath;
		}
		else
		{
			strBinfilePath = ("");
		}
	}
	/////////////////////////////////////////////////////////////
	//多端口的bin文件一次完成配置
	INT8U ret = 0;
	INT8U nLastError = 0;
	INT32S db_fpga_cfg_file_len_in_byte = 0;
	//
	for(i = 0; i < m_DMInfo[nMCB].nDBLineEnNum; i++)
	{
		ret = 0;
		db_fpga_cfg_file_len_in_byte = 0;
		//MCB dbline bin file 配置标志位
		//INT32U m_nDBLine_FPGAFileConfigFlag[MAX_MCBNUM][MAX_DBLINENUM];
		if(m_nDBLine_FPGAFileConfigFlag[nMCB][i] > 0)
		{
			filename = strBinfilePath + ("/") + m_szMCBDBLine_FPGAFilename[nMCB][i];
			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			int strLength = filename.length() + 1;
			char *pFPGAConfFile = new char[strLength];
			memset(pFPGAConfFile, 0, strLength);
            memcpy(pFPGAConfFile,filename.c_str(),strLength);
//			for(j = 0; j < strLength; j++)
//			{
//				pFPGAConfFile[j] = (char)(filename.at(j));
//			}
			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//sz_szInfo.Format(_T("HMCB(4) No.%d configuring FPGA file: %s, This process need about 1 minute, pls wait a moment.\n"), nMCB, filename) ;
			//sz_szInfo.Format(_T("HMCB(4) No.%02X configuring FPGA file: %s.\n"), m_nDBLine_FPGAFileConfigFlag[i], filename) ;
			memset(tmp,0,512);
			sprintf(tmp,"HMCB(4)%d-DB%d FPGA configuration file: %s.\n", nMCB, i, filename.c_str());
			sz_szInfo= tmp;
			std::cout << sz_szInfo;
			WriteLogFile(sz_szInfo);
			/////////////////////////////////////////////////////////////////////////
			ret = 0;
			//持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
			ETH_CTRL_read_buffer_residual_data(nMCB);
			/////////////////////////////////////////////////////////////////////////
			ret = EDAQ_CB_logic_reset(nMCB);			//CB逻辑复位
			usleep(5*1000);
			if(ret > 0)
			{
				nLastError++;
				memset(tmp,0,512);
                sprintf(tmp,"Error when calling EDAQ_CB_logic_reset(9) by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, i, ret);
				sz_szInfo = tmp;
				std::cout << sz_szInfo;
				WriteLogFile(sz_szInfo);
			}
			else
			{
				memset(tmp,0,512);
                sprintf(tmp,"No error when calling EDAQ_CB_logic_reset(9) by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, i, ret);
				sz_szInfo = tmp;
				std::cout << sz_szInfo;
				WriteLogFile(sz_szInfo, gLogLevel1);
			}
			/////////////////////////////////////////////////////////////////////////
			//持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
			ETH_CTRL_read_buffer_residual_data(nMCB);
			ret = EDAQ_RB_working_mode_set(0, nMCB);		//设置RB缓存工作模式. 1: CMD mode. 0: Auto mode.
			if(ret > 0)
			{
				nLastError++;
				memset(tmp,0,512);
				sprintf(tmp,"Error when calling EDAQ_RB_working_mode_set(0) by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, i, ret);
				sz_szInfo = tmp;
				std::cout << sz_szInfo;
				WriteLogFile(sz_szInfo);
			}
			else
			{
				memset(tmp,0,512);
				sprintf(tmp,"No error when calling EDAQ_RB_working_mode_set(0) by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, i, ret);
				sz_szInfo = tmp;
				std:cout << sz_szInfo;
				WriteLogFile(sz_szInfo, gLogLevel1);
			}

			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			db_fpga_cfg_file_len_in_byte = 0;
			//持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
			ETH_CTRL_read_buffer_residual_data(nMCB);
			//---------------------DB FPGA program part 1 start---------------------------------------
			//读取DB FPGA编程文件的数据，将其写入到HMCB的DDR缓存中
			//db_fpga_cfg_file_len_in_byte = EDAQ_read_and_write_DB_fpga_program_file_into_RB_via_ECS("E:\\work\\001_Container inspection system\\MainControlBox\\HMCB(4)\\HMCB201804_V5.2\\App\\CB_DAQ_V5.2\\DB_fpga_top.bin");
			db_fpga_cfg_file_len_in_byte = EDAQ_read_and_write_DB_fpga_program_file_into_RB_via_ECS(pFPGAConfFile, nMCB);
			delete pFPGAConfFile;
			pFPGAConfFile = NULL;
			//
			if(db_fpga_cfg_file_len_in_byte < 0)
			{
				/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				sz_szInfo = "The DB FPGA configuration file doesn't exist!\n";
				std::cout << sz_szInfo;
				WriteLogFile(sz_szInfo);
				/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                return false;
			}
			//持续从ETH读取数据，直到清空缓存中的数据。
			//持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
			ETH_CTRL_read_buffer_residual_data(nMCB);
			//
			//设置DB FPGA编程文件长度
			ret = EDAQ_set_DB_FPGA_CFG_data_byte_num(db_fpga_cfg_file_len_in_byte, nMCB);
			if(ret > 0)
			{
				nLastError++;
				memset(tmp,0,512);
				sprintf(tmp,"Error when calling EDAQ_set_DB_FPGA_CFG_data_byte_num() by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, i, ret);
				sz_szInfo = tmp;
				std::cout << sz_szInfo;
				WriteLogFile(sz_szInfo);
			}
			else
			{
				memset(tmp,0,512);
				sprintf(tmp,"No error when calling EDAQ_set_DB_FPGA_CFG_data_byte_num() by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, i, ret);
				sz_szInfo = tmp;
				std::cout << sz_szInfo;
				WriteLogFile(sz_szInfo, gLogLevel1);
			}
			usleep(10*1000);

			//EDAQ_read_DB_fpga_program_data_from_RB_via_ETH_and_dump_into_file();
			//持续从ETH读取数据，直到清空缓存中的数据。
			//持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
			ETH_CTRL_read_buffer_residual_data(nMCB);
			//
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
			//选择使能部分或全部DB Line，允许向其烧写DB FPGA程序
			ret = EDAQ_DB_Line_FPGA_program_enable(m_nDBLine_FPGAFileConfigFlag[nMCB][i], nMCB);
			if(ret > 0)
			{
				nLastError++;
				memset(tmp,0,512);
				sprintf(tmp,"Error when calling EDAQ_DB_Line_FPGA_program_enable(1) by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, i, ret);
				sz_szInfo = tmp;
				std::cout << sz_szInfo;
				WriteLogFile(sz_szInfo);
			}
			else
			{
				memset(tmp,0,512);
				sprintf(tmp,"No error when calling EDAQ_DB_Line_FPGA_program_enable(1) by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, i, ret);
				sz_szInfo = tmp;
				std::cout << sz_szInfo;
				WriteLogFile(sz_szInfo, gLogLevel1);
			}
			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
			ETH_CTRL_read_buffer_residual_data(nMCB);
			//请求开始向DB FPGA烧写程序
			EDAQ_DB_fpga_config_request(nMCB);
			//
			while (EDAQ_DB_FPGA_cfg_finished_flag_rd(nMCB) == 0)
			{
				loop_delay();
			}
			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//每次完成DB FPGA的程序烧写，必须复位HMCB(4) logic。从而，为后续FPGA程序烧写或数据采集做准备。
			//持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
			ETH_CTRL_read_buffer_residual_data(nMCB);
			ret = EDAQ_CB_logic_reset(nMCB);
            usleep(5);
			if(ret > 0)
			{
				nLastError++;
				memset(tmp,0,512);
                sprintf(tmp,"Error when calling EDAQ_CB_logic_reset(10) by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, i, ret);
				sz_szInfo =tmp;
				std::cout << sz_szInfo;
				WriteLogFile(sz_szInfo);
			}
			else
			{
				memset(tmp,0,512);
                sprintf(tmp,"No error when calling EDAQ_CB_logic_reset(10) by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, i, ret);
				sz_szInfo = tmp;
				std::cout << sz_szInfo;
				WriteLogFile(sz_szInfo);
			}
			if(pFPGAConfFile)
			{
				delete pFPGAConfFile;
				pFPGAConfFile = NULL;
			}
			//---------------------DB FPGA program part 1 end---------------------------------------
			memset(tmp,0,512);
			sprintf(tmp,"HMCB(4)%d-DB%d completes DB FPGA configuration.\n", nMCB, i);
			sz_szInfo = tmp;
			std::cout << sz_szInfo;
			WriteLogFile(sz_szInfo);
		}
		/////////////////////////////////////////////////////////////////////////
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//printf("The DB FPGA has been programmed.\n");
	//sz_szInfo.Format(_T("The DB FPGA has been programmed.\n")) ;
	//TRACE(sz_szInfo);
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
		sprintf(tmp,"Find errors in HMCB(4)%d EDAQ_RB_wr_port_sel(EDAQ_RB_WR_SEL_NO_DATA,...) function, the return value is %d.\n", nMCB, ret);
		sz_szInfo = tmp;
		std::cout << sz_szInfo;
		WriteLogFile(sz_szInfo);
	}
	else
	{
		memset(tmp,0,512);
		sprintf(tmp,"No errors in HMCB(4)%d EDAQ_RB_wr_port_sel(EDAQ_RB_WR_SEL_NO_DATA,...) function, the return value is %d.\n", nMCB, ret);
		sz_szInfo = tmp;
		std::cout << sz_szInfo;
		WriteLogFile(sz_szInfo);
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	ret = EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_NO_DATA, nMCB); //Stop reading RBOB.
	if(ret > 0)
	{
		nLastError++;
		memset(tmp,0,512);
		sprintf(tmp,"Find errors in HMCB(4)%d EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_NO_DATA,...) function, the return value is %d.\n", nMCB, ret);
		sz_szInfo = tmp;
		std::cout <<  sz_szInfo;
		WriteLogFile(sz_szInfo);
	}
	else
	{
		memset(tmp,0,512);
		sprintf(tmp,"No errors in HMCB(4)%d EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_NO_DATA,...) function, the return value is %d.\n", nMCB, ret);
		sz_szInfo = tmp;
		std::cout << sz_szInfo;
		WriteLogFile(sz_szInfo);
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	EDAQ_RBxB_reset(nMCB);		//reset RBIB AND RBOB
	//EDAQ_RB_wr_port_sel(EDAQ_RB_WR_SEL_DB_RX_DATA); 
	//EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_ETH_DATA);
	/////////////////////////////////////////////////////////////////////////
    usleep(10*1000);
	//---------------------DB FPGA program part 1 end---------------------------------------
	if(0 == nLastError)
	{
		memset(tmp,0,512);
		sprintf(tmp,"HMCB(4)%d completes DB FPGA configuration.\n", nMCB);
		sz_szInfo = tmp;
		std::cout <<  sz_szInfo;
		WriteLogFile(sz_szInfo);
		/////////////////////////////////////////////////////////////////////////
		m_MCBReadySts[nMCB] |=0x02;
		m_MCBReadySts[nMCB] |=0x04;
		//
		SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_ADFPGACFG_OK);
		//
		/////////////////////////////////
        return true;
	}
	else
	{
		memset(tmp,0,512);
		sprintf(tmp,"HMCB(4)%d DB FPGA configuration failed.\n", nMCB);
		sz_szInfo = tmp;
		std::cout << sz_szInfo;
		WriteLogFile(sz_szInfo);
        return false;
	}
	/////////////////////////////////////////////////////////////////////////
}

/////////////////////////////////////////////////////////////////////////
//通过用户参数设置增益模式 SETRANGE
int CMCBCtrlC::SetDMCmdReg_HMCB4(unsigned short nMCB)
{
    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    int i=0;
    int nRet = 0;
    //
    if (m_bStart)
    {
        std::cout << "error: HMCB(4) is running, can't set DM cmd register!\n";
        sz_szInfo = "error: HMCB(4) is running, can't set DM cmd register!\n";
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) +MCB_STSCD_SETREGATRUN);
        WriteLogFile(sz_szInfo);
        return 2;
    }
    //////////////////////////////////////////
    INT8U DB_line_num = m_DMInfo[nMCB].nDBLineEnNum;
    //设置各DBline的DM配置参数
    for(INT8U ndb = 0; ndb < DB_line_num; ndb++)
    {
	//通过用户参数设置增益模式 SETRANGE
	nRet += ConfigureUserParameter_HMCB4(ndb, nMCB);
    }
	/////////////////////////////////////////
    //参数设置成功
    if(0 == nRet)
    {
        memset(tmp,0,512);
        sprintf(tmp,"HMCB(4)%d successfully completed setting DMs' Reg value to : 0x%04X, 0x%04X, 0x%04X, 0x%04X...\n", nMCB,
			m_nMCBxDByREG[nMCB][0][0], m_nMCBxDByREG[nMCB][0][1], m_nMCBxDByREG[nMCB][0][2], m_nMCBxDByREG[nMCB][0][3]);
        sz_szInfo = tmp;
        std::cout << (sz_szInfo);
        WriteLogFile(sz_szInfo);
        //if set DM info
        m_bSetDMInfo[nMCB] = true;
    }
    //else
    if(0 != nRet)
    {
        memset(tmp,0,512);
        sprintf(tmp,"HMCB(4)%d failed to Set DMs' Reg value.The error code returned by the function is : %d.\n", nMCB, nRet);
        sz_szInfo = tmp;
        std::cout << (sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    return nRet;
}

//////////////////////////////////////////////////////////////////////////////////
//配置HCB0的nDBline上探测器参数
int CMCBCtrlC::ConfigureUserParameter_HMCB4(INT8U nDbline, unsigned short nMCB/*=0*/)
{
    std::string sz_szInfo = ("");
    std::string str = ("");
    char tmp[512] = {0};
    //////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////
    //参数长度：DB数目*2
    //其中，第i*2～i*2+1个字节的参数对应着编号为i的DB的参数。
    //将字节转化为参数时，按照大端格式进行，即序号小的参数对应着参数的高字节。
    INT8U i,dbline_i;
    INT8U wr_prm[256] = { 0x00 };
    INT8U rd_prm[256] = { 0x00 };
    INT8U nDMNum = m_DMInfo[nMCB].nDBDMNUM[nDbline];
    INT8U prm_len = 2*nDMNum;
    INT8U nFucRet = 0;//函数返回值
    INT8U nLastError = 0;//异常标识值
    //
    memset(tmp,0,512);
    sprintf(tmp,"Configure HMCB(4)%d-DB%d %d DMs parameters:", nMCB, nDbline, m_DMInfo[nMCB].nDBDMNUM[nDbline]);
    sz_szInfo += tmp;
    //先发离控制模块最远一个模块配置参数，后发离控制模块最近一个模块配置参数
    for(i=0; i < nDMNum; i++)
    {
        memset(tmp,0,512);
        sprintf(tmp,"[DM%d]-0x%04X, ", i, m_nMCBxDByREG[nMCB][nDbline][nDMNum - 1 - i]);
        sz_szInfo += tmp;
        //
	wr_prm[2*i]     = (m_nMCBxDByREG[nMCB][nDbline][nDMNum - 1 - i] & 0xff00)>>8;
	wr_prm[2*i + 1] = (m_nMCBxDByREG[nMCB][nDbline][nDMNum - 1 - i] & 0x00ff);
        //指定MCBx是否进入诊断模式，是，则配置参数最高位第15位（最低位为0位）置1
        if(m_bMCBxDTM[nMCB])
        {
            wr_prm[2*i] |= 0x80;
        }
    }
    std::cout << sz_szInfo << std::endl ;

    WriteLogFile(sz_szInfo +("\n"));
    //////////////////////////////////////////////////////////////////////////////////
    //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
    ETH_CTRL_read_buffer_residual_data(nMCB);
    /////////////////////////////////////////////////////////////////////////////
    //Config the control bus between CB and DB to be used as communication bus
    EDAQ_DB_FPGA_cfg_enable(0, nMCB);
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
        sprintf(tmp,"Error when calling EDAQ_RB_working_mode_set(1) by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, nDbline, nFucRet);
        sz_szInfo = tmp;
        std::cout << (sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"No error when calling EDAQ_RB_working_mode_set(1) by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, nDbline, nFucRet);
        sz_szInfo = tmp;
        std::cout << (sz_szInfo);
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
        sprintf(tmp,"Error when calling EDAQ_RB_wr_port_sel(EDAQ_RB_WR_SEL_DB_RX_DATA) by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, nDbline, nFucRet);
        sz_szInfo = tmp;
        std::cout << (sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"No error when calling EDAQ_RB_wr_port_sel(EDAQ_RB_WR_SEL_DB_RX_DATA) by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, nDbline, nFucRet);
        sz_szInfo = tmp;
        std::cout << (sz_szInfo);
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
        sprintf(tmp,"Error when calling EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_ETH_DATA) by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, nDbline, nFucRet);
        sz_szInfo = tmp;
        std::cout <<  (sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"No error when calling EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_ETH_DATA) by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, nDbline, nFucRet);
        sz_szInfo = tmp;
        std::cout << (sz_szInfo);
        WriteLogFile(sz_szInfo, gLogLevel1);
    }
    //////////////////////////////////////////////////////////////////////////////////
    //持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
    ETH_CTRL_read_buffer_residual_data(nMCB);
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
        sprintf(tmp,"Error when calling EDAQ_SC_data_pkg_monitor_enable(1) by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, nDbline, nFucRet);
        sz_szInfo = tmp;
        std::cout << (sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"No error when calling EDAQ_SC_data_pkg_monitor_enable(1) by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, nDbline, nFucRet);
        sz_szInfo = tmp;
        std::cout << (sz_szInfo);
        WriteLogFile(sz_szInfo, gLogLevel1);
    }
    /////////////////////////////////////////////////
    //EDAQ_DB_prm_cfg_RAM_test();
    dbline_i = nDbline;
    int nLoop = 0;
    //for(dbline_i = 0; dbline_i < 4; dbline_i++)
    do
    {
	//////////////////////////////////////////////////////////////////////////////////
	//持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
	ETH_CTRL_read_buffer_residual_data(nMCB);
	//////////////////////////////////////////////////////
	//功能：把参数写入到参数配置RAM中，并检查写入是否正确
	//nFucRet = EDAQ_DB_prm_cfg_RAM_write_with_check(dbline_i,wr_prm, prm_len, nMCB);
	nFucRet = EDAQ_DB_prm_cfg_RAM_write_with_check(dbline_i, wr_prm, rd_prm, prm_len, nMCB);
	////////////////////////////////////////////////////////////////////////
	sz_szInfo = ("");
	memset(tmp,0,512);
	sprintf(tmp,"Read back HMCB(4)%d-DB%d %d DMs parameters:", nMCB, dbline_i, nDMNum);
	str = tmp;
	sz_szInfo += str;
	//
	for(i=0; i < nDMNum; i++) 
	{
		memset(tmp,0,512);
		sprintf(tmp,"[DM%d]-0x%02X%02X, ", i, rd_prm[i*2], rd_prm[i*2+1]);
		str = tmp;
		sz_szInfo += str;
	}
	std::cout << sz_szInfo << std::endl;
	WriteLogFile(sz_szInfo + "\n");
	///////////////////////////////////////////////////////////////////////
        if(nFucRet > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_DB_prm_cfg_RAM_write_with_check(dbline_i,wr_prm, prm_len) by HMCB%d-DB%d. The return value is %d.\n", nMCB, nDbline, nFucRet);
            sz_szInfo = tmp;
            std::cout << (sz_szInfo);
            WriteLogFile(sz_szInfo);
            nLoop++;
        }
        else
        {
            memset(tmp,0,512);
            sprintf(tmp,"No error when calling EDAQ_DB_prm_cfg_RAM_write_with_check(dbline_i,wr_prm, prm_len) by HMCB%d-DB%d. The return value is %d.\n", nMCB, nDbline, nFucRet);
            sz_szInfo = tmp;
            std::cout << (sz_szInfo);
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
        sprintf(tmp,"Error when calling EDAQ_DB_reset_req() by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, nDbline, nFucRet);
        sz_szInfo = tmp;
        std::cout << (sz_szInfo);
        WriteLogFile(sz_szInfo);
    }
    else
    {
        memset(tmp,0,512);
        sprintf(tmp,"No error when calling EDAQ_DB_reset_req() by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, nDbline, nFucRet);
        sz_szInfo = tmp;
        std::cout << (sz_szInfo);
        WriteLogFile(sz_szInfo, gLogLevel1);
    }
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    Sleep(10);
#else
    usleep(10*1000);
#endif
    /////////////////////////////////////////////////
    //功能：发出DB定位请求
    //locate
    nFucRet = EDAQ_DB_locate_req(nMCB);
    if(nFucRet > 0)
    {
        nLastError++;
        char tmp[512] = {0};
        sprintf(tmp,"Error when calling EDAQ_DB_locate_req() by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, nDbline, nFucRet);
        sz_szInfo = tmp ;
        std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
    }
    else
    {
        char tmp[512] = {0};
        sprintf(tmp,"No error when calling EDAQ_DB_locate_req() by HMCB(4)%d-DB%d. The return value is %d.\n", nMCB, nDbline, nFucRet);
        sz_szInfo = tmp ;
        std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo, gLogLevel1);
    }
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    Sleep(10);
#else
    usleep(10*1000);
#endif
    /////////////////////////////////////////////////
    //功能：发出DB参数配置请求
    //DB prm cfg
    nFucRet = EDAQ_DB_prm_cfg_req(nMCB);
    if(nFucRet > 0)
    {
        char tmp[512] = {0};
        sprintf(tmp,"Error when calling EDAQ_DB_prm_cfg_req() by HMCB%d-DB%d. The return value is %d.\n", nMCB, nDbline, nFucRet);
        sz_szInfo = tmp;
        std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo);
    }
    else
    {
        char tmp[512] = {0};
        sprintf(tmp,"No error when calling EDAQ_DB_prm_cfg_req() by HMCB%d-DB%d. The return value is %d.\n", nMCB, nDbline, nFucRet);
        sz_szInfo = tmp;
        std::cout << sz_szInfo;
        WriteLogFile(sz_szInfo, gLogLevel1);
    }
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    Sleep(10);
#else
    usleep(10*1000);
#endif
    //当设置小于4个时耗时较长，注释掉不影响功能
    //ECM_read_and_parse_data(1, DB_RST_PKG, 1, 1, 0, cmd_pkg_parse_log_file,0);
    //ECM_read_and_parse_data(1, DB_LCT_PKG, 1, 1, 0, cmd_pkg_parse_log_file,0);
    //ECM_read_and_parse_data(1, DB_PRM_CFG_PKG, 1, 1, 0, cmd_pkg_parse_log_file,0);
    //
    //////////////////////////////////////////////////////////////////////////////////
    int nRet = 0;
    nLoop = 0;
//  do{
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
		//sz_szInfo.Format(_T("HMCB(4)%d Auto DM number:%d.\n"), nMCB, 0);
		//TRACE(sz_szInfo);
		//WriteLogFile(sz_szInfo);
		//
		m_MCBReadySts[nMCB] |=0x08;
		//SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_ADFPGACFG_OK);
	}
	//else
	//模块数自检与设置值不同，则需重新初始化
	if(0 != nRet)
	{
		//////////////////////////////////////////////////////////////////////////////////
		memset(tmp,0,512);
		sprintf(tmp,"HMCB(4)%d failed to self check the number of detector modules. The device will be reconnected..\n", nMCB);
		sz_szInfo = tmp;
		std::cout << sz_szInfo;
		WriteLogFile(sz_szInfo);
		//sz_szInfo.Format(_T("The setting value of HMCB(4)%d DB num of DB-Line0 is not equal to the parsed result of DB num of DB-Line0.\n"), nMCB);
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

//////////////////////////////////////////////////////////////////
//通过socket获取数据并保存至FIFO缓存中
void CMCBCtrlC::FetchMsg_HMCB4(CRequestSocket *pRequestSocket, int nIndex, int nPort)
{
    std::string sz_szInfo = ("");
    char tmp[512] = {0};
	memset(tmp,0,512);
//	sprintf(tmp,"HMCB(4)%d Socket Receives data and jump in FetchMsg_HMCB4() function.\n");
//	sz_szInfo = tmp;
//	WriteLogFile(sz_szInfo);

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    EnterCriticalSection(&ics);
    //
    WaitForSingleObject(hMutex, INFINITE);
#else
    //boost::unique_lock<boost::shared_mutex> /*read_*/lock(mutex1);
    //unique_lock<mutex> lock(_mutex);
    unique_lock<mutex> lock(m_Data_mutex[nIndex]);
#endif
    //

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
			//Rec_Bytelen = pRequestSocket->Receive(&glb_data_buf[nMCB][0], GLB_DATA_BUF_SIZE);
			unsigned char * chRecv = new unsigned char [SOCKETBUFSIZE];
			Rec_Bytelen = pRequestSocket->Receive(chRecv, SOCKETBUFSIZE);
			//std::vector<unsigned char> TempVector(chRecv, chRecv+Rec_Bytelen);
			//glb_data_buf_vetor[nMCB].insert(glb_data_buf_vetor[nMCB].end(), TempVector.begin(), TempVector.end());
			delete [] chRecv;
			//
			glb_data_buf_vetor[nMCB].clear();
			m_timeSocketRecData[nMCB] =NucTool::GetTickCount();
			//
			glb_data_AccWrBytes[nMCB] = 0;
			m_SocketRecBytes[nMCB] = 0;
			m_TimeSaveSocketData[nMCB] = NucTool::GetTickCount();
			//#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
			//        ReleaseMutex(hMutex);
			//#else
			//        lock.unlock();
			//#endif
			memset(tmp,0,512);
			sprintf(tmp,"HMCB(4)%d Socket Receives data before [Start], with a return data length and total receive data length are : %d / %I64u \n", nMCB, Rec_Bytelen, glb_data_AccWrBytes[nMCB] );
			sz_szInfo = tmp;
			WriteLogFile(sz_szInfo, gLogLevel2);
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
        buf_WR_Pt = gbl_data_buf_wraddr[nMCB] % GLB_DATA_BUF_SIZE;
        buf_WR_EmptyLength = GLB_DATA_BUF_SIZE - buf_WR_Pt;
        //从socket接收数据放到缓存区
        if(nMCB ==1)
        {
            int n=0;
        }
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
				//int nsize = glb_data_buf_vetor[nMCB].size();
				//int n=0;
				m_timeSocketRecData[nMCB] = NucTool::GetTickCount();
				//
				buffDataLength = glb_data_buf_vetor[nMCB].size();
				//缓存区内有大于等于2次触发的未处理数据
				if(buffDataLength >= 2*gbl_MCBTotal_Size[nMCB])
				{
					memset(tmp,0,512);
					sprintf(tmp,"HMCB(4)%d Socket Received data Length=%d and go to analysis.\n", nMCB, buffDataLength );
					sz_szInfo = tmp;
					WriteLogFile(sz_szInfo, gLogLevel1);
					//
					RECVPARAM *pRecvParam0 = new RECVPARAM;
					pRecvParam0->nMCBIndex = nMCB;
					pRecvParam0->nDBLineEnNum = m_DMInfo[nMCB].nDBLineEnNum;
					pRecvParam0->pkg_num = m_lcounter;
                    ETH_read_and_parse_dataEx_Thread_HMCB4((void*) pRecvParam0);
				}
			}
            delete [] chRecv;
		}



//        std::cout << " buf_WR_Pt : " << buf_WR_Pt << std::endl;
//        for(int i=0;i<16;i++)
//        {
//            std::cout << " FetchMsg_HMCB4 : " << std::hex << (int)(&glb_data_buf[nMCB][buf_WR_Pt])[i] << "  ";
//        }
//        std::cout << std::dec << std::endl;

        if(Rec_Bytelen <= 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling socket Receive() by HMCB%d. The return value is : %d \n", nMCB, Rec_Bytelen);
            sz_szInfo = tmp;
            //WriteLogFile(sz_szInfo);
           // lock.unlock();
            return;
        }
        else if(Rec_Bytelen > 0)
        {
//            gbl_data_buf_wraddr[nMCB] = (buf_WR_Pt + Rec_Bytelen) % GLB_DATA_BUF_SIZE;
//            glb_data_AccWrBytes[nMCB] += Rec_Bytelen;
            if(gLogLevel2 <= m_nLogLevel)
            {
                memset(tmp,0,512);
                sprintf(tmp,"HMCB(4)%d socket Received data successfully. The lengths of returned data and total received data are : %d / %I64u \n", nMCB, Rec_Bytelen, glb_data_AccWrBytes[nMCB] );
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
                m_SocketRawDatafile<<sz_szInfo;
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
                sprintf(tmp,"HMCB(4)%d the lengths of returned data and band width: %I64u bytes / %d Bytes/s\n", nMCB, glb_data_AccWrBytes[nMCB], (int)(m_SocketRecBytes[nMCB]*1000.0/dwLapTick));
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

//////////////////////////////////////////////////////////////////
//多线程解析数据
void CMCBCtrlC::ReadParseData_HMCB4(ULONG pkg_num)
{
    	std::string sz_szInfo = ("");
    	sz_szInfo = "Enter ReadParseData_HMCB4(). \n";
    	WriteLogFile(sz_szInfo, gLogLevel2);

	////////////////////////////////////////////////////////////////////////////////////////////////////
	int i = 0;
	for (i = 0; i < m_nMCBNum; i++)
	{
		if( 0 == i)
		{
            std::thread th([=]{
                RECVPARAM *pRecvParam0 = new RECVPARAM;
                pRecvParam0->nMCBIndex = 0;
                pRecvParam0->nDBLineEnNum = m_DMInfo[0].nDBLineEnNum;
                pRecvParam0->pkg_num = pkg_num;
            		ETH_read_and_parse_dataEx_Thread_HMCB4(pRecvParam0);
    			});
    			th.detach();
		}
		if( 1 == i)
		{

            std::thread th([=]{
                    RECVPARAM *pRecvParam1 = new RECVPARAM;
                    pRecvParam1->nMCBIndex = 1;
                    pRecvParam1->nDBLineEnNum = m_DMInfo[1].nDBLineEnNum;
                    pRecvParam1->pkg_num = pkg_num;
            		ETH_read_and_parse_dataEx_Thread_HMCB4(pRecvParam1);
    			});
    			th.detach();
		}
		if( 2 == i)
		{

            std::thread th([=]{
                    RECVPARAM *pRecvParam2 = new RECVPARAM;
                    pRecvParam2->nMCBIndex = 2;
                    pRecvParam2->nDBLineEnNum = m_DMInfo[2].nDBLineEnNum;
                    pRecvParam2->pkg_num = pkg_num;
            		ETH_read_and_parse_dataEx_Thread_HMCB4(pRecvParam2);
    			});
    			th.detach();
		}
		if( 3 == i)
		{

            std::thread th([=]{
                    RECVPARAM *pRecvParam3 = new RECVPARAM;
                    pRecvParam3->nMCBIndex = 3;
                    pRecvParam3->nDBLineEnNum = m_DMInfo[3].nDBLineEnNum;
                    pRecvParam3->pkg_num = pkg_num;
            		ETH_read_and_parse_dataEx_Thread_HMCB4(pRecvParam3);
    			});
    			th.detach();
		}
		if( 4 == i)
		{

            std::thread th([=]{
                    RECVPARAM *pRecvParam4 = new RECVPARAM;
                    pRecvParam4->nMCBIndex = 4;
                    pRecvParam4->nDBLineEnNum = m_DMInfo[4].nDBLineEnNum;
                    pRecvParam4->pkg_num = pkg_num;
            		ETH_read_and_parse_dataEx_Thread_HMCB4(pRecvParam4);
    			});
    			th.detach();
		}
		if( 5 == i)
		{

            std::thread th([=]{
                    RECVPARAM *pRecvParam5 = new RECVPARAM;
                    pRecvParam5->nMCBIndex = 5;
                    pRecvParam5->nDBLineEnNum = m_DMInfo[5].nDBLineEnNum;
                    pRecvParam5->pkg_num = pkg_num;
            		ETH_read_and_parse_dataEx_Thread_HMCB4(pRecvParam5);
    			});
    			th.detach();
		}
		if( 6 == i)
		{

            std::thread th([=]{
                    RECVPARAM *pRecvParam6 = new RECVPARAM;
                    pRecvParam6->nMCBIndex = 6;
                    pRecvParam6->nDBLineEnNum = m_DMInfo[6].nDBLineEnNum;
                    pRecvParam6->pkg_num = pkg_num;
            		ETH_read_and_parse_dataEx_Thread_HMCB4(pRecvParam6);
    			});
    			th.detach();
		}
		if( 7 == i)
		{

            std::thread th([=]{
                    RECVPARAM *pRecvParam7 = new RECVPARAM;
                    pRecvParam7->nMCBIndex = 7;
                    pRecvParam7->nDBLineEnNum = m_DMInfo[7].nDBLineEnNum;
                    pRecvParam7->pkg_num = pkg_num;
            		ETH_read_and_parse_dataEx_Thread_HMCB4(pRecvParam7);
    			});
    			th.detach();
		}
	}
  


}

//////////////////////////////////////////////////////////////////
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
DWORD WINAPI CMCBCtrlC::ETH_read_and_parse_dataEx_Thread_HMCB4(LPVOID lpParameter)
#else
DWORD  CMCBCtrlC::ETH_read_and_parse_dataEx_Thread_HMCB4(void * lpParameter)
#endif
{
	std::string  sz_szInfo = ("");
	char tmp[512] = {0};
	//对应的MCB索引
	INT8U nMCB = ((RECVPARAM *)lpParameter)->nMCBIndex;
	UINT nDBLineEnNum = ((RECVPARAM *)lpParameter)->nDBLineEnNum;
	if((nDBLineEnNum < 1) || (nDBLineEnNum > 4))
	{
		nDBLineEnNum = 1;
	}
	UINT nMaxDBLindIndex = nDBLineEnNum - 1;
	//一次采集需要获取的触发数据包数量
	//0:无限包数
	//>0:需要目标包数
	ULONG nTarget_pkg_num = ((RECVPARAM *)lpParameter)->pkg_num;

	memset(tmp,0,512);
//	sprintf(tmp," ETH_read_and_parse_dataEx_Thread_HMCB4 : nTarget_pkg_num is %d . \n", nTarget_pkg_num);
//	sz_szInfo = tmp;
//	WriteLogFile(sz_szInfo);

	delete lpParameter;
	//
	if(nMCB >= MAX_MCBNUM )
	{
		memset(tmp,0,512);
		sprintf(tmp,"HMCB(4) Index Parameter of ETH_read_and_parse_dataEx_Thread function is %d, bigger than MAX_MCBNUM = %d. \n", nMCB, MAX_MCBNUM);
		sz_szInfo = tmp;
		WriteLogFile(sz_szInfo);
		return 0;
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////
	//memset(tmp,0,512);
	//sprintf(tmp,"Enter HMCB(4)%d ETH_read_and_parse_dataEx_Thread.\n", nMCB);
	//sz_szInfo = tmp;
	//WriteLogFile(sz_szInfo);
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
				if(parsed_pkg_n[i] < nTarget_pkg_num)
				{
					tB1 = false;
				}
			}
			if((tB1) && (!m_bPreStop))
			{
				memset(tmp,0,512);
				sprintf(tmp,"HMCB0(4) has collected %d data and will stop(1) data acquisition.\n", parsed_pkg_n[0]);
				sz_szInfo = tmp;
				WriteLogFile(sz_szInfo);
				//
				Stop(1,nMCB);
				//break;
                return 0;
			}
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////
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
		//sz_szInfo.Format(_T("Enter MCB%d ETH_read_and_parse_dataEx_Thread. glb_data_AccWrBytes[nMCB]= %I64u; glb_data_AccRdBytes[nMCB]=%I64u; and buffDataLength = %I64u;\n"), 
		//	nMCB, glb_data_AccWrBytes[nMCB], glb_data_AccRdBytes[nMCB], buffDataLength);
		//WriteLogFile(sz_szInfo, gLogLevel2);
		////////////////////////////////////////////////////////////////////////////////////////////////////
		//缓存区内有大于等于一次触发的未处理数据
		//while(buffDataLength >= gbl_DBLine_Size[nMCB][0])
		while(buffDataLength >= 2*gbl_MCBTotal_Size[nMCB])
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
			buf_data1 = 0; buf_data2 = 0; buf_data3 = 0; buf_data4 = 0; buf_data5 = 0; buf_data6 = 0; buf_data7 = 0; buf_data8 = 0; buf_data9 = 0; buf_data10 = 0; 
			//
			buffDataLength = glb_data_buf_vetor[nMCB].size();
			if(buffDataLength < gbl_MCBTotal_Size[nMCB])
			{
				memset(tmp,0,512);
				sprintf(tmp,"HMCB%d(4) has not enough data for analysis and then the process interruption.[glb_data_buf_vetor[nMCB].size()=%d].\n", nMCB, buffDataLength);
				sz_szInfo = tmp;
				WriteLogFile(sz_szInfo);
				break;
			}
			//////////////////////////////////////////////////////////////////////////////////////////////
			if(buffDataLength > (i+10))
			{
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
			}
			else
			{
				memset(tmp,0,512);
				sprintf(tmp,"HMCB%d(4) has not enough data for analysis and then the process interruption.[glb_data_buf_vetor[nMCB].size()=%d].\n", nMCB, buffDataLength);
				sz_szInfo = tmp;
				WriteLogFile(sz_szInfo);
				break;
			}
			////每次数据解析起始地址
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
			//if((SC_pkg_db_line < 0) || (SC_pkg_db_line > 3))
			//{
			//	SC_pkg_db_line = 0;
			//}
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
					sprintf(tmp,"Not find SC_PKG_LEAD_CODE(0xAA55) when PM_GET_SC_PKG_HEADER with MCB%d-DB%d.\n", nMCB, SC_pkg_db_line);
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
					//ReleaseMutex(hMutex);
					//lock.unlock();
					//////////////////////////////////////////////////////////////////
					//LeaveCriticalSection(&ics);
					//////////////////////////////////////////////////////////////////
					memset(tmp,0,512);
					sprintf(tmp,"Not find SC_PKG_LEAD_CODE(0xAA55) more than 65535*2 times and Exit HMCB(4)%d ETH_read_and_parse_dataEx_Thread.\n", nMCB);
					sz_szInfo = tmp;
					WriteLogFile(sz_szInfo);
					//////////////////////////////////////////////////////////////////
					//上一次数据采集是否成功完成标志：失败
					m_bPreDAQSucessFlag[nMCB] = false;
					//////////////////////////////////////////////////////////////////
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
					// lock.unlock();
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
			else if((DB_DAQ_PKG != SC_pkg_type) || (SC_pkg_len != gbl_DBLine_Size[nMCB][SC_pkg_db_line]))
			{
				memset(tmp,0,512);
				sprintf(tmp,"Get Error when parse PM_GET_SC_PKG_HEADER data of HMCB(4)%d DB%d, return data length is : %d, need data length is %d. \n", nMCB, SC_pkg_db_line, SC_pkg_len, gbl_DBLine_Size[nMCB][SC_pkg_db_line]);
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
						for(i = 0; i < min(4096, (int)xB_pkg_size); i++)
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
						for(i = 0; i < min(16, (int)xB_pkg_size); i++)
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
						////////////////////////////////////////////////////////////////////////////////////////////////////
						//待确认
						//sz_szInfo.Format(_T("At end trigger_%d data analyse of HMCB(4)%d, gbl_data_buf_rdaddr[nMCB] = %d, glb_data_AccWrBytes[nMCB]= %I64u, glb_data_AccRdBytes[nMCB]=%I64u,  buffDataLength = %I64u\n"),
						//	pDBDataPackage.SC_pkg_index, nMCB, gbl_data_buf_rdaddr[nMCB], glb_data_AccWrBytes[nMCB], glb_data_AccRdBytes[nMCB], buffDataLength);
						//WriteLogFile(sz_szInfo, gLogLevel2);
						////////////////////////////////////////////////////////////////////////////////////////////////////
						//执行到这里，说明解析完一个数据包
						parse_mode = PM_START;
						////////////////////////////////////////////////////////////////////////////////////////////////////
						////////////////////////////////////////////////////////////////////////////////////////////////////
						//完成一次db0的解析，正确解析的数据包个数递增1
						if(nMaxDBLindIndex == pDBDataPackage.DB_line_i) 
						{
							parsed_pkg_n[nMCB]++;//已经正确解析的数据包个数递增1
						}
						//获取保存每一个HMCB(4) DB0 返回的第一个数据包
						//if(1 == parsed_pkg_n[nMCB])
						if((1 == parsed_pkg_n[nMCB]) && (nMaxDBLindIndex == pDBDataPackage.DB_line_i))
						{
							SC_1st_pkg_index[nMCB] = pDBDataPackage.SC_pkg_index;
							////每一个DB Line返回的第一个数据包内触发计数
							SC_1st_pkg_TriggCnt[nMCB] = (pDBDataPackage.CB_data[8]<<24)+(pDBDataPackage.CB_data[9]<<16)+(pDBDataPackage.CB_data[10]<<8)+(pDBDataPackage.CB_data[11]);// CountPos & CountNeg(16bit + 16bit)
						}
						////////////////////////////////////////////////////////////////////////////////////////////////////
						//每一个HMCB(4)返回的当前数据包索引(已考虑数据包溢出重新计数)
						//控制盒返回数据包编号2个字节:0~65535
						//完成一次db0的解析
						if( nMaxDBLindIndex == pDBDataPackage.DB_line_i) 
						{
							//每一个DB Line返回的当前数据包索引(已考虑数据包溢出重新计数)
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
								sprintf(tmp,"Get 0xA5 data packet from HMCB(4)%d DB%d. \n", nMCB, SC_pkg_db_line);
								sz_szInfo = tmp;
								WriteLogFile(sz_szInfo);
								//
								//是否已获取到开始的0xA5数据包
								m_bGet0xA5Packet[nMCB] = true;
							}
							//
							if(!m_bGet0xA5Packet[nMCB])
							{
								//获取到0xA5数据包前已解析出的数据包
								m_nMiss0XA5PacketCnt[nMCB]++;
								if(1 == (m_nMiss0XA5PacketCnt[nMCB] % 100))
								{
									memset(tmp,0,512);
									sprintf(tmp,"HMCB(4)%d DB0 has received %d packets, but has not received 0xA5 packets.\n", nMCB, m_nMiss0XA5PacketCnt[nMCB]);
									sz_szInfo = tmp;
									WriteLogFile(sz_szInfo);
								}
							}
						}
						////////////////////////////////////////////////////////////////////////////////////
						//是否要求获取开始的0xA5数据包,默认为需要获取
						if(!m_bNeedGet0xA5)
						{
							m_bGet0xA5Packet[nMCB] = true;
						}
						//parsed_pkg_n : 1次取数中软件触发计数，从0开始
						//前面(INVALID_DATA_PACKET_NUM - m_nDiscardNum[nMCB])个触发舍弃，不传数据给APP
						//if((m_bGet0xA5Packet[nMCB]) && ((parsed_pkg_n[nMCB] + m_nDiscardNum[nMCB]) > INVALID_DATA_PACKET_NUM))
						//2025.09.18 针对多端口第一次采集数据异常问题修改
						if((m_bGet0xA5Packet[nMCB]) && ((parsed_pkg_n[nMCB] + m_nDiscardNum[nMCB]) >= INVALID_DATA_PACKET_NUM))
						//if((parsed_pkg_n[nMCB] > (INVALID_DATA_PACKET_NUM - m_nDiscardNum[nMCB])))
						{
							//经调试，发现第1个(序号0)数据包数据不全
							//原因是数据包解析最后一个DB后，parsed_pkg_n[nMCB]增1，满足此if条件，调用UpdateOnePacketDataToApp_HMCB4(&pDBDataPackage);
							//但此时除最后一个DB的数据会解析保存外，其他DB数据都没有解析保存，因此这时需要阻止调用UpdateOnePacketDataToApp_HMCB4(&pDBDataPackage)
							//条件:当条件(parsed_pkg_n[nMCB] + m_nDiscardNum[nMCB]) == INVALID_DATA_PACKET_NUM时，遇到最后一个DB数据，不再往下执行
							//保证条件(parsed_pkg_n[nMCB] + m_nDiscardNum[nMCB]) > INVALID_DATA_PACKET_NUM时，所有DB的数据均已更新
							if((parsed_pkg_n[nMCB] + m_nDiscardNum[nMCB]) == INVALID_DATA_PACKET_NUM)
							{								
								//一个CB板的最后一个DB数据更新后，数据包裹数才增1
								if( nMaxDBLindIndex == pDBDataPackage.DB_line_i)
								{
									////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
									//2024-01-08????????????????????????????????????????????????????????????
									////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
									//数据包异常，但需更新包头数据
									UINT nError_DI = ((~pDBDataPackage.CB_data[2])& 0xF0)<<4;//????????;//Data 2????4??????????DDI
									if(m_bGoudang_S4)
									{
										nError_DI = (nError_DI & 0x0B00) + ((nError_DI&0x0200)<< 1);//????????????????????????
									}
									//??????????????????????
									long nPulseCounter = 0;
									if(m_bHEorCXMode)
									{
										//????
										nPulseCounter = (pDBDataPackage.CB_data[8]<<24)+(pDBDataPackage.CB_data[9]<<16)+(pDBDataPackage.CB_data[10]<<8)+(pDBDataPackage.CB_data[11]);
										nPulseCounter = nPulseCounter - SC_1st_pkg_TriggCnt[nMCB] - INVALID_DATA_PACKET_NUM + m_nDiscardNum[nMCB] + 1;
									}
									else
									{
										//????
										nPulseCounter = pDBDataPackage.SC_pkg_index;
										nPulseCounter = nPulseCounter - INVALID_DATA_PACKET_NUM;
									}
									//
									m_MCBDataHeadTmp[nMCB].head_use.scounter = pDBDataPackage.nSocketCounter;//nSocketCounter;	// socket packet serial number, self-add//????????????,1????
									m_MCBDataHeadTmp[nMCB].head_use.pcounter.l = nPulseCounter;//nPulseCounter;	// pulse trigger number//??????????????????????
									//m_MCBDataHeadTmp[nMCB].head_use.accEnergy = ((nError_DI&0x0200) ==0) ? 1 : 0;// acc energy sign
									//2024.11.04根据调试反馈，高能时给软件能量指示信号为0，正常应该为1，故改为下面逻辑
									m_MCBDataHeadTmp[nMCB].head_use.accEnergy = ((nError_DI&0x0200) ==0) ? 0 : 1;// acc energy sign
									///////////////////////////////////////////////////////////////////
									UINT nDI3 = ((nError_DI&0x0800) == 0) ? 0 : 1;// acc energy sign
									UINT nDI1 = ((nError_DI&0x0200) == 0) ? 0 : 1;// acc energy sign
									//适配三能加速器（DI(3)为高，则为0.7MeV，否则为高低能）
									if(m_bThreeEnergyAcc_En)
									{
										if((0 == nDI3) && (0 == nDI1)) //00超低
										{
											m_MCBDataHeadTmp[nMCB].head_use.accEnergy = 2;
										}
										else if((0 == nDI3) && (1 == nDI1)) //01超低
										{
											m_MCBDataHeadTmp[nMCB].head_use.accEnergy = 2;
										}
										else if((1 == nDI3) && (0 == nDI1))//10低能
										{
											m_MCBDataHeadTmp[nMCB].head_use.accEnergy = 0;
										}
										else//11高能
										{
											m_MCBDataHeadTmp[nMCB].head_use.accEnergy = 1;
										}
									}
									////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
									//CB??????????????12????????????,12????????
									//Data 0??DB line??????????4??????SDCLK????????????????4??????SDCLK??????????
									//Data 1??DB line??????????????????????
									//Data 2????4????????????DDI????4????????????0????????????
									//Data 3????????????????
									//Data 4??11: ??????????????
									//m_MCBDataHeadTmp[nMCB].d_head.head_use.ain0.ul =		(pDBDataPackage.CB_data[4]<<24)+(pDBDataPackage.CB_data[5]<<16)+(pDBDataPackage.CB_data[6]<<8)+(pDBDataPackage.CB_data[7]);// 24bit ADC??(????24bit + 8Bit (0x07))
									m_MCBDataHeadTmp[nMCB].head_use.ain0.ul =		(pDBDataPackage.CB_data[4]<<16)+(pDBDataPackage.CB_data[5]<<8)+(pDBDataPackage.CB_data[6]);//24bit ADC??(????24bit + 8Bit (0x07))
									m_MCBDataHeadTmp[nMCB].head_use.ptsmdata_h =	(pDBDataPackage.CB_data[0]<<24)+(pDBDataPackage.CB_data[1]<<16)+(pDBDataPackage.CB_data[2]<<8)+(pDBDataPackage.CB_data[3]);// ????0??3??????????
									//m_MCBDataHeadTmp[nMCB].d_head.head_use.ptsmdata_l =	(pDBDataPackage.CB_data[8]<<24)+(pDBDataPackage.CB_data[9]<<16)+(pDBDataPackage.CB_data[10]<<8)+(pDBDataPackage.CB_data[11]);// CountPos(32bit)
									m_MCBDataHeadTmp[nMCB].head_use.ptsmdata_l = nPulseCounter + INVALID_DATA_PACKET_NUM;
									//
									m_MCBDataHeadTmp[nMCB].head_use.ain1.ul = 0;				// ain1 data(UINT)
									m_MCBDataHeadTmp[nMCB].head_use.sscnt = Update_pkg_n[nMCB];	//nPulseCounter + INVALID_DATA_PACKET_NUM;		// socket packet serial number, from packet
									m_MCBDataHeadTmp[nMCB].head_use.regError_DI = nError_DI;

									continue;
								}
							}
							//
							pDBDataPackage.ADFormat_Bit = m_nADBit;
							pDBDataPackage.nSocketCounter = parsed_pkg_n[nMCB] - INVALID_DATA_PACKET_NUM + m_nDiscardNum[nMCB];//软件包裹计数,1开始;
							//软件包裹计数,1开始;
							if(pDBDataPackage.nSocketCounter < 1)
								pDBDataPackage.nSocketCounter = 1;
							///////////////////////////////////////////////////////////////////////////
							pDBDataPackage.overall_err = overall_err;
							////////////////////////////////////////////////////////////////////////////////////////////////////
							//只有当DB存在且第一个DB有DAQ数据包，才往应用程序发送数据更新消息
							if((pDBDataPackage.Max_DB_board > 0) && (DB_DAQ_PKG == pDBDataPackage.DB_pkg_type[0]))
							{
								////////////////////////////////////////////////////////////////////////////////////////////////////
								memset(tmp,0,512);
								sprintf(tmp,"HMCB(4)%d DB%d Update One Packet Data (No. %d) To App.\n", nMCB, SC_pkg_db_line, Update_pkg_n[nMCB]);
								sz_szInfo = tmp;
								//WriteLogFile(sz_szInfo);
								WriteLogFile(sz_szInfo, gLogLevel2);
								////////////////////////////////////////////////////////////////////////////////////////////////////
								//2025.09.17 for Debug
								if( 1 == m_bDebug)
								{
									//打印前5次采集的前几个通道数据
									if(Update_pkg_n[nMCB] < 5)
									{
										////////////////////////////////////////////////////////////////////////////////////////////////////
										memset(tmp,0,512);
										sprintf(tmp,"HMCB(4)%d DB%d- The first 5 bytes data of the %d Packet are: 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X.\n", nMCB, SC_pkg_db_line, Update_pkg_n[nMCB], pDBDataPackage.DB_data[0][0], pDBDataPackage.DB_data[0][1], pDBDataPackage.DB_data[0][2], pDBDataPackage.DB_data[0][3], pDBDataPackage.DB_data[0][4]);
										sz_szInfo = tmp;
										//WriteLogFile(sz_szInfo);
										WriteLogFile(sz_szInfo);
										////////////////////////////////////////////////////////////////////////////////////////////////////
									}
								}
								////////////////////////////////////////////////////////////////////////////////////////////////////
								//2024.05.14 前面几次采集，主控制盒的钩铛信号强制置0
								if((nMCB == 0) && (Update_pkg_n[nMCB]< 2))
								{
									//pDBDataPackage.CB_data[2] = pDBDataPackage.CB_data[2] & 0xBF;//CB_data[2]：高4位用于传输DDI（高位-低位依次为DI3,DI2DI1,DI0，对应备用-钩铛-高低能-触发
									//DI是反逻辑，置0需要写1
									pDBDataPackage.CB_data[2] = pDBDataPackage.CB_data[2] | 0x40;//CB_data[2]：高4位用于传输DDI（高位-低位依次为DI3,DI2DI1,DI0，对应备用-钩铛-高低能-触发
								}
								////////////////////////////////////////////////////////////////////////////////////////////////////
								UpdateOnePacketDataToApp_HMCB4(&pDBDataPackage);
								//一个CB板的最后一个DB数据更新后，数据包裹数才增1
								if( nMaxDBLindIndex == pDBDataPackage.DB_line_i)
								{
									Update_pkg_n[nMCB]++;
								}
								////////////////////////////////////////////////////////////////////////////////////////////////////
								//正确解析一包数据后，m_nMissAA55Cnt计数清零
								m_nMissAA55Cnt[nMCB] = 0;
								//数据采集是否成功完成标志
								m_bPreDAQSucessFlag[nMCB] = true;
							}
						}
						//??????????????m_bAdjAinDi??1??????adjust AIN DI...
						else
						{
							////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
							//2024-01-08????????????????????????????????????????????????????????????
							////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
							UINT nError_DI = ((~pDBDataPackage.CB_data[2])& 0xF0)<<4;//????????;//Data 2????4??????????DDI
							if(m_bGoudang_S4)
							{
								nError_DI = (nError_DI & 0x0B00) + ((nError_DI&0x0200)<< 1);//????????????????????????
							}
							//??????????????????????
							long nPulseCounter = 0;
							if(m_bHEorCXMode)
							{
								//????
								nPulseCounter = (pDBDataPackage.CB_data[8]<<24)+(pDBDataPackage.CB_data[9]<<16)+(pDBDataPackage.CB_data[10]<<8)+(pDBDataPackage.CB_data[11]);
								nPulseCounter = nPulseCounter - SC_1st_pkg_TriggCnt[nMCB] - INVALID_DATA_PACKET_NUM + m_nDiscardNum[nMCB] + 1;
							}
							else
							{
								//????
								nPulseCounter = pDBDataPackage.SC_pkg_index;
								nPulseCounter = nPulseCounter - INVALID_DATA_PACKET_NUM;
							}
							//
							m_MCBDataHeadTmp[nMCB].head_use.scounter = pDBDataPackage.nSocketCounter;//nSocketCounter;	// socket packet serial number, self-add//????????????,1????
							m_MCBDataHeadTmp[nMCB].head_use.pcounter.l = nPulseCounter;//nPulseCounter;	// pulse trigger number//??????????????????????
							//m_MCBDataHeadTmp[nMCB].head_use.accEnergy = ((nError_DI&0x0200) ==0) ? 1 : 0;// acc energy sign
							//2024.11.04根据调试反馈，高能时给软件能量指示信号为0，正常应该为1，故改为下面逻辑
							m_MCBDataHeadTmp[nMCB].head_use.accEnergy = ((nError_DI&0x0200) ==0) ? 0 : 1;// acc energy sign
							///////////////////////////////////////////////////////////////////
							UINT nDI3 = ((nError_DI&0x0800) == 0) ? 0 : 1;// acc energy sign
							UINT nDI1 = ((nError_DI&0x0200) == 0) ? 0 : 1;// acc energy sign
							//适配三能加速器（DI(3)为高，则为0.7MeV，否则为高低能）
							if(m_bThreeEnergyAcc_En)
							{
								if((0 == nDI3) && (0 == nDI1)) //00超低
								{
									m_MCBDataHeadTmp[nMCB].head_use.accEnergy = 2;
								}
								else if((0 == nDI3) && (1 == nDI1)) //01超低
								{
									m_MCBDataHeadTmp[nMCB].head_use.accEnergy = 2;
								}
								else if((1 == nDI3) && (0 == nDI1))//10低能
								{
									m_MCBDataHeadTmp[nMCB].head_use.accEnergy = 0;
								}
								else//11高能
								{
									m_MCBDataHeadTmp[nMCB].head_use.accEnergy = 1;
								}
							}
							////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
							//CB??????????????12????????????,12????????
							//Data 0??DB line??????????4??????SDCLK????????????????4??????SDCLK??????????
							//Data 1??DB line??????????????????????
							//Data 2????4????????????DDI????4????????????0????????????
							//Data 3????????????????
							//Data 4??11: ??????????????
							//m_MCBDataHeadTmp[nMCB].d_head.head_use.ain0.ul =		(pDBDataPackage.CB_data[4]<<24)+(pDBDataPackage.CB_data[5]<<16)+(pDBDataPackage.CB_data[6]<<8)+(pDBDataPackage.CB_data[7]);// 24bit ADC??(????24bit + 8Bit (0x07))
							m_MCBDataHeadTmp[nMCB].head_use.ain0.ul =		(pDBDataPackage.CB_data[4]<<16)+(pDBDataPackage.CB_data[5]<<8)+(pDBDataPackage.CB_data[6]);//24bit ADC??(????24bit + 8Bit (0x07))
							m_MCBDataHeadTmp[nMCB].head_use.ptsmdata_h =	(pDBDataPackage.CB_data[0]<<24)+(pDBDataPackage.CB_data[1]<<16)+(pDBDataPackage.CB_data[2]<<8)+(pDBDataPackage.CB_data[3]);// ????0??3??????????
							//m_MCBDataHeadTmp[nMCB].d_head.head_use.ptsmdata_l =	(pDBDataPackage.CB_data[8]<<24)+(pDBDataPackage.CB_data[9]<<16)+(pDBDataPackage.CB_data[10]<<8)+(pDBDataPackage.CB_data[11]);// CountPos(32bit)
							m_MCBDataHeadTmp[nMCB].head_use.ptsmdata_l = nPulseCounter + INVALID_DATA_PACKET_NUM;
							//
							m_MCBDataHeadTmp[nMCB].head_use.ain1.ul = 0;				// ain1 data(UINT)
							m_MCBDataHeadTmp[nMCB].head_use.sscnt = Update_pkg_n[nMCB];	//nPulseCounter + INVALID_DATA_PACKET_NUM;		// socket packet serial number, from packet
							m_MCBDataHeadTmp[nMCB].head_use.regError_DI = nError_DI;
						}
					}

				}
				//std::this_thread::sleep_for(std::chrono::nanoseconds(300));
			}
			//ReleaseMutex(hMutex);
			//lock.unlock();
		}
	//////////////////////////////////////////////////////////////////
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
	////////////////////////////////////////////////////////////////////////////////////////////////////
	//memset(tmp,0,512);
	//sprintf(tmp,"Exit HMCB(4)%d ETH_read_and_parse_dataEx_Thread. \n", nMCB);
	//sz_szInfo = tmp;
	//WriteLogFile(sz_szInfo);

	return 0;
}

//////////////////////////////////////////////////////////////////
void CMCBCtrlC::UpdateOnePacketDataToApp_HMCB4(OneDBLineOnePackage *pDBDataPackage)
{
	std::string sz_szInfo = ("");

    char tmp[512] = {0};
	INT16U i = 0, j = 0, k = 0, l = 0;
	///////////////////////////////////////////////
	unsigned short nMCB = pDBDataPackage->nMCB;
	nMCB = (nMCB < (MAX_MCBNUM - 1)) ? nMCB : (MAX_MCBNUM - 1);
	//
	unsigned short nDBLine = pDBDataPackage->DB_line_i;
	nDBLine = (nDBLine < (MAX_DBLINENUM - 1)) ? nDBLine : (MAX_DBLINENUM - 1);
	/////////////////////////////////////////////////////////////////////
    UINT nDBDMNum[MAX_DBLINENUM] = { 0 };
    //如果配置模块调整，则按HMCB控制盒默认顺序
    bool bModuleChAdj = false;
    UINT nDBDMNumIndex = 0;
    for(i = 0; i < MAX_CHNADJNUM; i++)
    {
        if(3 == m_nMCBxDByAdjChn[nMCB][nDBLine][i]) //[CHADJ] MCBxDBySTEP1 = MODULE
        {
            bModuleChAdj = true;
            //
            nDBDMNum[0] = 0;
            nDBDMNum[1] = m_DMInfo[nMCB].nDBDMNUM[0];
            nDBDMNum[2] = m_DMInfo[nMCB].nDBDMNUM[0] + m_DMInfo[nMCB].nDBDMNUM[1];
            nDBDMNum[3] = m_DMInfo[nMCB].nDBDMNUM[0] + m_DMInfo[nMCB].nDBDMNUM[1] + m_DMInfo[nMCB].nDBDMNUM[2];
            //nDBDMNum[0] = m_DMInfo[nMCB].nDBDMNUM[0] - 1;
            //nDBDMNum[1] = m_DMInfo[nMCB].nDBDMNUM[0] + m_DMInfo[nMCB].nDBDMNUM[1] - 1;
            //nDBDMNum[2] = m_DMInfo[nMCB].nDBDMNUM[0] + m_DMInfo[nMCB].nDBDMNUM[1] + m_DMInfo[nMCB].nDBDMNUM[2] - 1;
            //nDBDMNum[3] = m_DMInfo[nMCB].nDBDMNUM[0] + m_DMInfo[nMCB].nDBDMNUM[1] + m_DMInfo[nMCB].nDBDMNUM[2] + m_DMInfo[nMCB].nDBDMNUM[3] - 1;
            break;
        }
        else
        {
            bModuleChAdj = false;
            //
            //nDBDMNum[0] = 0;
            //nDBDMNum[1] = m_DMInfo[nMCB].nDBDMNUM[0];
            //nDBDMNum[2] = m_DMInfo[nMCB].nDBDMNUM[0] + m_DMInfo[nMCB].nDBDMNUM[1];
            //nDBDMNum[3] = m_DMInfo[nMCB].nDBDMNUM[0] + m_DMInfo[nMCB].nDBDMNUM[1] + m_DMInfo[nMCB].nDBDMNUM[2];
            nDBDMNum[0] = m_DMInfo[nMCB].nDBDMNUM[0] - 1;
            nDBDMNum[1] = m_DMInfo[nMCB].nDBDMNUM[0] + m_DMInfo[nMCB].nDBDMNUM[1] - 1;
            nDBDMNum[2] = m_DMInfo[nMCB].nDBDMNUM[0] + m_DMInfo[nMCB].nDBDMNUM[1] + m_DMInfo[nMCB].nDBDMNUM[2] - 1;
            nDBDMNum[3] = m_DMInfo[nMCB].nDBDMNUM[0] + m_DMInfo[nMCB].nDBDMNUM[1] + m_DMInfo[nMCB].nDBDMNUM[2] + m_DMInfo[nMCB].nDBDMNUM[3] - 1;
        }
    }
	//
	UINT nDBLineEnNum = m_DMInfo[nMCB].nDBLineEnNum;
	///////////////////////////////////////////////
	unsigned short nMaxDBBoardi = pDBDataPackage->Max_DB_board;//DB数量(1个起)
	nMaxDBBoardi = (nMaxDBBoardi <= MAX_MCBADNUM) ? nMaxDBBoardi : MAX_MCBADNUM;
	//上传的数据低字节在前
	INT8U ADFormat_Bit = pDBDataPackage->ADFormat_Bit;
	//
	unsigned long nADMChData1 = 0;
	unsigned long nADMChData2 = 0;
	unsigned char nTempCh[6] = { 0 };
	//数据调整
	//UINT nDMNum = m_DMInfo[nMCB].nDMNum;     //Detector Module number of the MCB
	UINT nRowNum = m_DMInfo[nMCB].nRowNum;    //Row of each Detector Module
	UINT nColNum = m_DMInfo[nMCB].nColNum;    //Column of each Detector Module
	//
	nRowNum = (nRowNum < MAX_DMROWNUM) ? nRowNum : MAX_DMROWNUM;
	nColNum = (nColNum < MAX_DMCOLNUM) ? nColNum : MAX_DMCOLNUM;
	//
	if(16 == ADFormat_Bit)
	{
		for(j = 0; j < nMaxDBBoardi; j++)
        {
            //////////////////////////////////////////////////////////////////////
            //是否调整模块通道顺序
            if(bModuleChAdj)
            {
                nDBDMNumIndex = nDBDMNum[nDBLine] + j;
            }
            else
            {
                nDBDMNumIndex = nDBDMNum[nDBLine] - j;
            }
            //////////////////////////////////////////////////////////////////////
			for(i = 0; i < pDBDataPackage->DB_data_len[j] / 2; i++)
            {
                //??????????2??????????????????16bit????4bit??DB????????4bit??Board????????8??????????????
                if( 2 == m_bDebug)
                {
                    nADMChData1 = (nDBLine<<12) + (j<<8) + i;
                }
                else//????????
                {
                    //2个字节数据表示一个通道数据，且低字节在前，高字节在后
                    nADMChData1 = pDBDataPackage->DB_data[j][i*2] + (pDBDataPackage->DB_data[j][i*2+1]<<8);
                }
				//////////////////////////////////////////////////////////////////////
				//数据解析按智能（=0）还是高能（=1）模式解析处理
				if(m_bHEorCXMode)
				{
                    //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] + j][i/nRowNum][i%nRowNum] = nADMChData1;//一次处理一个DBline带的一个模块的数据
                    //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] - j][i/nRowNum][i%nRowNum] = nADMChData1;//一次处理一个DBline带的一个模块的数据
                    pnDBlineDetectorData[nMCB][nDBDMNumIndex][i/nRowNum][i%nRowNum] = nADMChData1;//一次处理一个DBline带的一个模块的数据
                    //////////////////////////////////////////////////////////////////
					if((0 == i) && (j < DDataNUM))
					{
                        m_uDBParameter[nMCB][nDBDMNum[nDBLine] + j] = nADMChData1;//每个控制盒模块的第一个通道数据
					}
				}
				else
				{
					//////////////////////////////////////////////////////////////////
					//for 智能
					if((0 == i) && (j < DDataNUM))
					{
						m_uDBParameter[nMCB][j] = nADMChData1;//每个模块的第一个通道数据
					}
					//第二个数为触发编号，往后为通道数据
					else if(1 == i)
					{
						//nTriggerCnt = nADMChData1 + 1;//硬件返回的触发编号,硬件从0开始计数，软件从1开始计数，补差值1
					}
					else//(i>=2)
					{
						////long pnDBlineDetectorData[MAX_MCBNUM][MAX_MCBADNUM][MAX_DMCOLNUM][MAX_DMROWNUM];
                        //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] + j][(i-2)/nRowNum][(i-2)%nRowNum] = nADMChData1;//一次处理一个DBline带的一个模块的数据
                        //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] - j][(i-2)/nRowNum][(i-2)%nRowNum] = nADMChData1;//一次处理一个DBline带的一个模块的数据
                        pnDBlineDetectorData[nMCB][nDBDMNumIndex][(i-2)/nRowNum][(i-2)%nRowNum] = nADMChData1;//一次处理一个DBline带的一个模块的数据
					}
				}
			}
		}
	}
	if(20 == ADFormat_Bit)
	{
		for(j = 0; j < nMaxDBBoardi; j++)
        {
            //////////////////////////////////////////////////////////////////////
            //是否调整模块通道顺序
            if(bModuleChAdj)
            {
                nDBDMNumIndex = nDBDMNum[nDBLine] + j;
            }
            else
            {
                nDBDMNumIndex = nDBDMNum[nDBLine] - j;
            }
            //////////////////////////////////////////////////////////////////////
			for(i = 0; i < pDBDataPackage->DB_data_len[j] / 5; i++)
            {
                //??????????2??????????????????16bit????4bit??DB????????4bit??Board????????8??????????????
                if( 2 == m_bDebug)
                {
                    nADMChData1 = (nDBLine<<12) + (j<<8) + i*2;
                    nADMChData2 = (nDBLine<<12) + (j<<8) + i*2+1;
                }
                else//????????
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
                }
				//////////////////////////////////////////////////////////////////////
				//数据解析按智能（=0）还是高能（=1）模式解析处理
				if(m_bHEorCXMode)
				{
					//高能
                    //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] + j][i*2/nRowNum][i*2%nRowNum] = nADMChData1;//一次处理一个DBline带的一个模块的数据
                    //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] + j][(i*2+1)/nRowNum][(i*2+1)%nRowNum] = nADMChData2;//一次处理一个DBline带的一个模块的数据
                    //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] - j][i*2/nRowNum][i*2%nRowNum] = nADMChData1;//一次处理一个DBline带的一个模块的数据
                    //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] - j][(i*2+1)/nRowNum][(i*2+1)%nRowNum] = nADMChData2;//一次处理一个DBline带的一个模块的数据
                    pnDBlineDetectorData[nMCB][nDBDMNumIndex][i*2/nRowNum][i*2%nRowNum] = nADMChData1;//一次处理一个DBline带的一个模块的数据
                    pnDBlineDetectorData[nMCB][nDBDMNumIndex][(i*2+1)/nRowNum][(i*2+1)%nRowNum] = nADMChData2;//一次处理一个DBline带的一个模块的数据
                    ////////////////////////////////////////////////////////////////
					if((0 == i) && (j < DDataNUM))
					{
						m_uDBParameter[nMCB][nDBDMNum[nDBLine] + j] = nADMChData1;
					}
				}
				else
				{
					////////////////////////////////////////////////////////////////
					//智能
					//第一个数为配置参数
					//第二个数为触发编号，往后为通道数据
					if(0 == i)
					{
						if(j < DDataNUM)
						{
							m_uDBParameter[nMCB][j] = nADMChData1;
						}
						//nTriggerCnt = nADMChData2 + 1;//硬件返回的触发编号,硬件从0开始计数，软件从1开始计数，补差值1
					}
					else//(i>=1)
					{
                        //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] + j][(i-1)*2/nRowNum][(i-1)*2%nRowNum] = nADMChData1;//一次处理一个DBline带的一个模块的数据
                        //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] + j][((i-1)*2+1)/nRowNum][((i-1)*2+1)%nRowNum] = nADMChData2;//一次处理一个DBline带的一个模块的数据
                        //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] - j][(i-1)*2/nRowNum][(i-1)*2%nRowNum] = nADMChData1;//一次处理一个DBline带的一个模块的数据
                        //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] - j][((i-1)*2+1)/nRowNum][((i-1)*2+1)%nRowNum] = nADMChData2;//一次处理一个DBline带的一个模块的数据
                        pnDBlineDetectorData[nMCB][nDBDMNumIndex][(i-1)*2/nRowNum][(i-1)*2%nRowNum] = nADMChData1;//一次处理一个DBline带的一个模块的数据
                        pnDBlineDetectorData[nMCB][nDBDMNumIndex][((i-1)*2+1)/nRowNum][((i-1)*2+1)%nRowNum] = nADMChData2;//一次处理一个DBline带的一个模块的数据
					} 
				}
			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////
	//2025.09.17 for Debug
	if( 1 == m_bDebug)
	{
		//打印前5次采集的前几个通道数据
		if(Update_pkg_n[nMCB] < 5)
		{
			////////////////////////////////////////////////////////////////////////////////////////////////////
			memset(tmp,0,512);
			sprintf(tmp,"HMCB(4)%d DB%d- The first 5 channel data of the %d Packet are: 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X.\n", nMCB, nDBLine, Update_pkg_n[nMCB], pnDBlineDetectorData[nMCB][0][0][0], pnDBlineDetectorData[nMCB][0][0][1], pnDBlineDetectorData[nMCB][0][0][2], pnDBlineDetectorData[nMCB][0][0][3], pnDBlineDetectorData[nMCB][0][0][4]);
			sz_szInfo = tmp;
			//WriteLogFile(sz_szInfo);
			WriteLogFile(sz_szInfo);
			////////////////////////////////////////////////////////////////////////////////////////////////////
		}
	}
	////////////////////////////////////////////
	//如果所有dbline已经全部更新
	if(nDBLineEnNum > (nDBLine + 1) )
	{
		return;
	}
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
	//16个CB数据
	for(i = 0; i < 16; i++)
	{
		m_pOneDBLineOnePackageStatue[nMCB].CB_data[i] = pDBDataPackage->CB_data[i];
	}
	m_pOneDBLineOnePackageStatue[nMCB].CB_crc_code = pDBDataPackage->CB_crc_code;
    ///////////////////////////////////////////////

    if(m_timenodata[nMCB] == NucTool::GetTickCount())
    {

    }

    m_timehot[nMCB] = NucTool::GetTickCount();
    m_timenodata[nMCB] = NucTool::GetTickCount();
	/////////////////////////////
	//无触发计时器清零
	//m_nWaitDataTrigger[nMCB] = 0;
	////////////////////////////////////////////////////////////////////////////////////////////////
	//数据调整
	UINT nDMNum = m_DMInfo[nMCB].nDMNum;     //Detector Module number of the MCB
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
	////////////////////////////////////////////////////////////////////////////////////////////////////
	//2025.09.17 for Debug
	if( 1 == m_bDebug)
	{
		//打印前5次采集的前几个通道数据
		if(Update_pkg_n[nMCB] < 5)
		{
			////////////////////////////////////////////////////////////////////////////////////////////////////
			memset(tmp,0,512);
			sprintf(tmp,"HMCB(4)%d DB%d- The first 5 channel data of the %d Packet in m_MCBData[%d] are: 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X.\n", nMCB, nDBLine, Update_pkg_n[nMCB], nMCB, m_MCBData[nMCB].addata[0].ul, m_MCBData[nMCB].addata[1].ul, m_MCBData[nMCB].addata[2].ul,  m_MCBData[nMCB].addata[3].ul, m_MCBData[nMCB].addata[4].ul);
			sz_szInfo = tmp;
			//WriteLogFile(sz_szInfo);
			WriteLogFile(sz_szInfo);
			////////////////////////////////////////////////////////////////////////////////////////////////////
		}
	}
	////////////////////////////////////////////////////////////////////////
	int nIndex = nMCB;
	////////////////////////////////////////////////////////////////////////
	//HMCB默认离控制盒最远的探测器模块为第一个探测器模块，与MCB-H定义相反，默认对所有HMCB或DB-Line按3-MODULE方案调整通道顺序。
//	memcpy(m_MCBDataTmp.addata, m_MCBData[nIndex].addata, m_DMInfo[nIndex].nAllChnNum *sizeof(UINT));
//	for (i =0; i<int(m_DMInfo[nIndex].nAllChnNum); i++)
//	{
//		m_MCBData[nIndex].addata[i].ul = m_MCBDataTmp.addata[i/(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)*m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum
//			+((m_DMInfo[nIndex].nDMNum-1)-(i/m_DMInfo[nIndex].nRowNum)%m_DMInfo[nIndex].nDMNum)*m_DMInfo[nIndex].nRowNum +i%m_DMInfo[nIndex].nRowNum].ul;
//	}
	////////////////////////////////////////////////////////////////////////
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
        nError_DI = (nError_DI & 0x0B00) + ((nError_DI&0x0200)<< 1);//DI2清零后，把DI1的值赋给DI2
    }

	///////////////////////////////////////////////////////////////////
    //控制板返回的硬件触发数
	long nPulseCounter = 0;
	if(m_bHEorCXMode)
	{
		//高能
		nPulseCounter = (pDBDataPackage->CB_data[8]<<24)+(pDBDataPackage->CB_data[9]<<16)+(pDBDataPackage->CB_data[10]<<8)+(pDBDataPackage->CB_data[11]);
		nPulseCounter = nPulseCounter - SC_1st_pkg_TriggCnt[nIndex] - INVALID_DATA_PACKET_NUM + m_nDiscardNum[nMCB] + 1;
	}
	else
	{
		//智能
		nPulseCounter = pDBDataPackage->SC_pkg_index;
		nPulseCounter = nPulseCounter - INVALID_DATA_PACKET_NUM;
	}
	//
	m_MCBData[nMCB].d_head.head_use.scounter = pDBDataPackage->nSocketCounter;//nSocketCounter;	// socket packet serial number, self-add//软件包裹计数,1开始
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
		m_MCBData[nMCB].d_head.head_use.pcounter.l = pDBDataPackage->nSocketCounter;//nPulseCounter;	// pulse trigger number//控制板返回的硬件触发数
	}
	//m_MCBData[nMCB].d_head.head_use.accEnergy = ((nError_DI&0x0200) ==0) ? 1 : 0;// acc energy sign
	/////////////////////////////////////////////////////////////////
	//m_MCBData[nMCB].d_head.head_use.accEnergy = ((nError_DI&0x0200) ==0) ? 1 : 0;// acc energy sign
	//2024.11.04根据调试反馈，高能时给软件能量指示信号为0，正常应该为1，故改为下面逻辑
	m_MCBData[nMCB].d_head.head_use.accEnergy = ((nError_DI&0x0200) ==0) ? 0 : 1;// acc energy sign
	///////////////////////////////////////////////////////////////////
	UINT nDI3 = ((nError_DI&0x0800) == 0) ? 0 : 1;// acc energy sign
	UINT nDI1 = ((nError_DI&0x0200) == 0) ? 0 : 1;// acc energy sign
	//适配三能加速器（DI(3)为高，则为0.7MeV，否则为高低能）
	if(m_bThreeEnergyAcc_En)
	{
		if((0 == nDI3) && (0 == nDI1)) //00超低
		{
			m_MCBData[nMCB].d_head.head_use.accEnergy = 2;
		}
		else if((0 == nDI3) && (1 == nDI1)) //01超低
		{
			m_MCBData[nMCB].d_head.head_use.accEnergy = 2;
		}
		else if((1 == nDI3) && (0 == nDI1))//10低能
		{
			m_MCBData[nMCB].d_head.head_use.accEnergy = 0;
		}
		else//11高能
		{
			m_MCBData[nMCB].d_head.head_use.accEnergy = 1;
		}
	}
	///////////////////////////////////////////////////////////////////
	std::string sz_szInfoCBData = ("");
	char tmpCBData[512] ={0};
	//控制板返回的硬件触发数
	if( m_nLogLevel >= gLogLevel2 )
	{
		memset(tmpCBData,0,512);
		sprintf(tmpCBData,"HMCB(4)_%d CB[0]~CB[11]:\t", nMCB);
		sz_szInfoCBData += tmpCBData;
		//
		for(i = 0; i < 12; i++)
		{
			sprintf(tmpCBData,"%02X,\t", pDBDataPackage->CB_data[i]);
			sz_szInfoCBData += tmpCBData;
		}
		memset(tmpCBData,0,512);
		sprintf(tmpCBData,"[DEBUG]\n");
		sz_szInfoCBData += tmpCBData;
		//
		//std::cout <<(sz_szInfoCBData);
		WriteLogFile(sz_szInfoCBData);
		//
		memset(tmpCBData,0,512);
		sprintf(tmpCBData,"HMCB(4)_%d nError_DI = %08X, DI3=%d, DI1=%d, bThreeEnergyAcc=%d,	head_use.accEnergy=%d.[DEBUG]\n", nMCB, nError_DI, nDI3, nDI1, m_bThreeEnergyAcc_En, m_MCBData[nMCB].d_head.head_use.accEnergy);
		sz_szInfoCBData = tmpCBData;
		WriteLogFile(sz_szInfoCBData);
	}
	///////////////////////////////////////////////////////////////////
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
		{  // copy last accEnergy, AIN, DI...:
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
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if(m_bZeroPadding)//????????????????????????
    {
        long nIncPulseCnt = 0;
        if(0 == m_lPrePulseCnt[nMCB])
        {
            m_lPrePulseCnt[nMCB] = m_MCBData[nMCB].d_head.head_use.pcounter.l;//??????????????????????????????????????
        }
        else
        {
            nIncPulseCnt = m_MCBData[nMCB].d_head.head_use.pcounter.l - m_lPrePulseCnt[nMCB] - 1;
            if(nIncPulseCnt > 0)//??????????????
            {
                for(i = 0; i < nIncPulseCnt; i++)
                {
                    //set m_MCBDataTmp as 0????????
                    //memset(&m_MCBDataTmp, 0, sizeof(MCBData));
                    //拷贝前次数据备份数据
                    memcpy(&m_MCBDataTmp, &m_MCBDataBackup[nMCB], sizeof(MCBData));//拷贝前次数据备份
                    //copy current packet head to m_MCBDataTmp????????????????
                    memcpy(&m_MCBDataTmp.d_head, &m_MCBData[nMCB].d_head, sizeof(MCBData_H));   //temporarily save current MCBData_H
                    ////////////////////////////////////////
                    //??????????????????????????
                    m_MCBDataTmp.d_head.head_use.scounter = m_MCBDataTmp.d_head.head_use.scounter + i;//nSocketCounter;	// socket packet serial number, self-add//????????????,1????
                    m_MCBDataTmp.d_head.head_use.pcounter.l = m_lPrePulseCnt[nMCB] + 1 + i;//nPulseCounter;	// pulse trigger number//??????????????????????
                    m_MCBDataTmp.d_head.head_use.accEnergy = -1;//??????????????????
                    ////////////////////////////////////////
                    parsed_pkg_n[nMCB]++;//????????????????????????????1
                    //????????????????
					////////////////////////////////////////
					//2026.03.19增加对CS0200M的支持
					//1——CS0200M， 调整主控制盒的m_MCBData[nIndex].d_head.head_use.accEnergy为出束光机的编号( m_MCBData[nIndex].d_head.head_use.accEnergy = (long)l_angle);
					//出束光机的编号规则：光机触发信号接DI-0，光机循环信号接DI-1，DI-1为1时的光机编号为0，DI-1为0后每一次触发的光机编号+1
					//射线源序号
					if( 0 == nMCB)
					{
						if(1 == m_nDeviceTypeID)
						{	
							if(1 == m_MCBDataTmp.d_head.head_use.accEnergy)
							{
								m_nXRayGenN = 0;
							}
							else
							{
								m_nXRayGenN++;
							}
							m_MCBDataTmp.d_head.head_use.accEnergy = (LONG)m_nXRayGenN;
						}
					}
					////////////////////////////////////////
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
                        sz_szInfo = tmp;

                        WriteLogFile(sz_szInfo);
                        //????X????????????????
                    }
                }
                //??????????????????????????????
                m_MCBData[nMCB].d_head.head_use.scounter = pDBDataPackage->nSocketCounter + nIncPulseCnt;//nSocketCounter;	// socket packet serial number, self-add//????????????,1????
                //m_MCBData[nMCB].d_head.head_use.pcounter.l = m_lPrePulseCnt + nIncPulseCnt + 1;//nPulseCounter;	// pulse trigger number//??????????????????????
            }
        }
    }
	////////////////////////////////////////
	//2026.03.19增加对CS0200M的支持
	//1——CS0200M， 调整主控制盒的m_MCBData[nIndex].d_head.head_use.accEnergy为出束光机的编号( m_MCBData[nIndex].d_head.head_use.accEnergy = (long)l_angle);
	//出束光机的编号规则：光机触发信号接DI-0，光机循环信号接DI-1，DI-1为1时的光机编号为0，DI-1为0后每一次触发的光机编号+1
	//射线源序号
	if(0 == nMCB)
	{
		if(1 == m_nDeviceTypeID)
		{	
			if(1 == m_MCBData[nMCB].d_head.head_use.accEnergy)
			{
				m_nXRayGenN = 0;
			}
			else
			{
				m_nXRayGenN++;
			}
			m_MCBData[nMCB].d_head.head_use.accEnergy = (LONG)m_nXRayGenN;
		}
	}
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//记录发送给软件的数据包包头信息
	if( m_nLogLevel >= gLogLevel1 )
	{
		memset(tmp,0,512);
		sprintf(tmp,"HMCB%d gCallBack(MCB_MSG_DATA)- the header of the data packet passed to the software : scounter=%d, pcounter=%d, accEnergy=%d, ain0=%d, sscnt=%d, regError_DI=0x%08X.\n",
			nMCB, m_MCBData[nMCB].d_head.head_use.scounter, m_MCBData[nMCB].d_head.head_use.pcounter.l,  m_MCBData[nMCB].d_head.head_use.accEnergy, m_MCBData[nMCB].d_head.head_use.ain0, m_MCBData[nMCB].d_head.head_use.sscnt, m_MCBData[nMCB].d_head.head_use.regError_DI);
		sz_szInfo = tmp;
		std::cout << sz_szInfo;
		WriteLogFile(sz_szInfo);
	}
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    gCallBack(MCB_MSG_DATA, WPARAM(&m_MCBData[nMCB]), nMCB);
    //
    memcpy(&m_MCBDataBackup[nMCB], &m_MCBData[nMCB], sizeof(MCBData));//数据备份
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    m_lPrePulseCnt[nMCB] = m_MCBData[nMCB].d_head.head_use.pcounter.l;//????????????????????
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if(1 == ((m_MCBData[nMCB].d_head.head_use.scounter) % 100))
	{
		memset(tmp,0,512);
        sprintf(tmp,"HMCB%d software socket data counts: %d, hardware pluse counts: %d, differ:%d\n",
							nMCB, m_MCBData[nMCB].d_head.head_use.scounter, m_MCBData[nMCB].d_head.head_use.pcounter.l,
								  m_MCBData[nMCB].d_head.head_use.pcounter.l - m_MCBData[nMCB].d_head.head_use.scounter);
		sz_szInfo = tmp;
		std::cout << sz_szInfo;
		WriteLogFile(sz_szInfo);
		//执行X命令读取寄存器值
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //gCallBack(MCB_MSG_DATA, WPARAM(&m_MCBData[nMCB]), nMCB);
}

////////////////////////////////////////////////////
//void CMCBCtrlC::CheckDVReadySts_HMCB4()
//{
//    std::string str = ("");
//    std::string  sz_szInfo = ("");
//    ////////////////////////////////////////////////////
//    sz_szInfo = "Enter CheckDVReadySts_HMCB4 process.\n";
//    WriteLogFile(sz_szInfo, gLogLevel6);
//    //
//    bool bIni = m_bAllMCBInInitialing;
//    bIni = bIni || m_bxMCBInInitialing[0];
//    //
//    if(bIni)
//    {
//        sz_szInfo = "In the CheckDVReadySts_HMCB4 process, but HMCB is in initialization........\n";
//        WriteLogFile(sz_szInfo, gLogLevel2);
//        return;
//    }
//    /////////////////////////////////////
//    bool b,b1[MAX_MCBNUM];
//    //
//    b =  true;
//    b1[0] = true;
//    /////////////////////////////////////
//    b1[0] =  ((m_MCBReadySts[0]&0x0f)==0x0f);//设备就绪状态
//    b = b && ((m_MCBReadySts[0]&0x0f)==0x0f);//设备就绪状态
//    /////////////////////////////////////
//    if (!b1[0])
//    {
//        char tmp[512] = {0};
//        sprintf(tmp,"HMCB0-DB%d not ready!!! Repeat Initial it again.\n", 0);
//        sz_szInfo = tmp;
//        WriteLogFile(sz_szInfo);
//        //
//        InitialMCB_HMCB4();
//    }
//    /////////////////////////////////////
//    //str.Format(_T("m_MCBReadySts[1-2]= 0x%02X 0x%02X........\n"), m_MCBReadySts[0], m_MCBReadySts[1]);
//    //WriteLogFile(str);
//    /////////////////////////////////////
//    if (b) // DEVICE READY
//    {
//        /////////////////////////////////////
//        if (!m_bDVReady)
//        {
//            /////////////////////////////////////////////////////////////////////////////
//            sz_szInfo = ("HMCB-4 ready!!! Core Firmware Version--");
//            //
//            char tmp[512] = {0};
//            sprintf(tmp,"%s,\t", m_strMCBCoreFirmwareVer[0].c_str());

//            sz_szInfo += tmp;
//            sz_szInfo += "\n";
//            std::cout << sz_szInfo;
//            WriteLogFile(sz_szInfo);
//            //
//            SendMCBMsg(MCB_MSG_STATUS, MCB_DVREADY, (LPARAM)m_nMCB_Version[0]);
//            //SendMCBMsg(MCB_MSG_STATUS, MCB_DVREADY, 0);
//            m_bDVReady = true;
//        }
//    }
//    else
//    {
//        m_bDVReady = false;
//    }
//}

//void CMCBCtrlC::CheckRunStatus_HMCB4()
//{
//    std::string sz_szInfo = ("");
//    unsigned short i = 0;
//    bool b = false;
//    unsigned short nMCB = 0;
//    INT8U ret = 0;
//    //
//    sz_szInfo = "Enter CheckRunStatus_HMCB4 process.\n";
//    WriteLogFile(sz_szInfo, gLogLevel6);
//    //如果只有一个控制盒，start已直接启动采集
//    if ((m_MCBRunSts[0]&0x01) == 1)
//    {
//        //TRACE(_T("Master HMCB start OK, begin running...\n"));
//        //WriteLogFile(_T("Master HMCB start OK, begin running...\n"));
//        //不需CheckRunStatus
//        m_bPreStart = false;
//        //SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, MCB_STSCD_STARTOK);
//    }
//    else if ((NucTool::GetTickCount()-m_timeprestart) >10000)
//    {
//        std::cout <<  "Master HMCB start error, over time!\n";
//        WriteLogFile(("Master HMCB start error, over time!\n"));
//        m_bStart = false;
//        SendMCBMsg(MCB_MSG_STATUS, MCB_STARTERR, 0);
//        //
//        m_timeprestart = NucTool::GetTickCount();
//    }
//}
////////////////////////////////////////////////////////////////////////////////////
//void CMCBCtrlC::CheckStopSts_HMCB4()
//{
//    ////////////////////////////////////////////
//    //如果只有一个控制盒，则直接停止主控制盒采集
//    std::string sz_szInfo = ("");
//    std::string str = ("");
//    char tmp[512] = {0};
//    unsigned short i = 0;
//    //
//    bool b = true;
//    //
//    sz_szInfo = "Enter CheckStopSts_HMCB4 process.\n";
//    WriteLogFile(sz_szInfo, gLogLevel6);
//    ///////////////////////////////////////////////////////
//    INT8U ret = 0;
//    unsigned short nMCB = 0;
//    //主控制盒状态
//    b = ((m_MCBRunSts[nMCB]&0x04)== 0x04);
//    ///////////////////////////////////////////////////////
//    if(!b)
//    {
//        sz_szInfo="Master HMCB not end running and end receive data.\n";
//        std::cout << (sz_szInfo);
//        WriteLogFile(sz_szInfo);
//        //重发停止采集??
//    }
//    //
//    b = b && ((m_MCBRunSts[0]&0x04)== 0x04);//设备就绪状态
//    if(b)
//    {
//        //m_bPreStop = FALSE;
//        m_bStart = false;
//    }
//    /////////////////////////////////////////////////////////
//    if ((NucTool::GetTickCount() - m_timeprestop) > 30000)
//    {
//        std::cout << "HMCB stop error, over time!\n";
//        WriteLogFile(("HMCB stop error, over time!\n"));
//        //
//        m_bPreStop = false;
//        m_bStart = false;
//        //
//        SendMCBMsg(MCB_MSG_STATUS, MCB_STOPERR, 0);
//    }
//}
//////////////////////////////////////////////////////
//void CMCBCtrlC::CheckDataEnd_HMCB4()
//{
//    std::string sz_szInfo = ("");
//    char tmp[512] = {0};
//    sz_szInfo = "Enter CheckDataEnd_HMCB4 process.\n";
//    WriteLogFile(sz_szInfo, gLogLevel6);
//    //
//    int id = 0;
//    unsigned long i = 0;
//    unsigned short j = 0;
//    bool b = true;
//    std::string str = ("");

//    b =true;
//    b = b&&((m_MCBRunSts[0]&0x04)== 0x04);
//    //
//    if (b) // all MCB already end data acquire normally
//    {
//        std::cout << "All HMCB-4s normal ended receiving data successfully!\n";
//        WriteLogFile("All HMCBs normal ended receiving data successfully!\n");
//        m_bStart = false;
//        //
//        //SendMCBMsg(MCB_MSG_STATUS, MCB_STOPOK, 1);
//        //
//        if(!m_bPreDataEnd)
//        {
//            NormalEndDAQ(0);
//        }
//        //
//        m_bPreStop = false;
//        ////////////////////////////////////////////////////////////////////////
//        // record goudang info
//        if (m_scounter[0] >INVALID_DATA_PACKET_NUM)
//        {
//            bool b =false;
//            int dlast = m_godown[0];
//            for (i = 0; i < min(m_scounter[0]-INVALID_DATA_PACKET_NUM-1, (unsigned long)GODOWNNUM); i++)
//            {
//                if (dlast !=m_godown[i])
//                {
//                    if (!b)
//                    {
//                        sz_szInfo =  "GOUDANG INFO:\n";
//                        memset(tmp,0,512);
//                        sprintf(tmp,"                         %d,%d\n", 0, m_godown[0]);
//                        sz_szInfo += tmp;
//                        //
//                        b =true;
//                    }
//                    memset(tmp,0,512);
//                    sprintf(tmp,"                         %d,%d\n", i+1, m_godown[i]);
//                    sz_szInfo += tmp;
//                    dlast =m_godown[i];
//                }
//            }
//            if (b)  //write goudong info
//            {
//                std::cout << sz_szInfo;
//                WriteLogFile(sz_szInfo);
//            }
//        }
//        ////////////////////////////////////////////////////////////////////////
//        if (m_scounter[0] >INVALID_DATA_PACKET_NUM)  //output 1channel det_data
//        {
//            memset(tmp,0,512);
//            sprintf(tmp,"Det_Data INFO(%d):\n", m_nRecvStartCmdCnt);
//            sz_szInfo = tmp;
//            sz_szInfo += "                         SN,DI0,DI1,DI2,DI3,MCB0CH0";
//            for (id=1; id<m_nMCBNum; id++)
//            {
//                memset(tmp,0,512);
//                sprintf(tmp,",MCB%dCH0",id);
//                sz_szInfo += tmp;
//            }
//            sz_szInfo += ("\n");
//            for (i=0; i< min(m_scounter[0]-INVALID_DATA_PACKET_NUM-1, (unsigned long)DDataNUM); i++)
//            {
//                memset(tmp,0,512);
//                sprintf(tmp,"                         %d,%d,%d,%d,%d,%d", i+1, m_InfoExdata[2][i]&0x1, (m_InfoExdata[2][i]>>1)&0x1, (m_InfoExdata[2][i]>>2)&0x1, (m_InfoExdata[2][i]>>3)&0x1, m_uDetData[0][i]);
//                sz_szInfo += tmp;
//                for (j=1; j<m_nMCBNum; j++)
//                {
//                    memset(tmp,0,512);
//                    sprintf(tmp,",%d",m_uDetData[j][i]);
//                    sz_szInfo += tmp;
//                }
//                sz_szInfo +="\n";
//            }
//            //TRACE(sz_szInfo);
//            WriteLogFile(sz_szInfo);
//        }
//        ////////////////////////////////////////////////////////////////////////
//        //打印最后一次CB状态
//        if (m_scounter[0] >(unsigned long)INVALID_DATA_PACKET_NUM)  //output 1channel det_data
//        {

//            sz_szInfo = ("The last packet status:\n");
//            for (id=0; id< m_nMCBx4DBLine; id++)
//            {
//                memset(tmp,0,512);
//                sprintf(tmp,"                         HMCB%d: SC_pkg_index=%d,\t CB_logic_err1=0x%02X, CB_logic_err2=0x%02X\t", id, m_pOneDBLineOnePackageStatue[id].SC_pkg_index, m_pOneDBLineOnePackageStatue[id].CB_logic_err1, m_pOneDBLineOnePackageStatue[id].CB_logic_err2);

//                sz_szInfo += tmp;
//                memset(tmp,0,512);
//                sprintf(tmp,"one DB status: DB_board_index = %d, DB_logic_err1=0x%02X, DB_logic_err2=0x%02X.\n",
//                    m_pOneDBLineOnePackageStatue[id].DB_board_index[0], m_pOneDBLineOnePackageStatue[id].DB_logic_err1[0], m_pOneDBLineOnePackageStatue[id].DB_logic_err2[0]);
//                sz_szInfo += tmp;
//            }
//            //TRACE(sz_szInfo);
//            WriteLogFile(sz_szInfo);
//            WriteLogFile("\n");
//        }
//        ////////////////////////////////////////////////////////////////////////
//        if (m_bSaveExdata)  //save DI, ptsm data...
//        {
//            SaveExdata();
//        }
//        return;
//    }
//    //else if ((time(NULL)-m_timepreend) >15)
//    else if ((NucTool::GetTickCount() - m_timepreend) >15000)
//    {
//        std::cout << "MHCB stop error, over time!\n";
//        WriteLogFile("HMCB stop error, over time!\n");
//        SendMCBMsg(MCB_MSG_STATUS, MCB_STOPERR, 1);
//        Stop(2);
//    }
//}
