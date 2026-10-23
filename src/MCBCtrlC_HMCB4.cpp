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
#include <sys/syscall.h>   // SYS_gettid
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
//ÈÕÖ¾¼ÇÂ¼Ë®Æ½
//²¹³ä¼ÇÂ¼Ò»Ğ©º¯Êı²Ù×÷¼°·µ»ØÖµÈÕÖ¾¼ÇÂ¼
#define gLogLevel1	1
//¼ÇÂ¼Ò»Ğ©Êı¾İ½âÎö²¿·ÖµÄÈÕÖ¾
#define gLogLevel2	2
//Áô´æ´ıÓÃ
#define gLogLevel3	3
//Áô´æ´ıÓÃ
#define gLogLevel4	4
//¼ÇÂ¼socket½ÓÊÕµÄÊı¾İÇÒÁí´æÎªtxtÎÄ¼ş
#define gLogLevel5	5
//´òÓ¡²¿·Ö³ÌĞòµ÷ÓÃÁ÷×ªÈÕÖ¾¼ÇÂ¼
#define gLogLevel6	6
//////////////////////////////////////////////////////////////////////
//#define GLB_DATA_BUF_SIZE	(100000)	//²»ÄÜ³¬¹ı32Î»ÊıµÄ±íÊ¾·¶Î§£¬±ØĞë´óÓÚ1¸öSCÊı¾İ°ü³¤¶È
#define GLB_DATA_BUF_SIZE	(64*1024*1024)	//²»ÄÜ³¬¹ı32Î»ÊıµÄ±íÊ¾·¶Î§£¬±ØĞë´óÓÚ1¸öSCÊı¾İ°ü³¤¶È
//
#define SOCKETBUFSIZE (65536)   // V2.0.1.1 å¤šç›’å¼ºåŒ–ï¼šå•æ¬¡æ¥æ”¶æå¤§ï¼Œå‡å°‘ç³»ç»Ÿè°ƒç”¨/é˜²å†…æ ¸ç¼“å†²æº¢å‡ºåå‹
//extern INT8U glb_data_buf[MAX_MCBNUM][GLB_DATA_BUF_SIZE];
extern std::vector<unsigned char> glb_data_buf_vetor[MAX_MCBNUM];
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
extern HANDLE hMutex;
#else
/*extern*/ //boost::shared_mutex mutex1;
mutex _mutex;
#endif

extern ULONGLONG glb_data_AccWrBytes[MAX_MCBNUM];//¼ÇÂ¼×Ü½ÓÊÕÊı¾İ×Ö½ÚÊı
extern ULONGLONG glb_data_AccRdBytes[MAX_MCBNUM];//¼ÇÂ¼×Ü¶Á³ö´¦Àí×Ö½ÚÊı

extern INT32U gbl_data_buf_wraddr[MAX_MCBNUM];			//¼ÇÂ¼Êı¾İ»º´æÇøÏÂÒ»¸öÊı¾İµÄĞ´ÈëÎ»ÖÃ
extern INT32U gbl_data_buf_rdaddr[MAX_MCBNUM];			//¼ÇÂ¼ÏÂÒ»¸öÊı¾İµÄ¶ÁÈ¡µØÖ·
extern INT32S gbl_sc_pkg_index[MAX_MCBNUM];			//¼ÇÂ¼¸÷¸öDB LineÉÏÒ»¸ösc pkgµÄ°ü±àºÅ

//Ò»¸öDBLineµÄsize= 10×Ö½ÚÂß¼­ + DBÊı*Ò»¸öDB°üsize + 1¸öCB°ü×Ö½Ú
//MAX_MCBNUM¸öMCB£¬MAX_DBLINENUM¸öDBlineÃ¿¸öDBlineĞèÒª½ÓÊÕµÄ×Ö½Ú¸öÊı
extern INT32U gbl_DBLine_Size[MAX_MCBNUM][MAX_DBLINENUM];
//Ò»¸öMCBµÄsize= ¸ÃMCBµÄËùÓĞDBLineµÄsizeÖ®ºÍ
extern INT32U gbl_MCBTotal_Size[MAX_MCBNUM];
//
//INT32U xB_pkg_index_Temp1[MAX_MCBNUM] = { 0 };
//INT32U xB_pkg_index_Temp2[MAX_MCBNUM] = { 0 };
//INT32U xB_pkg_index_Temp3[MAX_MCBNUM] = { 0 };
//--------------------------------------------------------------
//      È«¾ÖÊı¾İ¶ÁÈ¡»º´æÇø
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
    // °ÑÕâÒ»²¿·Ö·ÅÔÚµ¥¸öHMCB³õÊ¼»¯ÀïÃæ
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
    // ½¨Á¢¿ØÖÆºÍÊı¾İÁ´½Ósocket £¬³É¹¦·µ»Øtrue
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
        //³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
         ETH_CTRL_read_buffer_residual_data(nMCB);
        ///////////////////////////////////////////////////////////////
        //»Ö¸´Ä¬ÈÏÉèÖÃ£¬½ûÖ¹ËùÓĞÊı¾İÔ´
        ret = EDAQ_daq_mode_enable(0, nMCB);	//ÍË³öDAQÄ£Ê½
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
        //³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
        ETH_CTRL_read_buffer_residual_data(nMCB);
        ret = EDAQ_TEST_DATA_enable(0, nMCB);	//Ê¹ÄÜ(ÖÃÒ»)»ò½ûÓÃ£¨ÇåÁã£©FPGAÄÚ²¿¼ÆÊıĞÍ²âÊÔÊı¾İ²úÉúÄ£¿é
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
        //³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
        ETH_CTRL_read_buffer_residual_data(nMCB);
        ret = EDAQ_SC_data_pkg_monitor_enable(0, nMCB);	//Ê¹ÄÜ(ÖÃÒ»)»ò½ûÓÃ£¨ÇåÁã£©FPGAÄÚÇ¶µÄSCÊı¾İ°ü¼ì²éÄ£¿é
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
        //³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
         ETH_CTRL_read_buffer_residual_data(nMCB);
        ///////////////////////////////////////////////////////////////
        //Âß¼­¸´Î»FPGAÂß¼­
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
        //Í¨ĞÅ´íÎóÅĞ¶Ï
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
        //³ÖĞø´ÓETH(_ETH_Data_socket)¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
        ETH_CTRL_read_buffer_residual_data(nMCB);
        ///////////////////////////////////////////////////////////////
        //¹¦ÄÜ£ºÉèÖÃMCB¹¤×÷Ä£Ê½
        //²ÎÊı:
        // 1: Work as Master HMCB
        // 0: Work as Cascaded HMCB
        if(0 == nMCB)
        {
            //ÉèÖÃHMCB¼¶Áª¹¤×÷Ä£¿é
            ret = EDAQ_HMCB_work_mode_set(1, nMCB);	//1£ºÖ÷Ä£Ê½£»0£º´ÓÄ£Ê½
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
            //ÉèÖÃHMCB(4)¼¶Áª¹¤×÷Ä£¿é
            ret = EDAQ_HMCB_work_mode_set(0, nMCB);	//1£ºÖ÷Ä£Ê½£»0£º´ÓÄ£Ê½
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
        // ³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
	ETH_CTRL_read_buffer_residual_data(nMCB);
	INT8U DB_line_num = m_DMInfo[nMCB].nDBLineEnNum;
        //¹¦ÄÜ£ºÉèÖÃÊ¹ÄÜµÄDB LineÊıÄ¿
        ret = EDAQ_DB_line_num_set(DB_line_num, nMCB);	//ÉèÖÃDB LineÊıÄ¿¡£
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
        //³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ
        ETH_CTRL_read_buffer_residual_data(nMCB);
        //¹¦ÄÜ£ºÉèÖÃDB°å×ÓÊıÄ¿
        ret =	EDAQ_DB_num_set(m_DMInfo[nMCB].nDBDMNUM[3], m_DMInfo[nMCB].nDBDMNUM[2], m_DMInfo[nMCB].nDBDMNUM[1], m_DMInfo[nMCB].nDBDMNUM[0], nMCB);//ÉèÖÃ4¸öDB Line¸÷×ÔÁ¬½ÓDB°åÊıÄ¿¡£µÚÒ»¸ö²ÎÊı£¬¶ÔÓ¦DB Line 3µÄDB num¡£
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
        //³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
         ETH_CTRL_read_buffer_residual_data(nMCB);
        ///////////////////////////////////////////////////////////////
        INT8U nMaxDBLine = EDAQ_read_DB_line_max_num(nMCB);
        memset(tmp,0,512);
        sprintf(tmp,"HMCB(4)%d is ECM1GA-%d and support the maximum value of DB line number is : %d.\n", nMCB, nMaxDBLine, nMaxDBLine);
        sz_szInfo = tmp;
        std::cout << (sz_szInfo);
        WriteLogFile(sz_szInfo);
        ///////////////////////////////////////////////////////////////
        //³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
         ETH_CTRL_read_buffer_residual_data(nMCB);
        //////////////////////////////////////////////////
        //HMCB(4) FPGA¹Ì¼şºËĞÄ°æ±¾ºÅºÍÍâÎ§°æ±¾ºÅÉú³ÉÊ±¼ä
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
        //ÓÃ»§¿É×Ô¶¨ÒåµÄ×´Ì¬¼Ä´æÆ÷(REV_STATUS_REG1,µØÖ·Îª32*4)ÓÃÓÚ±íÊ¾HMCB FPGA¹Ì¼şÍâÎ§°æ±¾ºÅ
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
        //EEPROMÖĞ´æ´¢µÄ°å×ÓÉú²úÊ±¼ä
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
        //ÏÂÔØFPGAºóÖØĞÂ¶ÁÈ¡¹Ì¼şºËĞÄ°æ±¾ºÅµÈĞÅÏ¢
        //³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
        ETH_CTRL_read_buffer_residual_data(nMCB);
        ///////////////////////////////////////////////////////////////
        //HMCB FPGA¹Ì¼şºËĞÄ°æ±¾ºÅºÍÍâÎ§°æ±¾ºÅÉú³ÉÊ±¼ä
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
        //ÓÃ»§¿É×Ô¶¨ÒåµÄ×´Ì¬¼Ä´æÆ÷(REV_STATUS_REG1,µØÖ·Îª32*4)ÓÃÓÚ±íÊ¾HMCB FPGA¹Ì¼şÍâÎ§°æ±¾ºÅ
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
        //EEPROMÖĞ´æ´¢µÄ°å×ÓÉú²úÊ±¼ä
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
        //ÉèÖÃCB¹¤×÷²ÎÊı
        SetReg(nMCB);
        /////////////////////////////////////////////////////////////////////
        int nRet = 0;
        //ÉèÖÃ¸÷DBlineµÄDMÅäÖÃ²ÎÊı
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
		m_MCBReadySts[nMCB] |=0x10;//×Ô¼ìÊ§°Ü
		//¼æÈİÀÏ°æ±¾±¨ Detector module overflow
		SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_DMOVERFLOW);
		//ĞÂ°æ±¾±¨ Detector module number error.
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
	//ÊÕµ½[Start]ÃüÁî´ÎÊı
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
	//Ö÷HMCB(4)Ä£¿é£¬µ÷ÓÃEDAQ_daq_mode_enable(0)£¬È·±£²»´¦ÓÚÊı¾İ²É¼¯Ä£Ê½¡£
	for(i = 0; i < m_nMCBNum; i++)
	{
		////////////////////////////////////////////////////////////////
		//³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
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
	//¸´Î»ÉÏÒ»´ÎÊı¾İ²É¼¯ÊÇ·ñ³É¹¦Íê³É±êÖ¾
	for(i = 0; i < m_nMCBNum; i++)
	{
		m_bPreDAQSucessFlag[i] = false;
	}
	////////////////////////////////////////////////////////////
	//¼ÇÂ¼data socketÔ­Ê¼Êı¾İÎÄ¼ş
/*	if( m_nLogLevel >= 5)
	{
		//////////////////////////////////////////////////////
		SYSTEMTIME t_st;
		GetLocalTime(&t_st);
		//¹Ø±ÕÇ°Ãæ´ò¿ªµÄÎÄ¼ş
		if(m_bSocketRawDatafile)
		{
			m_SocketRawDatafile.Close();
			m_bSocketRawDatafile = FALSE;
		}
		//
		m_unSocketRawWrCnt = 0;//Ğ´Èë¼ÇÂ¼µÄ×ÜÊı¾İ×Ö½ÚÊı
		//ËæÊ±¼äĞÎ³ÉĞÂµÄÎÄ¼şÃû
		m_SocketRawDatafilename.Format(_T("%s\\SocketRawData_%04d%02d%02d%02d%02d%02d.txt"), m_szPath, t_st.wYear, t_st.wMonth, t_st.wDay, t_st.wHour, t_st.wMinute, t_st.wSecond);
		//´´½¨´ò¿ªĞÂÎÄ¼ş
		locale loc = locale::global(locale("")); //Òª´ò¿ªµÄÎÄ¼şÂ·¾¶º¬ÖĞÎÄ£¬ÉèÖÃÈ«¾ÖlocaleÎª±¾µØ»·¾³  
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
		//´ò¿ªÎÄ¼ş
		locale::global(loc);//»Ö¸´È«¾Ölocale</span>  
	}
    */ //¼ÇµÃ·Å¿ª ssx 2023Äê12ÔÂ08ÈÕ10:21:14
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	m_bStart = true;
	m_bPreStop = false;
	m_bPreDataEnd = false;
	//ÒÑÊÕµ½Í£Ö¹²É¼¯ÃüÁî
	m_bRecStopCmd = false;
	//
	m_nSendAllRecorded = 0;//½øÈësend all Ä£Ê½ÈÕÖ¾ÊÇ·ñÒÑ¼ÇÂ¼ 0 ³õÊ¼ 1ĞèÒª¼ÇÂ¼ 2ÒÑ¼ÇÂ¼
	
    //m_nMissAA55Cnt = 0;
    for(i = 0; i < m_nMCBNum; i++)
    {
        m_nMissAA55Cnt[i]=0;
    }
	//
	///////////////////////////////////////////////////////////////
	////Ò»´Î²É¼¯ĞèÒª»ñÈ¡µÄ´¥·¢Êı¾İ°üÊıÁ¿
	//0:ÎŞÏŞ°üÊı
	//>0:ĞèÒªÄ¿±ê°üÊı
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
        memset(m_MCBDataBackup, 0x00, sizeof(m_MCBDataBackup));//Ç°´ÎÊı¾İ±¸·İ
        m_lPrePulseCnt[i] = 0;//Ç°´ÎÕı³£½âÎöÊı¾İ
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
	//Ê¹ÄÜCheckRunStatus
    m_bPreStart = true;

	memset(m_uDetData, 0, sizeof(m_uDetData));
	memset(m_uDBParameter, 0, sizeof(m_uDBParameter));
	memset(m_uDBPulse, 0, sizeof(m_uDBPulse));
	/////////////////////////////////////////////////////////////
	for(i = 0; i < MAX_MCBNUM; i++)
	{
		parsed_pkg_n[i] = 0;//ÒÑ¾­½âÎöµÄÊı¾İ°ü¸öÊı
		Update_pkg_n[i] = 0;//ÒÑ¾­½âÎöÇÒ·¢ËÍApp¸üĞÂµÄÊı¾İ°ü¸öÊı
		SC_1st_pkg_index[i] = 0;//Ã¿Ò»¸öDB Line·µ»ØµÄµÚÒ»¸öÊı¾İ°ü
		//
		SC_1st_pkg_TriggCnt[i] = 0;//Ã¿Ò»¸öDB Line·µ»ØµÄµÚÒ»¸öÊı¾İ°üÄÚ´¥·¢¼ÆÊı
		//SC_Current_pkg_TriggCnt[i] = 0;//Ã¿Ò»¸öDB Line·µ»ØµÄµ±Ç°Êı¾İ°üÄÚ´¥·¢¼ÆÊı
		//
		SC_pre_pkg_index[i] = 0;//Ã¿Ò»¸öDB Line·µ»ØµÄÇ°Ò»¸öÊı¾İ°üË÷Òı
		SC_current_pkg_index[i] = 0;//Ã¿Ò»¸öDB Line·µ»ØµÄµ±Ç°Êı¾İ°üË÷Òı
		///////////////////////////////////////////////////////////////
		//ÊÇ·ñÒÑ»ñÈ¡µ½¿ªÊ¼µÄ0xA5Êı¾İ°ü
		m_bGet0xA5Packet[i] = false;
		//»ñÈ¡µ½0xA5Êı¾İ°üÇ°ÒÑ½âÎö³öµÄÊı¾İ°ü
		m_nMiss0XA5PacketCnt[i] = 0;
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	}
	///////////////////////////////////////////////////////////////
	INT8U ADFormat_Bit = (m_nADBit > 16) ? 20 : 16; //AD×ª»»Î»Êı
	//
	UINT nRowNum[MAX_MCBNUM] = { 0 };//Ã¿ĞĞ/ÁĞ±ÊÊı
	UINT nColNum[MAX_MCBNUM] = { 0 };//ĞĞÁĞÊı
	//UINT nDMNum[MAX_MCBNUM] = { 0 };//Ã¿¸öDBLine´øµÄÓĞĞ§Ä£¿éÊı
	for(i = 0; i < MAX_MCBNUM; i++)
	{
		nRowNum[i] = m_DMInfo[i].nRowNum;//Ã¿ĞĞ/ÁĞ±ÊÊı
		nColNum[i] = m_DMInfo[i].nColNum;//ĞĞÁĞÊı
		//nDMNum[i] = m_DMInfo[i].nDMNum;//Ã¿¸öDBLine´øµÄÓĞĞ§Ä£¿é
	}
	///////////////////////////////////////////////////////////
	//
	INT32U OneDBSize = 0;
	//Ò»¸öCBDataPackageµÄsize = 6×Ö½ÚÂß¼­+12×Ö½ÚÊı¾İ+1×Ö½ÚCRC
	INT32 CBDataPackSize = 6 + 12 + 1;
	//
	for(i = 0; i < m_nMCBNum; i++)
	{
		//////////////////////////////////////////////////////////////////////
		//Êı¾İ½âÎö°´ÖÇÄÜ£¨=0£©»¹ÊÇ¸ßÄÜ£¨=1£©Ä£Ê½½âÎö´¦Àí
		if(m_bHEorCXMode)
		{
			//Ò»¸öDB°üsize = 6×Ö½ÚÍ·+£¨2+Í¨µÀÊı£©¸öÍ¨µÀÊı¾İ+1¸öCRC
			//2022-08-09¸ÄÎªÒ»¸öDB°üsize = 6×Ö½ÚÍ·+£¨Ä£¿éÍ¨µÀÊı£©¸öÍ¨µÀÊı¾İ+1¸öCRC
			OneDBSize = 6 + (nRowNum[i]*nColNum[i])*ADFormat_Bit/8 + 1;
		}
		else
		{
			//for ÖÇÄÜ£ºÒ»¸öDB°üsize = 6×Ö½ÚÍ·+£¨2(2¸öÌØÊâÍ¨µÀ)+Í¨µÀÊı£©¸öÍ¨µÀÊı¾İ+1¸öCRC
			OneDBSize = 6 + (2 + nRowNum[i]*nColNum[i])*ADFormat_Bit/8 + 1;
		}
        //////////////////////////////////////////////////////////////////////
        gbl_MCBTotal_Size[i] = 0;
        for(j = 0; j < m_DMInfo[i].nDBLineEnNum; j++)
		{
			//Ò»¸öDBlineµÄÊı¾İsize = 10¸ö°üÍ· + DBÊıÁ¿*Ò»¸öDB°üsize + Ò»¸öCBDataPackageµÄsize
			gbl_DBLine_Size[i][j] = 10 + m_DMInfo[i].nDBDMNUM[j]*OneDBSize + CBDataPackSize;
            gbl_MCBTotal_Size[i] += gbl_DBLine_Size[i][j];
		}
		//
		glb_data_AccWrBytes[i] = 0;		//¼ÇÂ¼×Ü½ÓÊÕÊı¾İ×Ö½ÚÊı
		glb_data_AccRdBytes[i] = 0;		//¼ÇÂ¼×Ü¶Á³ö´¦Àí×Ö½ÚÊı
		gbl_data_buf_wraddr[i] = 0;		//¼ÇÂ¼Êı¾İ»º´æÇøÏÂÒ»¸öÊı¾İµÄĞ´ÈëÎ»ÖÃ
		gbl_data_buf_rdaddr[i] = 0;		//¼ÇÂ¼ÏÂÒ»¸öÊı¾İµÄ¶ÁÈ¡µØÖ·	
		gbl_sc_pkg_index[i] = -1;		//¼ÇÂ¼¸÷¸öDB LineÉÏÒ»¸ösc pkgµÄ°ü±àºÅ
	}
	////////////////////////////////////////////////////////////
	//hMutex = CreateMutex(NULL, FALSE, NULL);
	////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////
    //å¯åŠ¨æ¯ MCB ç‹¬ç«‹è§£æçº¿ç¨‹ï¼ˆV2.0.1.1 è§£è€¦ï¼šæ¥æ”¶çº¿ç¨‹åªå…¥ç¼“å†²ï¼›æ­¤å¤„å·²åœ¨
    //gbl_DBLine_Size è®¡ç®—ä¹‹åï¼Œä¿è¯è§£æçº¿ç¨‹è¯»åˆ°çš„åŒ…å°ºå¯¸æœ‰æ•ˆï¼‰
    if (!StartParseThreads())
    {
        memset(tmp,0,512);
        sprintf(tmp,"Old parse thread still running, cannot start new one.\n");
        sz_szInfo = tmp;
        WriteLogFile(sz_szInfo, gLogLevel1);
        SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, MCB_STSCD_STARTRUN);
        //å›æ”¶å·²ç»å¯åŠ¨çš„è§£æçº¿ç¨‹
        StopParseThreads();
        //å¤±è´¥åç½®å› m_bStart=0ï¼Œä¸‹æ¬¡ Start_HMCB4 ä¸ä¼šå›  m_bStart å·²ç½®ä½è€Œæ‹’ç»
        m_bStart = false;
        return;
    }
	/////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////
	//for(i = 0; i < m_nMCBNum; i++)
	//{
	//	//³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
	//	ETH_CTRL_read_buffer_residual_data(i);
	//}
	///////////////////////////////////////////////////////////////
	unsigned short nMCB = 0;
	//Èç¹ûÖ»ÓĞÒ»¸ö¿ØÖÆºĞ£¬ÔòÖ±½ÓÆô¶¯²É¼¯
	//Èç¹ûÓĞ2¸ö»ò2¸öÒÔÉÏ¿ØÖÆºĞ£¬ÔòÏÈÆô¶¯´Ó¿ØÖÆºĞ£¬ÔÚOnTime ->CheckRunStatusº¯ÊıÄÚÔÙÆô¶¯Ö÷¿ØÖÆºĞ
	if (1 == m_nMCBNum)  //only one MCB, start at once;
	{
		nMCB = 0;
		m_MCBRunSts[nMCB] &= 0xfe;
		/////////////////////////////
		//²»ĞèCheckRunStatus
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
		////³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
		//ETH_CTRL_read_buffer_residual_data(nMCB);
		///////////////////////////////////////////////////////////////////
		////Æô¶¯Êı¾İ²É¼¯
		////¹¦ÄÜ£º·¢³öDB DAQÃüÁî´«ÊäÇëÇó
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
		////³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
		//ETH_CTRL_read_buffer_residual_data(nMCB);
		///////////////////////////////////////////////////////////////////
		////³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
		////Ñ¡Ôñ´¥·¢·½Ê½
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
		////³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
		//ETH_CTRL_read_buffer_residual_data(nMCB);
		///////////////////////////////////////////////////////////////////
		////½øÈëÈ¡ÊıÄ£Ê½//CB½øÈëµ½Êı¾İ²É¼¯Ä£Ê½
		//ret = EDAQ_daq_mode_enable(1, nMCB);//½øÈëÈ¡ÊıÄ£Ê½//CB½øÈëµ½Êı¾İ²É¼¯Ä£Ê½
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
		//´Ó¿ØÖÆºĞÊÇ·ñÒÑÈ«²¿Æô¶¯
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
			////³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
			//ETH_CTRL_read_buffer_residual_data(nMCB);
			//usleep(10*1000);
			////Æô¶¯Êı¾İ²É¼¯
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
			////³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
			////Ñ¡Ôñ´¥·¢·½Ê½
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
			////³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
			//ETH_CTRL_read_buffer_residual_data(nMCB);
			////½øÈëÈ¡ÊıÄ£Ê½//CB½øÈëµ½Êı¾İ²É¼¯Ä£Ê½
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
    //³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
    ETH_CTRL_read_buffer_residual_data(nMCB);
    ////////////////////////////////////////////////////////////////
    //¹Ø±Õ¿ØÖÆºÍÊı¾İÁ´½Ósocket
    ETH_CTRL_close(nMCB);
    //
    if (m_pDataSocket[nMCB])
    {
        m_pDataSocket[nMCB]->Close();
        delete m_pDataSocket[nMCB];
        m_pDataSocket[nMCB] = NULL;
    }
    ///////////////////////////////////////////////////////////////////////////////
    //ÖØĞÂ½¨Á¢¿ØÖÆºÍÊı¾İÁ´½Ósocket£¬³É¹¦·µ»Øtrue
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
        //»Ö¸´Ä¬ÈÏÉèÖÃ£¬½ûÖ¹ËùÓĞÊı¾İÔ´
        ret = EDAQ_daq_mode_enable(0, nMCB);//ÍË³öDAQÄ£Ê½
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
        ret = EDAQ_TEST_DATA_enable(0, nMCB);	//Ê¹ÄÜ(ÖÃÒ»)»ò½ûÓÃ£¨ÇåÁã£©FPGAÄÚ²¿¼ÆÊıĞÍ²âÊÔÊı¾İ²úÉúÄ£¿é
        if(ret > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_TEST_DATA_enable(0) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////
        ret = EDAQ_SC_data_pkg_monitor_enable(0, nMCB);	//Ê¹ÄÜ(ÖÃÒ»)»ò½ûÓÃ£¨ÇåÁã£©FPGAÄÚÇ¶µÄSCÊı¾İ°ü¼ì²éÄ£¿é
        if(ret > 0)
        {
            memset(tmp,0,512);
            sprintf(tmp,"Error when calling EDAQ_SC_data_pkg_monitor_enable(0) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo= tmp;
            std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////
        //Âß¼­¸´Î»FPGAÂß¼­//The parsed result of DB num of DB-Line[0..3]¶ÔÓ¦¼Ä´æÆ÷ÖµÇå0
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
        //³ÖĞø´ÓETH(_ETH_Data_socket)¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
        //ETH_read_buffer_residual_data();
        ///////////////////////////////////////////////////////////////
        //¹¦ÄÜ£ºÉèÖÃMCB¹¤×÷Ä£Ê½
        //²ÎÊı:
        // 1: Work as Master HMCB
        // 0: Work as Cascaded HMCB
        if(0 == nMCB)
        {
            //ÉèÖÃHMCB¼¶Áª¹¤×÷Ä£¿é
            ret = EDAQ_HMCB_work_mode_set(1, nMCB);	//1£ºÖ÷Ä£Ê½£»0£º´ÓÄ£Ê½
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
            //ÉèÖÃHMCB(4)¼¶Áª¹¤×÷Ä£¿é
            ret = EDAQ_HMCB_work_mode_set(0, nMCB);	//1£ºÖ÷Ä£Ê½£»0£º´ÓÄ£Ê½
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
        //¹¦ÄÜ£ºÉèÖÃDB LineÊıÄ¿//Ã¿¸öHMCB(4)¶ÔÓ¦Ò»¸öDB Line
        ret = EDAQ_DB_line_num_set(DB_line_num, nMCB);	//ÉèÖÃDB LineÊıÄ¿¡£
        if(ret > 0)
        {
            char tmp[512] = {0};
            sprintf(tmp,"Error when calling EDAQ_DB_line_num_set(1) by HMCB(4)%d. The return value is %d.\n", nMCB, ret);
            sz_szInfo = tmp;
            std::cout << sz_szInfo;
            WriteLogFile(sz_szInfo);
        }
        ///////////////////////////////////////////////////////////////
        //¹¦ÄÜ£ºÉèÖÃDB°å×ÓÊıÄ¿
        ret =	EDAQ_DB_num_set(m_DMInfo[nMCB].nDBDMNUM[3], m_DMInfo[nMCB].nDBDMNUM[2], m_DMInfo[nMCB].nDBDMNUM[1], m_DMInfo[nMCB].nDBDMNUM[0], nMCB);//ÉèÖÃ4¸öDB Line¸÷×ÔÁ¬½ÓDB°åÊıÄ¿¡£µÚÒ»¸ö²ÎÊı£¬¶ÔÓ¦DB Line 3µÄDB num¡£
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
            //HMCB FPGA¹Ì¼şºËĞÄ°æ±¾ºÅºÍÍâÎ§°æ±¾ºÅÉú³ÉÊ±¼ä
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
            //ÓÃ»§¿É×Ô¶¨ÒåµÄ×´Ì¬¼Ä´æÆ÷(REV_STATUS_REG1,µØÖ·Îª32*4)ÓÃÓÚ±íÊ¾HMCB FPGA¹Ì¼şÍâÎ§°æ±¾ºÅ
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
            //EEPROMÖĞ´æ´¢µÄ°å×ÓÉú²úÊ±¼ä
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
            //ÏÂÔØFPGAºóÖØĞÂ¶ÁÈ¡¹Ì¼şºËĞÄ°æ±¾ºÅµÈĞÅÏ¢
            ///////////////////////////////////////////////////////////////
            //HMCB(4) FPGA¹Ì¼şºËĞÄ°æ±¾ºÅºÍÍâÎ§°æ±¾ºÅÉú³ÉÊ±¼ä
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
            //ÓÃ»§¿É×Ô¶¨ÒåµÄ×´Ì¬¼Ä´æÆ÷(REV_STATUS_REG1,µØÖ·Îª32*4)ÓÃÓÚ±íÊ¾HMCB FPGA¹Ì¼şÍâÎ§°æ±¾ºÅ
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
            //EEPROMÖĞ´æ´¢µÄ°å×ÓÉú²úÊ±¼ä
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
        ////ÉèÖÃCB¹¤×÷²ÎÊı
        //SetReg(nMCB);
        ///////////////////////////////////////////////////////////////////////
        ////Õı³£Æô¶¯ºóÕâÒ»²½¿ÉÒÔÖØĞÂÉèÖÃ£¬Ò²¿ÉÒÔ²»ÖØĞÂÉèÖÃ
        ////ÖØĞÂÉèÖÃ²ÎÊıºÄÊ±»á³¤£¬ÎªÁË¼õÉÙstartÊ±¼ä£¬Ò»°ã²»ÔÙÖØĞÂÉèÖÃ£¬ºóĞø¼ÇÂ¼ The parsed result of DB num of DB-Line[0..3]: ÏÔÊ¾0ÎªÕı³£
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
	//ÅĞ¶ÏÊÇ·ñ´æÔÚ D:\\XRayV3\\Config\\Setting.ini »ò D:\\XRayV4\\Config\\Setting.iniÎÄ¼şÇÒÉèÖÃÁË_T("MainConfig"), _T("DeviceType"),²ÎÊı
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
	//¶à¶Ë¿ÚµÄbinÎÄ¼şÒ»´ÎÍê³ÉÅäÖÃ
	INT8U ret = 0;
	INT8U nLastError = 0;
	INT32S db_fpga_cfg_file_len_in_byte = 0;
	//
	for(i = 0; i < m_DMInfo[nMCB].nDBLineEnNum; i++)
	{
		ret = 0;
		db_fpga_cfg_file_len_in_byte = 0;
		//MCB dbline bin file ÅäÖÃ±êÖ¾Î»
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
			//³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
			ETH_CTRL_read_buffer_residual_data(nMCB);
			/////////////////////////////////////////////////////////////////////////
			ret = EDAQ_CB_logic_reset(nMCB);			//CBÂß¼­¸´Î»
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
			//³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
			ETH_CTRL_read_buffer_residual_data(nMCB);
			ret = EDAQ_RB_working_mode_set(0, nMCB);		//ÉèÖÃRB»º´æ¹¤×÷Ä£Ê½. 1: CMD mode. 0: Auto mode.
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
			//³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
			ETH_CTRL_read_buffer_residual_data(nMCB);
			//---------------------DB FPGA program part 1 start---------------------------------------
			//¶ÁÈ¡DB FPGA±à³ÌÎÄ¼şµÄÊı¾İ£¬½«ÆäĞ´Èëµ½HMCBµÄDDR»º´æÖĞ
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
			//³ÖĞø´ÓETH¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
			//³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
			ETH_CTRL_read_buffer_residual_data(nMCB);
			//
			//ÉèÖÃDB FPGA±à³ÌÎÄ¼ş³¤¶È
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
			//³ÖĞø´ÓETH¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
			//³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
			ETH_CTRL_read_buffer_residual_data(nMCB);
			//
			//Ñ¡ÔñÊ¹ÄÜ²¿·Ö»òÈ«²¿DB Line£¬ÔÊĞíÏòÆäÉÕĞ´DB FPGA³ÌĞò
			//EDAQ_DB_Line_FPGA_program_enable
			//¹¦ÄÜ£ºÊ¹ÄÜÈ«²¿»ò²¿·ÖDB LineµÄDB FPGA³ÌĞòÉÕĞ´ĞÅºÅ
			//	Í¨¹ı¶à´Îµ÷ÓÃ±¾º¯Êı£¬Ñ¡Ôñ²¿·ÖDB Line½øĞĞFPGA³ÌĞòÉÕĞ´£¬¿ÉÒÔÊµÏÖÏò²»Í¬DB LineĞ´Èë²»Í¬µÄDB FPGA³ÌĞò¡£
			//²ÎÊı£º
			//	enable_x4: 4 bits. 
			//			b3 = 1£ºÔÊĞíÏòDB Line #3µÄDB FPGAÉÕĞ´³ÌĞò
			//			b2 = 1£ºÔÊĞíÏòDB Line #2µÄDB FPGAÉÕĞ´³ÌĞò
			//			b1 = 1£ºÔÊĞíÏòDB Line #1µÄDB FPGAÉÕĞ´³ÌĞò
			//			b0 = 1£ºÔÊĞíÏòDB Line #0µÄDB FPGAÉÕĞ´³ÌĞò
			//·µ»ØÖµ£ºÎŞ
			//×¢Òâ£º
			//	ÒªÏòÄ³¸ö»òÄ³Ğ©DB LineµÄDB FPGAÉÕĞ´³ÌĞò£¬³ıÁËµ÷ÓÃ±¾º¯ÊıÊ¹ÄÜÏàÓ¦µÄDB Line£¬»¹±ØĞëµ÷ÓÃEDAQ_DB_FPGA_cfg_enable(1)º¯Êı¡£
			//EDAQ_DB_Line_FPGA_program_enable(m_nDBLine_FPGAFileConfigFlag[i], nMCB);
			//ret = EDAQ_DB_Line_FPGA_program_enable(15, nMCB);
			//Ö»Ê¹ÄÜÁËDB Line #0
			//Ñ¡ÔñÊ¹ÄÜ²¿·Ö»òÈ«²¿DB Line£¬ÔÊĞíÏòÆäÉÕĞ´DB FPGA³ÌĞò
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
			//³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
			ETH_CTRL_read_buffer_residual_data(nMCB);
			//ÇëÇó¿ªÊ¼ÏòDB FPGAÉÕĞ´³ÌĞò
			EDAQ_DB_fpga_config_request(nMCB);
			//
			while (EDAQ_DB_FPGA_cfg_finished_flag_rd(nMCB) == 0)
			{
				loop_delay();
			}
			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//Ã¿´ÎÍê³ÉDB FPGAµÄ³ÌĞòÉÕĞ´£¬±ØĞë¸´Î»HMCB(4) logic¡£´Ó¶ø£¬ÎªºóĞøFPGA³ÌĞòÉÕĞ´»òÊı¾İ²É¼¯×ö×¼±¸¡£
			//³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
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
//Í¨¹ıÓÃ»§²ÎÊıÉèÖÃÔöÒæÄ£Ê½ SETRANGE
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
    //ÉèÖÃ¸÷DBlineµÄDMÅäÖÃ²ÎÊı
    for(INT8U ndb = 0; ndb < DB_line_num; ndb++)
    {
	//Í¨¹ıÓÃ»§²ÎÊıÉèÖÃÔöÒæÄ£Ê½ SETRANGE
	nRet += ConfigureUserParameter_HMCB4(ndb, nMCB);
    }
	/////////////////////////////////////////
    //²ÎÊıÉèÖÃ³É¹¦
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
//ÅäÖÃHCB0µÄnDBlineÉÏÌ½²âÆ÷²ÎÊı
int CMCBCtrlC::ConfigureUserParameter_HMCB4(INT8U nDbline, unsigned short nMCB/*=0*/)
{
    std::string sz_szInfo = ("");
    std::string str = ("");
    char tmp[512] = {0};
    //////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////
    //²ÎÊı³¤¶È£ºDBÊıÄ¿*2
    //ÆäÖĞ£¬µÚi*2¡«i*2+1¸ö×Ö½ÚµÄ²ÎÊı¶ÔÓ¦×Å±àºÅÎªiµÄDBµÄ²ÎÊı¡£
    //½«×Ö½Ú×ª»¯Îª²ÎÊıÊ±£¬°´ÕÕ´ó¶Ë¸ñÊ½½øĞĞ£¬¼´ĞòºÅĞ¡µÄ²ÎÊı¶ÔÓ¦×Å²ÎÊıµÄ¸ß×Ö½Ú¡£
    INT8U i,dbline_i;
    INT8U wr_prm[256] = { 0x00 };
    INT8U rd_prm[256] = { 0x00 };
    INT8U nDMNum = m_DMInfo[nMCB].nDBDMNUM[nDbline];
    INT8U prm_len = 2*nDMNum;
    INT8U nFucRet = 0;//º¯Êı·µ»ØÖµ
    INT8U nLastError = 0;//Òì³£±êÊ¶Öµ
    //
    memset(tmp,0,512);
    sprintf(tmp,"Configure HMCB(4)%d-DB%d %d DMs parameters:", nMCB, nDbline, m_DMInfo[nMCB].nDBDMNUM[nDbline]);
    sz_szInfo += tmp;
    //ÏÈ·¢Àë¿ØÖÆÄ£¿é×îÔ¶Ò»¸öÄ£¿éÅäÖÃ²ÎÊı£¬ºó·¢Àë¿ØÖÆÄ£¿é×î½üÒ»¸öÄ£¿éÅäÖÃ²ÎÊı
    for(i=0; i < nDMNum; i++)
    {
        memset(tmp,0,512);
        sprintf(tmp,"[DM%d]-0x%04X, ", i, m_nMCBxDByREG[nMCB][nDbline][nDMNum - 1 - i]);
        sz_szInfo += tmp;
        //
	wr_prm[2*i]     = (m_nMCBxDByREG[nMCB][nDbline][nDMNum - 1 - i] & 0xff00)>>8;
	wr_prm[2*i + 1] = (m_nMCBxDByREG[nMCB][nDbline][nDMNum - 1 - i] & 0x00ff);
        //Ö¸¶¨MCBxÊÇ·ñ½øÈëÕï¶ÏÄ£Ê½£¬ÊÇ£¬ÔòÅäÖÃ²ÎÊı×î¸ßÎ»µÚ15Î»£¨×îµÍÎ»Îª0Î»£©ÖÃ1
        if(m_bMCBxDTM[nMCB])
        {
            wr_prm[2*i] |= 0x80;
        }
    }
    std::cout << sz_szInfo << std::endl ;

    WriteLogFile(sz_szInfo +("\n"));
    //////////////////////////////////////////////////////////////////////////////////
    //³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
    ETH_CTRL_read_buffer_residual_data(nMCB);
    /////////////////////////////////////////////////////////////////////////////
    //Config the control bus between CB and DB to be used as communication bus
    EDAQ_DB_FPGA_cfg_enable(0, nMCB);
    //
    EDAQ_daq_mode_enable(0, nMCB);
    /////////////////////////////////////////////////
    //¹¦ÄÜ£ºÉèÖÃRB»º´æ¹¤×÷Ä£Ê½
    //²ÎÊı£º
    //	Mode =	1: CMD mode. DBÊı¾İ»º´æ²ÉÓÃ¸ÃÄ£Ê½¡£
    //			0: Auto mode. ²âÊÔÊı¾İºÍDB FPGAÏÂÔØ³ÌĞòµÄ»º´æ²ÉÓÃ¸ÃÄ£Ê½¡£
    //·µ»ØÖµ: 0 = No error, others = Find errors.
    nFucRet = EDAQ_RB_working_mode_set(1, nMCB);	//ÉèÖÃRB»º´æ¹¤×÷Ä£Ê½. 1: CMD mode. 0: Auto mode.
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
    //¹¦ÄÜ£ºÑ¡ÔñRing bufferĞ´Èë¶Ë¿Ú
    //·µ»ØÖµ: 0 = No error, others = Find errors.
    nFucRet = EDAQ_RB_wr_port_sel(EDAQ_RB_WR_SEL_DB_RX_DATA, nMCB);  //Ñ¡ÔñDDR»º´æµÄĞ´ÈëÊı¾İµÄÀ´Ô´£ºDBÊı¾İ
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
    //¹¦ÄÜ£ºÑ¡ÔñRing buffer¶ÁÈ¡¶Ë¿Ú
    //·µ»ØÖµ: 0 = No error, others = Find errors.
    nFucRet = EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_ETH_DATA, nMCB);	 //Ñ¡ÔñDDR»º´æµÄ¶Á³öÊı¾İµÄÄ¿µÄµØ£ºÒÔÌ«Íø
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
    //³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
    ETH_CTRL_read_buffer_residual_data(nMCB);
    /////////////////////////////////////////////////
    //¹¦ÄÜ£ºÊ¹ÄÜ/³ıÄÜSC data pkg monitor
    //²ÎÊı£º
    //	en=	1: Enable
    //		0: Disable
    //·µ»ØÖµ: 0 = No error, others = Find errors.
    nFucRet = EDAQ_SC_data_pkg_monitor_enable(1, nMCB);	//Ê¹ÄÜ(ÖÃÒ»)»ò½ûÓÃ£¨ÇåÁã£©FPGAÄÚÇ¶µÄSCÊı¾İ°ü¼ì²éÄ£¿é
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
	//³ÖĞø´Ó_ETH_CTRL_socket¶ÁÈ¡Êı¾İ£¬Ö±µ½Çå¿Õ»º´æÖĞµÄÊı¾İ¡£
	ETH_CTRL_read_buffer_residual_data(nMCB);
	//////////////////////////////////////////////////////
	//¹¦ÄÜ£º°Ñ²ÎÊıĞ´Èëµ½²ÎÊıÅäÖÃRAMÖĞ£¬²¢¼ì²éĞ´ÈëÊÇ·ñÕıÈ·
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
    //¹¦ÄÜ£º·¢³öDB FPGA¸´Î»ÇëÇó
    //·µ»ØÖµ: 0 = No error, others = Find errors.
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
    //¹¦ÄÜ£º·¢³öDB¶¨Î»ÇëÇó
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
    //¹¦ÄÜ£º·¢³öDB²ÎÊıÅäÖÃÇëÇó
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
    //µ±ÉèÖÃĞ¡ÓÚ4¸öÊ±ºÄÊ±½Ï³¤£¬×¢ÊÍµô²»Ó°Ïì¹¦ÄÜ
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
	//ÕıÈ·Íê³ÉÄ£Ê½ÊıÊ¶±ğ
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
	//Ä£¿éÊı×Ô¼ìÓëÉèÖÃÖµ²»Í¬£¬ÔòĞèÖØĞÂ³õÊ¼»¯
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
		m_MCBReadySts[nMCB] |=0x80;//×Ô¼ìÊ§°Ü
		//¼æÈİÀÏ°æ±¾±¨ Detector module overflow
		SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_DMOVERFLOW);
		//ĞÂ°æ±¾±¨ Detector module number error.
		SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB))<<16) + MCB_STSCD_DMNUM_ERROR);
	}
	//
	return nRet;
}

////////////////////////////////////////////////////////////
//é€šè¿‡socketè¯»å–æ•°æ®å¹¶ç¼“å­˜è‡³FIFOç¼“å†²åŒºï¼ˆV2.0.1.1 è§£è€¦ç‰ˆï¼šåªåšæ¥æ”¶ï¼Œè§£æç”±ç‹¬ç«‹çº¿ç¨‹å®Œæˆï¼‰
void CMCBCtrlC::FetchMsg_HMCB4(CRequestSocket *pRequestSocket, int nIndex, int nPort)
{
    int nMCB = nIndex;
    // ---- åªå¤„ç†æ•°æ®ç«¯å£çš„æ•°æ® ----
    if (nPort != ETH_DATA_PORT)
        return;
    // ---- æœªå¼€å§‹é‡‡é›†æ—¶ï¼Œsocket æ”¶åˆ°çš„æ•°æ®ç›´æ¥ä¸¢å¼ƒ ----
    //ï¼ˆç¼“å†²æ¸…ç©ºç»Ÿä¸€åœ¨ Start_HMCB4 æ—¶åšï¼›æ®‹ç•™æ•°æ®ä¸ä¼šè¿›å…¥è§£æçº¿ç¨‹çš„å¾…å¤„ç†ç¼“å†²ï¼‰
    if (AtomicLoadL(&m_bStart) == 0)
    {
        unsigned char chDummy[SOCKETBUFSIZE];
        pRequestSocket->Receive(chDummy, SOCKETBUFSIZE);
        m_timeSocketRecData[nMCB] = NucTool::GetTickCount();
        return;
    }

    // ---- æ¥æ”¶çƒ­è·¯å¾„ï¼šåªåš recv + Append + signalï¼Œæœ€çŸ­ä¸´ç•ŒåŒº ----
    unsigned char chRecv[SOCKETBUFSIZE];
    INT32S Rec_Bytelen = pRequestSocket->Receive(chRecv, SOCKETBUFSIZE);
    if (Rec_Bytelen <= 0)
        return;

    // ---- Append + è®¡æ•° + signal æ”¾åœ¨åŒä¸€ä¸´ç•ŒåŒºï¼Œé¿å…ä¸¢å¤±å”¤é†’ ----
    //ï¼ˆæ‰‹åŠ¨é‡ç½®äº‹ä»¶ï¼šsignal åä¿æŒä¿¡å·æ€ï¼Œç”±è§£æçº¿ç¨‹æ¶ˆè´¹å clearï¼‰
    {
        std::lock_guard<std::mutex> lk(g_parseCtx[nMCB].csBuf);
        g_parseCtx[nMCB].shared.Append(chRecv, (size_t)Rec_Bytelen);
        g_parseCtx[nMCB].totalRecvBytes += (unsigned long long)Rec_Bytelen;
        g_parseCtx[nMCB].dataEvent.signal();
    }

    // ---- è¯Šæ–­ï¼ˆHMCB_LIB_DEBUG=1ï¼‰ï¼šæ¥æ”¶ä¾§æ¯ç§’ç»Ÿè®¡ ----
    static unsigned s_dbgRecvCnt = 0; static unsigned long long s_dbgRecvBytes = 0;
    static DWORD s_dbgRecvT0 = 0;
    if (getenv("HMCB_LIB_DEBUG")) {
        s_dbgRecvCnt++; s_dbgRecvBytes += (unsigned long long)Rec_Bytelen;
        DWORD now = NucTool::GetTickCount();
        if (s_dbgRecvT0 == 0) s_dbgRecvT0 = now;
        if (now - s_dbgRecvT0 >= 1000) {
            fprintf(stderr, "[libdbg][recv] cnt=%u bytes=%llu start=%ld stop=%ld\n",
                    s_dbgRecvCnt, s_dbgRecvBytes, (long)AtomicLoadL(&m_bStart), (long)AtomicLoadL(&m_bPreStop));
            s_dbgRecvCnt = 0; s_dbgRecvBytes = 0; s_dbgRecvT0 = now;
        }
    }
        // æ¯æ¿é¦–ä¸ªæœ‰æ•ˆåŒ…çš„å‰ 48 å­—èŠ‚ hexï¼ˆä¸‰æ¿æµç»“æ„æ¯”å¯¹ï¼‰
        if (Rec_Bytelen > 48) {
            static bool s_dbgFirst[8] = {false};
            if (!s_dbgFirst[nMCB]) {
                s_dbgFirst[nMCB] = true;
                char hx[256] = {0};
                for (int k = 0; k < 48; k++) sprintf(hx + k*3, "%02X ", chRecv[k]);
                fprintf(stderr, "[libdbg][first%d] %s\n", nMCB, hx);
            }
        }
    m_timeSocketRecData[nMCB] = NucTool::GetTickCount();

    // ---- ä¿å­˜åŸå§‹socketæ•°æ®è®°å½•ï¼ˆLevel5 è°ƒè¯•ç”¨ï¼›ä¸åšæ¯ç¬” Flush é™ä½å¼€é”€ï¼‰----
    if ((0 == nMCB) && m_bSocketRawDatafile && (m_nLogLevel >= gLogLevel5))
    {
        std::string sz_szInfo = ("");
        char tmp[16] = {0};
        for (INT32S nWr = 0; nWr < Rec_Bytelen; nWr++)
        {
            memset(tmp,0,16);
            sprintf(tmp,"%02X ", chRecv[nWr]);
            m_unSocketRawWrCnt++;
            if (0 == (m_unSocketRawWrCnt % 64))
                strcat(tmp, "\n");
            sz_szInfo += tmp;
        }
        m_SocketRawDatafile << sz_szInfo;
    }
}

//////////////////////////////////////////////////////////////////
//¶àÏß³Ì½âÎöÊı¾İ
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
// ============================================================
// V2.0.1.1 æ¥æ”¶/è§£æè§£è€¦æ”¹é€ ï¼šåŸ"æ¥æ”¶+è§£æ"ä¸€ä½“çº¿ç¨‹é€€å½¹ï¼Œ
// è§£æå·¥ä½œç”± StartParseThreads() å¯åŠ¨çš„æ¯ MCB ç‹¬ç«‹è§£æçº¿ç¨‹å®Œæˆã€‚
// ä¿ç•™å…¥å£ä»¥å…¼å®¹æ—§è°ƒç”¨ç‚¹ï¼ˆReadParseData_HMCB4ï¼‰ï¼Œä¸å†æ‰¿æ‹…è§£æã€‚
// ============================================================
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
DWORD WINAPI CMCBCtrlC::ETH_read_and_parse_dataEx_Thread_HMCB4(LPVOID lpParameter)
#else
DWORD  CMCBCtrlC::ETH_read_and_parse_dataEx_Thread_HMCB4(void * lpParameter)
#endif
{
    std::string sz_szInfo = ("");
    //å‚æ•°å†…å­˜æŒ‰æ—§å¥‘çº¦ç”±æœ¬å…¥å£è´Ÿè´£é‡Šæ”¾
    if (lpParameter != NULL) { delete (RECVPARAM *)lpParameter; lpParameter = NULL; }
    sz_szInfo = "ETH_read_and_parse_dataEx_Thread_HMCB4 retired; parsing moved to MCBParseThreadProc.\n";
    WriteLogFile(sz_szInfo, gLogLevel2);
    return 0;
}

// ============================================================
// è§£æçº¿ç¨‹ä¸»ä½“ï¼ˆå¯¹ç…§ V2.0.1.1 MCBParseThreadProcï¼‰
// ç­‰æ•°æ®äº‹ä»¶ â†’ å…±äº«ç¼“å†²æ•´ä½“æ¬åˆ°ç§æœ‰ local â†’ æ— é”è§£æå®Œæ•´åŒ…å¹¶å›è°ƒ App
// ============================================================
void CMCBCtrlC::MCBParseThreadProc(int nMCB)
{
    if ((nMCB < 0) || (nMCB >= MAX_MCBNUM))
        return;

    MCBParseContext& ctx = g_parseCtx[nMCB];

    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    memset(tmp,0,512);
    sprintf(tmp,"HMCB(4)%d parse thread started, tid=%ld.\n", nMCB, (long)syscall(SYS_gettid));
    sz_szInfo = tmp;
    WriteLogFile(sz_szInfo);

    ////////////////////////////////////////////////////////////
    // æ¯ MCB ä¸€æ¬¡é‡‡é›†çš„æœ€å°æ‰¹å¤„ç†å­—èŠ‚æ•°ï¼ˆHMCB4 è·¯å¾„æŒ‰ DBLine åŒ…é•¿å–æœ€å¤§ï¼Œ
    // å¤š DBLine æ—¶é˜²æ­¢åŠåŒ…è¿‡æ—©è¿›å…¥è§£æï¼›å¼‚å¸¸å…œåº• 4096ï¼‰
    size_t minBatchBytes = 0;
    for (INT8U d = 0; d < m_DMInfo[nMCB].nDBLineEnNum; ++d)
    {
        if (gbl_DBLine_Size[nMCB][d] > minBatchBytes)
            minBatchBytes = gbl_DBLine_Size[nMCB][d];
    }
    if (minBatchBytes == 0)
        minBatchBytes = 4096;
    ////////////////////////////////////////////////////////////
    while (0 == AtomicLoadL(&ctx.bStop))
    {
        // ---- ç­‰å¾…æ•°æ®ï¼ˆ100ms è¶…æ—¶ä»¥ä¾¿æ£€æŸ¥é€€å‡ºï¼‰----
        bool bSignaled = ctx.dataEvent.wait(100);

        if (0 != AtomicLoadL(&ctx.bStop))
            break;

        // äº‹ä»¶æœªä¿¡å·ä¸”æ•°æ®ä¸è¶³ä¸€æ‰¹ï¼šç»§ç»­ç­‰ï¼ˆè¶…æ—¶æœŸé—´æ£€æŸ¥æ˜¯å¦åº”é€€å‡ºï¼‰
        if ((!bSignaled) && (ctx.local.Size() < minBatchBytes))
        {
            if ((AtomicLoadL(&m_bStart) == 0) && (ctx.local.Size() == 0))
                break;
            continue;
        }
        // ---- è¯Šæ–­ï¼ˆHMCB_LIB_DEBUG=1ï¼‰ï¼šè§£æä¾§æ¯ç§’çŠ¶æ€ ----
        static DWORD s_dbgParseT0 = 0; static unsigned long long s_dbgParsed0 = 0;
        if (getenv("HMCB_LIB_DEBUG")) {
            DWORD now = NucTool::GetTickCount();
            if (s_dbgParseT0 == 0) { s_dbgParseT0 = now; s_dbgParsed0 = ctx.totalParsedBytes; }
            if (now - s_dbgParseT0 >= 1000) {
                fprintf(stderr, "[libdbg][parse%d] rate=%lldB/s shared=%zu local=%zu start=%ld stop=%ld bStop=%ld pkg=%u a5miss=%u\n", nMCB,
                        (long long)(ctx.totalParsedBytes - s_dbgParsed0), ctx.shared.Size(), ctx.local.Size(),
                        (long)AtomicLoadL(&m_bStart), (long)AtomicLoadL(&m_bPreStop), (long)AtomicLoadL(&ctx.bStop),
                        parsed_pkg_n[nMCB], m_nMiss0XA5PacketCnt[nMCB]);
                s_dbgParseT0 = now; s_dbgParsed0 = ctx.totalParsedBytes;
            }
        }
        // ---- æŠŠå…±äº«ç¼“å†²æ•°æ®æ¬åˆ°çº¿ç¨‹ç§æœ‰ local ----
        {
            std::lock_guard<std::mutex> lk(ctx.csBuf);
            if (ctx.shared.Size() > 0)
            {
                const unsigned char* pMain = ctx.shared.Data();
                size_t nMainLen = ctx.shared.Size();
                if (pMain && nMainLen > 0)
                    ctx.local.Append(pMain, nMainLen);
                ctx.shared.Clear();
            }
            //æ‰‹åŠ¨é‡ç½®äº‹ä»¶ç”±æ¶ˆè´¹æ–¹è‡ªå·±å¤ä½ï¼ˆä¸´ç•ŒåŒºå†…ï¼Œé˜²ä¸¢å¤±å”¤é†’ï¼‰
            ctx.dataEvent.clear();
        }

        // ---- è§£æ local ä¸­çš„æ•°æ®ï¼ˆå¾ªç¯ç›´åˆ°ä¸è¶³ä¸€åŒ…/è¦æ±‚é€€å‡ºï¼‰----
        while ((0 == AtomicLoadL(&ctx.bStop)) && (AtomicLoadL(&m_bStart) != 0))
        {
            size_t consumed = 0;
            bool bContinue = ParseBuffer_HMCB4(nMCB, ctx.local, m_lcounter, consumed);

            ctx.totalParsedBytes += consumed;

            if (!bContinue)
            {
                AtomicStoreL(&ctx.bStop, 1);
                break;      // åœæ­¢ / ç›®æ ‡åˆ°è¾¾ / ä¸¥é‡é”™è¯¯ï¼šé€€å‡ºå†…å±‚
            }

            if (consumed == 0)
                break;      // ç¼“å†²æ²¡æœ‰å¯è§£ææ•°æ®ï¼Œé€€å‡ºå†…å±‚ç­‰ä¸‹ä¸€æ¬¡äº‹ä»¶
        }

        //é‡‡é›†æœŸé—´é˜²æ­¢ç§æœ‰ç¼“å†²æ— é™è†¨èƒ€ï¼ˆConsume å†…éƒ¨å·²æ‘Šé”€å‹ç¼©ï¼Œæ­¤å¤„å…œåº•æ¸…ç©ºï¼‰
        if (ctx.local.Size() == 0)
            ctx.local.Clear();
    }

    //çº¿ç¨‹é€€å‡ºï¼šç½®å®Œæˆæ ‡å¿—å¹¶å”¤é†’ç­‰å¾…æ–¹ï¼ˆStopParseThreads çš„é™æ—¶ç­‰å¾…ï¼‰
    {
        std::lock_guard<std::mutex> lk(ctx.doneMu);
        ctx.bThreadDone = true;
    }
    ctx.doneCv.notify_all();
    //
    memset(tmp,0,512);
    sprintf(tmp,"HMCB(4)%d parse thread exit. recv=%llu, parsed=%llu\n", nMCB, ctx.totalRecvBytes, ctx.totalParsedBytes);
    sz_szInfo = tmp;
    WriteLogFile(sz_szInfo);
}

// ============================================================
// å¯åŠ¨æ‰€æœ‰ MCB çš„è§£æçº¿ç¨‹ï¼ˆStart_HMCB4 åœ¨ m_bStart=1 åè°ƒç”¨ï¼‰
// ============================================================
bool CMCBCtrlC::StartParseThreads()
{
static std::mutex s_parseLcMu;   // Start/Stop å¹¶å‘é˜²æŠ¤ï¼ˆstd::thread::join ä¸å¯é‡å…¥ï¼‰
std::lock_guard<std::mutex> lcLock(s_parseLcMu);
    bool bStartOK = true;
    std::string sz_szInfo = ("");
    char tmp[512] = {0};

    for (int i = 0; i < (int)m_nMCBNum; i++)
    {
        MCBParseContext& ctx = g_parseCtx[i];

        //æ£€æŸ¥æ—§è§£æçº¿ç¨‹æ˜¯å¦å·²é€€å‡ºï¼›æœªé€€å‡ºå…ˆé€šçŸ¥é€€å‡ºå¹¶é™æ—¶ç­‰å¾…
        if (ctx.parseThread.joinable())
        {
            if (!ctx.bThreadDone)
            {
                AtomicStoreL(&ctx.bStop, 1);
                ctx.dataEvent.signal();
                //
                bool bDone = false;
                {
                    std::unique_lock<std::mutex> lk(ctx.doneMu);
                    bDone = ctx.doneCv.wait_for(lk, std::chrono::milliseconds(500),
                                                [&ctx] { return ctx.bThreadDone; });
                }
                if (!bDone)
                {
                    //å†ç­‰ 5sï¼ˆå¯¹ç…§ Windows 500ms+5s ä¸¤æ®µå¼ç­‰å¾…ï¼‰
                    std::unique_lock<std::mutex> lk(ctx.doneMu);
                    bDone = ctx.doneCv.wait_for(lk, std::chrono::milliseconds(5000),
                                                [&ctx] { return ctx.bThreadDone; });
                }
                if (!bDone)
                {
                    memset(tmp,0,512);
                    sprintf(tmp,"HMCB(4)%d old parse thread still running, cannot start new one.\n", i);
                    sz_szInfo = tmp;
                    WriteLogFile(sz_szInfo);
                    bStartOK = false;
                    break;
                }
            }
            ctx.parseThread.join();
        }
        ////////////////////////////////////////////////////////////
        if (!bStartOK)
        {
            return bStartOK;
        }
        ////////////////////////////////////////////////////////////
        //å¤ä½çŠ¶æ€
        AtomicStoreL(&ctx.bStop, 0);
        ctx.totalRecvBytes   = 0;
        ctx.totalParsedBytes = 0;
        ctx.local.Clear();
        ctx.bThreadDone = false;

        //æ¸…ç©ºå…±äº«ç¼“å†²
        {
            std::lock_guard<std::mutex> lk(ctx.csBuf);
            ctx.shared.Clear();
            ctx.dataEvent.clear();
        }

        //å¯åŠ¨è§£æçº¿ç¨‹
        ctx.parseThread = std::thread(MCBParseThreadProc, i);

        memset(tmp,0,512);
        sprintf(tmp,"HMCB(4)%d parse thread created.\n", i);
        sz_szInfo = tmp;
        WriteLogFile(sz_szInfo);
    }
    return bStartOK;
}

// ============================================================
// åœæ­¢æ‰€æœ‰è§£æçº¿ç¨‹ï¼ˆCheckDataEnd / NormalEndDAQ / CloseMCB è°ƒç”¨ï¼‰
// å…ˆç½®åœæ­¢æ ‡å¿—å¹¶å”¤é†’ï¼Œå†é™æ—¶ç­‰å¾…è‡ªç„¶é€€å‡ºï¼›ç»ä¸å¼ºæ€çº¿ç¨‹
// ============================================================
void CMCBCtrlC::StopParseThreads()
{
static std::mutex s_parseLcMu;   // Start/Stop å¹¶å‘é˜²æŠ¤ï¼ˆstd::thread::join ä¸å¯é‡å…¥ï¼‰
std::lock_guard<std::mutex> lcLock(s_parseLcMu);
    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    //ç¬¬ä¸€æ­¥ï¼šç½®åœæ­¢æ ‡å¿—å¹¶å”¤é†’æ‰€æœ‰çº¿ç¨‹
    for (int i = 0; i < MAX_MCBNUM; i++)
    {
        if (g_parseCtx[i].parseThread.joinable())
        {
            AtomicStoreL(&g_parseCtx[i].bStop, 1);
            g_parseCtx[i].dataEvent.signal();   //å”¤é†’ç­‰å¾…
        }
    }
    //ç¬¬äºŒæ­¥ï¼šç­‰å¾…çº¿ç¨‹è‡ªç„¶é€€å‡º
    for (int i = 0; i < MAX_MCBNUM; i++)
    {
        if (g_parseCtx[i].parseThread.joinable())
        {
            MCBParseContext& ctx = g_parseCtx[i];
            bool bDone = ctx.bThreadDone;
            if (!bDone)
            {
                std::unique_lock<std::mutex> lk(ctx.doneMu);
                bDone = ctx.doneCv.wait_for(lk, std::chrono::milliseconds(5000),
                                            [&ctx] { return ctx.bThreadDone; });
                lk.unlock();
                if (!bDone)
                {
                    //ç¬¬ä¸€æ¬¡è¶…æ—¶å†ç­‰ 10sï¼ˆå¯¹ç…§ Windows 5s+10s ä¸¤æ®µå¼ï¼‰
                    memset(tmp,0,512);
                    sprintf(tmp,"HMCB(4)%d parse thread exit timeout(5s), wait another 10s...\n", i);
                    sz_szInfo = tmp;
                    WriteLogFile(sz_szInfo);
                    lk.lock();
                    bDone = ctx.doneCv.wait_for(lk, std::chrono::milliseconds(10000),
                                                [&ctx] { return ctx.bThreadDone; });
                    lk.unlock();
                }
            }
            if (bDone)
            {
                memset(tmp,0,512);
                sprintf(tmp,"HMCB(4)%d parse thread exit.\n", i);
                sz_szInfo = tmp;
                WriteLogFile(sz_szInfo);
                ctx.parseThread.join();
            }
            else
            {
                //ä»è¶…æ—¶ï¼šåˆ†ç¦»çº¿ç¨‹è®©å…¶ç¨åè‡ªè¡Œé€€å‡ºï¼ˆbStop å·²ç½®ä½ï¼›ä¸Šä¸‹æ–‡ä¸ºé™æ€å­˜å‚¨ï¼Œ
                //çº¿ç¨‹å°¾éƒ¨ä»…è®¿é—® ctx ä¸æ—¥å¿—ï¼›CLOSE è·¯å¾„ä¼šå†æ¬¡ç­‰å¾…å›æ”¶ï¼‰
                memset(tmp,0,512);
                sprintf(tmp,"HMCB(4)%d parse thread still running after 15s, detach it.\n", i);
                sz_szInfo = tmp;
                WriteLogFile(sz_szInfo);
                ctx.parseThread.detach();
            }
        }
    }
}

// è¿”å›å€¼ï¼štrue  - å¯ç»§ç»­è§£æï¼ˆç¼“å†²ä¸è¶³ä¸€åŒ…/æœ¬æ¿å·²è¾¾ç›®æ ‡åŒ…æ•°ï¼‰
//         false - è§£æçº¿ç¨‹åº”é€€å‡ºï¼ˆåœæ­¢ã€ç›®æ ‡åˆ°ã€ä¸¥é‡é”™è¯¯ï¼‰
// å‡ºå‚ï¼š  consumed - æœ¬æ¬¡ä» local å·²æ¶ˆè´¹çš„å­—èŠ‚æ•°ï¼ˆç›¸å¯¹è¯»æŒ‡é’ˆï¼‰
// æ³¨æ„ï¼š  pBuf æŒ‡å‘ local å†…éƒ¨å­˜å‚¨ï¼ŒPushFront/EraseAt/Consume åå¤±æ•ˆï¼Œ
//         å¾ªç¯å›åˆ°å¤´éƒ¨å¿…é¡»é‡æ–°å– pBuf = local.Data()
// ============================================================
bool CMCBCtrlC::ParseBuffer_HMCB4(int nMCB, CMCBDataBuffer& local, ULONG nTarget_pkg_num, size_t& consumed)
{
    consumed = 0;
    //
    std::string sz_szInfo = ("");
    char tmp[512] = {0};
    //æœªåŒ¹é…åˆ° SC å¤´çš„éæ•°æ®åŒ…å­—èŠ‚è®¡æ•°ï¼ˆä»…æ—¥å¿—ç”¨ï¼Œçº¿ç¨‹ç§æœ‰ï¼‰
    static INT32U s_nNotDataPktCnt[MAX_MCBNUM] = {0};
    //
    if (nMCB < 0 || nMCB >= MAX_MCBNUM)
        return false;
    //è‹¥æ‰€æœ‰DBLineæœªä½¿èƒ½ï¼Œç›´æ¥è¿”å›ï¼ˆé˜² 1 å­—èŠ‚/æ¬¡ç©ºè½¬æ¶ˆè€—ç¼“å†²ï¼‰
    if (0 == m_DMInfo[nMCB].nDBLineEnNum)
    {
        sz_szInfo = "HMCB(4)nDBLineEnNum == 0, cannot parse.\n";
        WriteLogFile(sz_szInfo);
        return false;
    }
    /////////////////////////////////////////////////////////////////////////////////
    // ä»ç¼“å†²æ‹¿åˆ°çš„ä¸´æ—¶çŠ¶æ€
    OneDBLineOnePackage pDBDataPackage;

    // æ¯æ¬¡è¿›å…¥è§£æéœ€è‡³å°‘å¯è§£ä¸€ä¸ªå®Œæ•´ SC åŒ…ï¼ˆHMCB4 ä¸€ä¸ª MCB æœ‰å¤šä¸ª DBLineï¼›
    // å–å„ DBLine åŒ…é•¿æœ€å¤§å€¼ï¼Œå¼‚å¸¸å…œåº• 4096ï¼‰
    const size_t SC_HEADER_SIZE = 10;
    size_t maxDBLineSize = 0;
    for (INT8U d = 0; d < m_DMInfo[nMCB].nDBLineEnNum; ++d)
    {
        if (gbl_DBLine_Size[nMCB][d] > maxDBLineSize)
            maxDBLineSize = gbl_DBLine_Size[nMCB][d];
    }
    if (maxDBLineSize == 0)
        maxDBLineSize = 4096;   //å‡ ä½•å¼‚å¸¸å…œåº•
    //
    const size_t minSize = maxDBLineSize;
    const INT32U MAX_SC_PKG_LEN = 16 * 1024 * 1024;  //é•¿åº¦ä¸Šé™ï¼ˆé˜²å¼‚å¸¸é•¿åº¦å­—æ®µï¼‰
    //
    INT32S idx = 0;
    // ------------------------------------------------------------------
    // ä¸»å¾ªç¯ï¼šæ¯æ¬¡è¿­ä»£è§£æè¶Šè¿‡ 1 ä¸ªå®Œæ•´ SC åŒ…ï¼Œæˆ– 1 å­—èŠ‚é‡åŒæ­¥
    // ------------------------------------------------------------------
    while (true)
    {
        // ---------- 0) é€€å‡ºæ¡ä»¶ ----------
        if (AtomicLoadL(&m_bStart) == 0)
            return false;

        if (nTarget_pkg_num > 0)
        {
            //æ‰€æœ‰ MCB æ˜¯å¦å·²é‡‡å¤Ÿç›®æ ‡åŒ…æ•°
            BOOL bAllReached = TRUE;
            for (idx = 0; idx < (INT32S)m_nMCBNum; ++idx)
            {
                if (parsed_pkg_n[idx] < nTarget_pkg_num)
                {
                    bAllReached = FALSE;
                    break;
                }
            }
            if (bAllReached && (AtomicLoadL(&m_bPreStop) == 0))
            {
                //è§£æçº¿ç¨‹ä¸åšStopï¼ˆå›è°ƒ/æ—¥å¿—ä¼šé‡å…¥ï¼‰ï¼Œç½®æ ‡å¿—äº¤ç”±
                //OnTimer->CheckStopSts / CheckDataEnd å»æ‰§è¡Œ
                m_timeprestop = NucTool::GetTickCount();
                AtomicStoreL(&m_bPreStop, 1);
                //
                return false;
            }
            if (parsed_pkg_n[nMCB] >= nTarget_pkg_num)
                return true;
        }

        // ---------- 1) å–ç¼“å†²æ•°æ®æŒ‡é’ˆ ----------
        size_t avail = local.Size();
        if (avail < minSize)
        {
            //æ•°æ®ä¸è¶³ä¸€åŒ…ï¼Œç­‰ä¸‹æ¬¡äº‹ä»¶
            return true;
        }
        //æ³¨æ„ï¼špBuf æŒ‡å‘ local å†…éƒ¨å­˜å‚¨ï¼ŒPushFront/EraseAt/Consume åå¤±æ•ˆ
        const unsigned char* pBuf = local.Data();
        if (pBuf == NULL)
        {
            //æ•°æ®å­˜å‚¨ä¸ºç©ºï¼Œç­‰ä¸‹æ¬¡äº‹ä»¶
            return true;
        }
        ///////////////////////////////////////////////////////////
        memset(&pDBDataPackage, 0, sizeof(pDBDataPackage));
        // ---------- 2) è§£æ SC å¤´ï¼ˆåç§» 0..9ï¼‰ ----------
        INT16U SC_pkg_lead_code = ((INT16U)pBuf[0] << 8) | pBuf[1];
        INT32U SC_pkg_len       = ((INT32U)pBuf[3] << 16) | ((INT32U)pBuf[4] << 8) | pBuf[5];
        INT8U  SC_pkg_db_line   = pBuf[6];
        INT8U  SC_pkg_type      = pBuf[7];
        INT16U SC_pkg_index     = ((INT16U)pBuf[8] << 8) | pBuf[9];
        //æŒ‰å®é™…å¯ç”¨çš„ DBLine æ•°æ ¡éªŒæ”¶åˆ°çš„ DBLine å·ï¼ˆè¶Šç•ŒæŒ‰ 1 å­—èŠ‚/æ¬¡é‡åŒæ­¥æ¶ˆè€—ï¼‰
        if (SC_pkg_db_line >= m_DMInfo[nMCB].nDBLineEnNum)
        {
            local.Consume(1);
            consumed += 1;
            continue;
        }
        /////////////////////////////////////////////////////////
        //åŒ…å¤´å¼•å¯¼ç ä¹‹åã€é•¿åº¦æ¯”è¾ƒä¹‹å‰ï¼Œå…ˆæ£€æŸ¥ SC_pkg_len æ˜¯å¦è¶…å‡ºå½“å‰å¯è¯»é•¿åº¦
        if (SC_pkg_len < SC_HEADER_SIZE || SC_pkg_len > MAX_SC_PKG_LEN)
        {
            //é•¿åº¦å­—æ®µæ— æ•ˆï¼ŒæŒ‰ 1 å­—èŠ‚/æ¬¡é‡åŒæ­¥
            local.Consume(1);
            consumed += 1;
            continue;
        }
        // ---------- 3) å¤´éƒ¨æ ¡éªŒä¸å®¹é”™ ----------
        if (SC_pkg_lead_code != 0xAA55)
        {
            // 3a) ç¼º 0xAA å®¹é”™ï¼šæ•°æ®ä¸º [55,00,L2,L1,L0,...]ï¼ŒæŠŠ 0xAA è¡¥åœ¨å‰é¢
            INT32U altLen = ((INT32U)pBuf[2] << 16) | ((INT32U)pBuf[3] << 8) | pBuf[4];
            //
            INT8U tmpDbLine = pBuf[5];
            if (tmpDbLine >= m_DMInfo[nMCB].nDBLineEnNum)
            {
                tmpDbLine = (m_DMInfo[nMCB].nDBLineEnNum > 0) ? (m_DMInfo[nMCB].nDBLineEnNum - 1) : 0;
            }
            //
            BOOL bFixMissingAA = (pBuf[0] == 0x55) && (pBuf[1] == 0x00) &&
                                 ((altLen + 1) == (INT32U)gbl_DBLine_Size[nMCB][tmpDbLine]);
            //
            //é”™è¯¯è®¡æ•°
            if (m_nMissAA55Cnt[nMCB] < 0xFFFFFFF0)
            {
                m_nMissAA55Cnt[nMCB]++;
            }
            //
            if (bFixMissingAA)
            {
                if (1 == (m_nMissAA55Cnt[nMCB] % 10000))
                {
                    sz_szInfo = "HMCB(4) missing 0xAA lead, insert and fix length.\n";
                    WriteLogFile(sz_szInfo);
                }
                //ä¿®å¤ç¼“å†²å¤´éƒ¨ï¼ˆO(1) å–å†³äºè¯»æŒ‡é’ˆ m_nReadPosï¼‰
                local.PushFront(0xAA);
                //è¡¥ 0xAA ååŒ…é•¿åº¦+1
                const INT32U newLen = gbl_DBLine_Size[nMCB][tmpDbLine];
                local.At(3) = (unsigned char)((newLen >> 16) & 0xFF);
                local.At(4) = (unsigned char)((newLen >>  8) & 0xFF);
                local.At(5) = (unsigned char)( newLen        & 0xFF);
                continue;   //é‡æ–°è¿›ä¸»å¾ªç¯æ ¡éªŒåŒ…å¤´
            }
            // 3b) æœªè¯†åˆ«çš„å¤´éƒ¨ï¼Œè·³ 1 å­—èŠ‚ç»§ç»­æ‰¾åŒ…å¤´
            if (1 == (m_nMissAA55Cnt[nMCB] % 10000))
            {
                sprintf(tmp,"HMCB(4)%d no SC_PKG_LEAD_CODE 0xAA55, skip 1 byte.\n", nMCB);
                sz_szInfo = tmp;
                WriteLogFile(sz_szInfo);
            }
            if (m_nMissAA55Cnt[nMCB] > 65535 * 2)
            {
                sprintf(tmp,"HMCB(4)%d miss 0xAA55 more than %d times, exit parse.\n", nMCB, 65535 * 2);
                sz_szInfo = tmp;
                WriteLogFile(sz_szInfo);
                AtomicStoreL(&m_bPreDAQSucessFlag[nMCB], 0);
                SendMCBMsg(MCB_MSG_STATUS, MCB_DVSTS, ((long(nMCB)) << 16) + MCB_STSCD_MISSAA55_ERROR);
                //
                //ç”±è§£æçº¿ç¨‹è§¦å‘åœæ­¢
                if (AtomicLoadL(&m_bPreStop) == 0)
                {
                    //çœŸæ­£åœæ­¢ç”±å®šæ—¶çº¿ç¨‹å»åšï¼ˆè§£æçº¿ç¨‹ä¸é‡å…¥ Stopï¼‰
                    m_timeprestop = NucTool::GetTickCount();
                    AtomicStoreL(&m_bPreStop, 1);
                }
                return false;
            }

            local.Consume(1);
            consumed += 1;
            continue;
        }

        // 3c) ç±»å‹ / é•¿åº¦ä¸åŒ¹é…
        if (SC_pkg_type != DB_DAQ_PKG || SC_pkg_len != gbl_DBLine_Size[nMCB][SC_pkg_db_line])
        {
            s_nNotDataPktCnt[nMCB]++;

            if (SC_pkg_len != gbl_DBLine_Size[nMCB][SC_pkg_db_line])
            {
                //æ•°æ®é”™è¯¯åœºæ™¯ï¼Œæ¯ 1000 åŒ…å†™ä¸€æ¬¡æ—¥å¿—ï¼ˆé˜²æ­¢åˆ·çˆ†æ—¥å¿—å½±å“ä¼ è¾“ï¼‰
                static INT32U s_nLenErrCnt[MAX_MCBNUM] = {0};
                s_nLenErrCnt[nMCB]++;
                if (1 == (s_nLenErrCnt[nMCB] % 1000))
                {
                    sprintf(tmp,"HMCB(4)%d DB%d SC_pkg_len=%u, expected=%u (count=%u).\n",
                            nMCB, SC_pkg_db_line, SC_pkg_len,
                            (unsigned int)gbl_DBLine_Size[nMCB][SC_pkg_db_line], s_nLenErrCnt[nMCB]);
                    sz_szInfo = tmp;
                    WriteLogFile(sz_szInfo);
                }
            }

            INT32S extra = (INT32S)SC_pkg_len - (INT32S)gbl_DBLine_Size[nMCB][SC_pkg_db_line];

            // 3d) å¤š 1 å­—èŠ‚æ•°æ®å®¹é”™ï¼ˆDB å‰æˆ– CB å‰æ··å…¥ 1 å­—èŠ‚ï¼‰
            if (extra == 1)
            {
                BOOL bFixed = FALSE;
                const INT16U oneDBSz = (INT16U)(m_DMInfo[nMCB].nRowNum * m_DMInfo[nMCB].nColNum * m_nADBit / 8);
                // æƒ…å½¢ Aï¼šDB å­åŒ…å‰å¤š 1 å­—èŠ‚ï¼ˆä½ç½® 10ï¼‰
                if (avail >= 15) //è‡³å°‘èƒ½è®¿é—® pBuf[14]
                {
                    INT8U  DB_t2    = pBuf[11];
                    INT16U subSize  = pBuf[12] | ((INT16U)(pBuf[13] & 0x0F) << 8);
                    INT8U  boardIdx = pBuf[14];
                    if (boardIdx >= MAX_MCBADNUM)
                        boardIdx = MAX_MCBADNUM - 1;
                    if ((DB_t2 == DB_DAQ_PKG) && (subSize == oneDBSz) &&
                        (boardIdx < m_DMInfo[nMCB].nDBDMNUM[SC_pkg_db_line]))
                    {
                        sprintf(tmp,"HMCB(4)%d DB%d extra 1 byte before DB sub-packet, erase it.\n", nMCB, SC_pkg_db_line);
                        sz_szInfo = tmp;
                        WriteLogFile(sz_szInfo);
                        //
                        local.EraseAt(10);
                        local.At(3) = (unsigned char)((gbl_DBLine_Size[nMCB][SC_pkg_db_line] >> 16) & 0xFF);
                        local.At(4) = (unsigned char)((gbl_DBLine_Size[nMCB][SC_pkg_db_line] >>  8) & 0xFF);
                        local.At(5) = (unsigned char)( gbl_DBLine_Size[nMCB][SC_pkg_db_line]        & 0xFF);
                        bFixed = TRUE;
                    }
                }
                // æƒ…å½¢ Bï¼šCB å­åŒ…å‰å¤š 1 å­—èŠ‚ï¼ˆéœ€ä¿è¯ SC_pkg_len <= avail æ‰å®‰å…¨ï¼‰
                if (!bFixed && SC_pkg_len >= 20 && SC_pkg_len <= avail)
                {
                    INT8U  cb_t1   = pBuf[SC_pkg_len - 20];
                    INT8U  cb_t2   = pBuf[SC_pkg_len - 19];
                    INT16U cbSize  = pBuf[SC_pkg_len - 18] | ((INT16U)(pBuf[SC_pkg_len - 17] & 0x0F) << 8);
                    if (cb_t1 != CB_PKG && cb_t2 == CB_PKG && cbSize == 0x0C)
                    {
                        sprintf(tmp,"HMCB(4)%d DB%d extra 1 byte before CB sub-packet, erase it.\n", nMCB, SC_pkg_db_line);
                        sz_szInfo = tmp;
                        WriteLogFile(sz_szInfo);
                        //
                        local.EraseAt(SC_pkg_len - 20);
                        local.At(3) = (unsigned char)((gbl_DBLine_Size[nMCB][SC_pkg_db_line] >> 16) & 0xFF);
                        local.At(4) = (unsigned char)((gbl_DBLine_Size[nMCB][SC_pkg_db_line] >>  8) & 0xFF);
                        local.At(5) = (unsigned char)( gbl_DBLine_Size[nMCB][SC_pkg_db_line]        & 0xFF);
                        bFixed = TRUE;
                    }
                }
                if (bFixed)
                    continue;
            }
            // æœªè¯†åˆ«çš„é•¿åº¦é”™è¯¯ï¼Œè·³ 1 å­—èŠ‚
            local.Consume(1);
            consumed += 1;
            continue;
        }
        if (SC_pkg_len > avail)
        {
            //å½“å‰ SC åŒ…æœªæ”¶å…¨ï¼Œç­‰ä¸‹ä¸€æ¬¡äº‹ä»¶
            return true;
        }
        // ---------- 4) å¤´éƒ¨æœ‰æ•ˆï¼Œè§£æå„å­åŒ… ----------
        // åŒ…å¤´å…± 10 å­—èŠ‚ï¼Œå­åŒ…ä»åç§» 10 å¼€å§‹åˆ°åç§» SC_pkg_len ç»“æŸ
        size_t subOff     = 10;
        size_t subEndOff  = (size_t)SC_pkg_len;   //ä¸ SC åŒ…çš„ç»“æŸä½ç½®å¯¹é½
        //
        INT8U Max_DB_board_index = 0;
        INT8U overall_err        = 0;
        BOOL  bSubPackagesOK     = FALSE;

        while (subOff + 6 <= subEndOff)
        {
            if (subEndOff > avail)
            {
                //å½“å‰ SC åŒ…ä½™ä¸‹æ•°æ®é•¿åº¦è¶…é™ï¼Œç­‰æ•°æ®
                return true;
            }
            INT8U  xB_pkg_type    = pBuf[subOff];
            INT16U xB_pkg_size    = pBuf[subOff + 1];
            INT8U  high4          = pBuf[subOff + 2];
            xB_pkg_size          |= (INT16U)(high4 & 0x0F) << 8;
            INT8U  xB_pkg_index   = (high4 & 0xF0) >> 4;
            INT8U  xB_pkg_board_i = pBuf[subOff + 3];
            if (xB_pkg_board_i >= MAX_MCBADNUM) xB_pkg_board_i = MAX_MCBADNUM - 1;
            INT8U  xB_pkg_logic_err1 = pBuf[subOff + 4];
            INT8U  xB_pkg_logic_err2 = pBuf[subOff + 5];

            // ---- DB å­åŒ… ----
            if (xB_pkg_type == DB_ERR_PKG || xB_pkg_type == SC_pkg_type)
            {
                INT8U boardIdx = xB_pkg_board_i;
                pDBDataPackage.DB_pkg_type[boardIdx]   = xB_pkg_type;
                pDBDataPackage.DB_pkg_index[boardIdx]  = xB_pkg_index;
                pDBDataPackage.DB_board_index[boardIdx]= boardIdx;
                pDBDataPackage.DB_logic_err1[boardIdx] = xB_pkg_logic_err1;
                pDBDataPackage.DB_logic_err2[boardIdx] = xB_pkg_logic_err2;
                pDBDataPackage.DB_data_len[boardIdx]   = xB_pkg_size;
                if (Max_DB_board_index < boardIdx) Max_DB_board_index = boardIdx;
                pDBDataPackage.Max_DB_board = Max_DB_board_index + 1;
                //
                //é˜²è¶Šç•Œï¼šå®é™…å¯æ§é•¿åº¦ä½œä¸ºå†™å…¥ä¸Šé™ï¼Œç»ä¸æŒ‰ç†è®ºé•¿åº¦ç›´æ¥å†™
                const size_t maxDBDataSize = sizeof(pDBDataPackage.DB_data[boardIdx]);
                if ((size_t)xB_pkg_size > maxDBDataSize)
                {
                    sprintf(tmp,"HMCB(4)%d DB%d sub-packet size %u exceeds buffer %u, skip and resync.\n",
                            nMCB, SC_pkg_db_line, (unsigned int)xB_pkg_size, (unsigned int)maxDBDataSize);
                    sz_szInfo = tmp;
                    WriteLogFile(sz_szInfo);
                    //è·³å‡ºå­åŒ…å¾ªç¯ï¼ŒæŒ‰å­—èŠ‚é‡åŒæ­¥
                    break;
                }

                size_t dataStart = subOff + 6;
                size_t copyLen   = (size_t)xB_pkg_size;
                //avail æ˜¯ local çš„å¯ç”¨é•¿åº¦ï¼ŒsubEndOff æ˜¯å½“å‰ SC åŒ…çš„ç»“æŸåç§»
                if (dataStart + copyLen > subEndOff)
                {
                    //å­åŒ…è¶Šå‡ºå½“å‰ SC åŒ…è¾¹ç•Œï¼ŒåŒ…å¤´æŸå
                    break;
                }
                if (dataStart + copyLen > avail)
                {
                    //è¶Šç•Œè¯´æ˜æ•°æ®ä¸è¶³ï¼Œç­‰æ•°æ®
                    break;
                }
                //////////////////////////////////////////////////////////
                //DB å­åŒ…è´Ÿè½½é•¿åº¦ä¸æœŸæœ›å€¼æ¯”å¯¹
                const UINT nExpectDBLen = m_DMInfo[nMCB].nRowNum * m_DMInfo[nMCB].nColNum * m_nADBit / 8;
                if (xB_pkg_size != nExpectDBLen)
                {
                    sprintf(tmp,"HMCB(4)%d DB%d sub-packet size %u != expected %u, resync.\n",
                            nMCB, SC_pkg_db_line, (unsigned int)xB_pkg_size, nExpectDBLen);
                    sz_szInfo = tmp;
                    WriteLogFile(sz_szInfo);
                    bSubPackagesOK = FALSE;
                    break;
                }
                //////////////////////////////////////////////////////////
                memcpy(pDBDataPackage.DB_data[boardIdx], pBuf + dataStart, copyLen);

                size_t crcOff = dataStart + xB_pkg_size;
                if (crcOff >= avail) break;
                pDBDataPackage.DB_crc_code[boardIdx] = pBuf[crcOff];

                //æŒ‰å®é™…å€¼å†™å…¥æ•°æ®é•¿åº¦
                pDBDataPackage.DB_data_len[boardIdx] = (INT16U)xB_pkg_size;

                subOff = crcOff + 1;
                continue;
            }

            // ---- CB å­åŒ…ï¼ˆä¹‹å SC åŒ…ç»“æŸï¼‰ ----
            if (xB_pkg_type == CB_PKG)
            {
                pDBDataPackage.CB_pkg_type      = xB_pkg_type;
                pDBDataPackage.CB_pkg_index     = xB_pkg_index;
                pDBDataPackage.CB_board_index   = xB_pkg_board_i;
                pDBDataPackage.CB_data_len      = xB_pkg_size;
                pDBDataPackage.CB_logic_err1    = xB_pkg_logic_err1;
                pDBDataPackage.CB_logic_err2    = xB_pkg_logic_err2;
                //
                size_t dataStart = subOff + 6;
                //CB å­åŒ…è´Ÿè½½æŒ‰ CB_data å®é™…å¤§å°
                size_t copyLen = (size_t)xB_pkg_size;
                if (copyLen > sizeof(pDBDataPackage.CB_data))
                    copyLen = sizeof(pDBDataPackage.CB_data);
                //
                if (dataStart + copyLen > subEndOff)
                {
                    //å­åŒ…è¶Šå‡ºå½“å‰ SC åŒ…è¾¹ç•Œï¼ŒåŒ…å¤´æŸå
                    break;
                }
                if (dataStart + copyLen > avail)
                {
                    break;
                }
                memcpy(pDBDataPackage.CB_data, pBuf + dataStart, copyLen);
                //
                size_t crcOff = dataStart + xB_pkg_size;
                //
                if (crcOff >= avail)
                    break;
                pDBDataPackage.CB_crc_code = pBuf[crcOff];

                //å¡«å†™ä¸€ä¸ªå®Œæ•´ SC åŒ…çš„æè¿°
                pDBDataPackage.nMCB          = nMCB;
                pDBDataPackage.DB_line_i     = SC_pkg_db_line;
                pDBDataPackage.rx_DB_line_i  = SC_pkg_db_line;
                pDBDataPackage.SC_pkg_type   = SC_pkg_type;
                pDBDataPackage.SC_pkg_index  = SC_pkg_index;
                pDBDataPackage.overall_err   = overall_err;
                bSubPackagesOK = TRUE;
                break;
            }

            // ---- æœªçŸ¥å­åŒ…ç±»å‹ï¼šè·³å‡ºæŒ‰ 1 å­—èŠ‚é‡åŒæ­¥ ----
            break;
        }

        if (!bSubPackagesOK)
        {
            local.Consume(1);
            consumed += 1;
            continue;
        }

        // ---- å…³é”®æ”¹åŠ¨ï¼šåªè¦è§£å‡ºä¸€ä¸ªå®Œæ•´æœ‰æ•ˆåŒ…ï¼Œå°±åˆ·æ–°æ´»è·ƒæ—¶é—´ ----
        m_timehot[nMCB]     = NucTool::GetTickCount();
        m_timenodata[nMCB]  = NucTool::GetTickCount();
        //
        // ---------- 5) åŒ…ç»´æŠ¤ï¼ˆä¸æ—§ ETH_read_and_parse_dataEx_Thread_HMCB4 ä¸€è‡´ï¼‰ ----------
        // 5.1 åªæœ‰æœ€åä¸€ä¸ª dbLine çš„æ•°æ®å®Œæ•´ï¼Œæ‰ç»Ÿè®¡å·²è§£æçš„æ•°æ®åŒ…
        const UINT nMaxDBLindIndex = m_DMInfo[nMCB].nDBLineEnNum > 0 ? m_DMInfo[nMCB].nDBLineEnNum - 1 : 0;
        //
        if (nMaxDBLindIndex == (UINT)pDBDataPackage.DB_line_i)
        {
            parsed_pkg_n[nMCB]++;
            //
            if (1 == parsed_pkg_n[nMCB])
            {
                SC_1st_pkg_index[nMCB] = pDBDataPackage.SC_pkg_index;
                SC_1st_pkg_TriggCnt[nMCB] =
                    ((INT32U)pDBDataPackage.CB_data[ 8] << 24) |
                    ((INT32U)pDBDataPackage.CB_data[ 9] << 16) |
                    ((INT32U)pDBDataPackage.CB_data[10] <<  8) |
                    ((INT32U)pDBDataPackage.CB_data[11]);
            }

            // 16 ä½åŒ…å·æº¢å‡ºæ‹¼æ¥
            INT32U delta = (pDBDataPackage.SC_pkg_index + 65536u -
                            (SC_pre_pkg_index[nMCB] & 0xFFFFu)) % 65536u;
            SC_current_pkg_index[nMCB] += delta;
            //
            SC_pre_pkg_index[nMCB] = pDBDataPackage.SC_pkg_index;
            pDBDataPackage.SC_pkg_index = SC_current_pkg_index[nMCB] - SC_1st_pkg_index[nMCB] + 1;
            // åˆ¤å®šè§£æåçš„å‰ 6 ä¸ªé€šé“æ•°æ®ï¼Œè‹¥ä¸º 0xA5 åˆ™åˆ¤å®šä¸ºæœ¬æ¬¡é‡‡é›†çš„é¦–ä¸ªæ•°æ®åŒ…
            if (pDBDataPackage.DB_data_len[0] >= 6 &&
                pDBDataPackage.DB_data[0][0] == 0xA5 &&
                pDBDataPackage.DB_data[0][1] == 0xA5 &&
                pDBDataPackage.DB_data[0][2] == 0xA5 &&
                pDBDataPackage.DB_data[0][3] == 0xA5 &&
                pDBDataPackage.DB_data[0][4] == 0xA5 &&
                pDBDataPackage.DB_data[0][5] == 0xA5)
            {
                parsed_pkg_n[nMCB]    = 0;
                Update_pkg_n[nMCB]    = 0;
                SC_1st_pkg_index[nMCB]      = 0;
                SC_1st_pkg_TriggCnt[nMCB]   = 0;
                SC_pre_pkg_index[nMCB]      = 0;
                SC_current_pkg_index[nMCB]  = 0;

                // å…³é”®æ”¹åŠ¨ï¼šæ¸…é›¶è·¨æ¬¡é‡‡é›†çš„ 0xA5 ç¼ºå¤±è®¡æ•°
                m_nMiss0XA5PacketCnt[nMCB] = 0;
                //
                m_bGet0xA5Packet[nMCB] = true;
                //
                sprintf(tmp,"HMCB(4)%d DB%d got 0xA5 start packet.\n", nMCB, SC_pkg_db_line);
                sz_szInfo = tmp;
                WriteLogFile(sz_szInfo);
            }

            if (!m_bGet0xA5Packet[nMCB])
            {
                m_nMiss0XA5PacketCnt[nMCB]++;
                if (1 == (m_nMiss0XA5PacketCnt[nMCB] % 100))
                {
                    sprintf(tmp,"HMCB(4)%d DB0 got %d packets, no 0xA5 yet.\n",
                            nMCB, m_nMiss0XA5PacketCnt[nMCB]);
                    sz_szInfo = tmp;
                    WriteLogFile(sz_szInfo);
                }
            }
        }

        //æ˜¯å¦è¦æ±‚è·å–èµ·å§‹çš„ 0xA5 æ•°æ®åŒ…
        if (!m_bNeedGet0xA5)
            m_bGet0xA5Packet[nMCB] = true;

        // 5.2 è¶…è¿‡ INVALID_DATA_PACKET_NUM ä¸ªä¸¢å¼ƒåŒ…ä¹‹åæ‰å›è°ƒåº”ç”¨
        const UINT nDiscardThreshold = (UINT)INVALID_DATA_PACKET_NUM;
        BOOL bReadyToDispatch = m_bGet0xA5Packet[nMCB] &&
                                ((parsed_pkg_n[nMCB] + m_nDiscardNum[nMCB]) > nDiscardThreshold);
        if (bReadyToDispatch)
        {
            // ä»ç¬¬ 1 ä¸ªæœ‰æ•ˆæ•°æ®åŒ…å¼€å§‹
            pDBDataPackage.ADFormat_Bit = m_nADBit;
            pDBDataPackage.nSocketCounter = parsed_pkg_n[nMCB] - nDiscardThreshold + m_nDiscardNum[nMCB];
            if (pDBDataPackage.nSocketCounter < 1)
                pDBDataPackage.nSocketCounter = 1;
            pDBDataPackage.overall_err = overall_err;

            //åªæœ‰DBæœ‰æ•°æ®ä¸”ç¬¬ä¸€ä¸ªDBæ˜¯DAQæ•°æ®åŒ…ï¼Œæ‰å¾€åº”ç”¨ç¨‹åºå‘é€æ•°æ®æ›´æ–°æ¶ˆæ¯
            if ((pDBDataPackage.Max_DB_board > 0) && (pDBDataPackage.DB_pkg_type[0] == DB_DAQ_PKG) &&
                (pDBDataPackage.DB_data_len[0] > 0))
            {
                if (m_nLogLevel >= gLogLevel2)
                {
                    sprintf(tmp,"HMCB(4)%d DB%d dispatch SC_pkg#%d to App.\n",
                            nMCB, SC_pkg_db_line, pDBDataPackage.SC_pkg_index);
                    sz_szInfo = tmp;
                    WriteLogFile(sz_szInfo, gLogLevel2);
                }
                //ä¸»æ§åˆ¶ç›’å‰ 2 æ¬¡é‡‡é›†ï¼šé’©é“›ä¿¡å·å¼ºåˆ¶ç½® 0
                if ((nMCB == 0) && (Update_pkg_n[nMCB] < 2))
                {
                    pDBDataPackage.CB_data[2] = pDBDataPackage.CB_data[2] | 0x40;
                }

                UpdateOnePacketDataToApp_HMCB4(&pDBDataPackage);
                //ä¸€ä¸ªCBè§£æå®Œæœ€åä¸€ä¸ªDBæ•°æ®åï¼Œæ•°æ®åŒ…è£¹æ•°æ‰å¢1
                if (nMaxDBLindIndex == pDBDataPackage.DB_line_i)
                {
                    Update_pkg_n[nMCB]++;
                }
                ////////////////////////////////////////////////////////////////////////////////////////////////////
                //æ­£ç¡®è§£æä¸€åŒ…æ•°æ®åï¼Œm_nMissAA55Cnt è®¡æ•°æ¸…é›¶
                m_nMissAA55Cnt[nMCB] = 0;
                //æ•°æ®é‡‡é›†æ˜¯å¦æˆåŠŸå®Œæˆæ ‡å¿—
                AtomicStoreL(&m_bPreDAQSucessFlag[nMCB], 1);
            }
        }
        else
        {
            //èµ·å§‹é˜¶æ®µä¹Ÿè¦ç»´æŠ¤åŒ…å¤´ç¼“å­˜ï¼ˆé˜²æ­¢ m_MCBDataHeadTmp.scounter ä¸º 0ï¼‰
            pDBDataPackage.nSocketCounter = parsed_pkg_n[nMCB] - nDiscardThreshold + m_nDiscardNum[nMCB];
            if (pDBDataPackage.nSocketCounter < 1)
                pDBDataPackage.nSocketCounter = 1;
            // æ•°æ®åŒ…å¤´ç»´æŠ¤é€»è¾‘
            UINT nError_DI = ((~pDBDataPackage.CB_data[2]) & 0xF0) << 4;
            if (m_bGoudang_S4)
            {
                nError_DI = (nError_DI & 0x0B00) + ((nError_DI & 0x0200) << 1);
            }

            long nPulseCounter = 0;
            if (m_bHEorCXMode)
            {
                nPulseCounter = ((INT32U)pDBDataPackage.CB_data[ 8] << 24) |
                                ((INT32U)pDBDataPackage.CB_data[ 9] << 16) |
                                ((INT32U)pDBDataPackage.CB_data[10] <<  8) |
                                ((INT32U)pDBDataPackage.CB_data[11]);
                nPulseCounter = nPulseCounter - SC_1st_pkg_TriggCnt[nMCB] - nDiscardThreshold + m_nDiscardNum[nMCB] + 1;
            }
            else
            {
                nPulseCounter = pDBDataPackage.SC_pkg_index - nDiscardThreshold;
            }
            //
            m_MCBDataHeadTmp[nMCB].head_use.scounter   = pDBDataPackage.nSocketCounter;
            m_MCBDataHeadTmp[nMCB].head_use.pcounter.l = nPulseCounter;
            m_MCBDataHeadTmp[nMCB].head_use.accEnergy  = ((nError_DI & 0x0200) == 0) ? 0 : 1;
            //
            //é€‚é…ä¸‰èƒ½åŠ é€Ÿå™¨ï¼ˆDI(3)ä¸ºé«˜ï¼Œåˆ™ä¸º0.7MeVï¼›å¦åˆ™ä¸ºé«˜ä½èƒ½ï¼‰
            if (m_bThreeEnergyAcc_En && ((nError_DI & 0x0800) == 0))
                m_MCBDataHeadTmp[nMCB].head_use.accEnergy += 2;
            //
            // AIN0ï¼ˆ24bit åŸç è½¬æŒ‰ AD ä½å®½çš„åç§»äºŒè¿›åˆ¶ï¼‰
            m_MCBDataHeadTmp[nMCB].head_use.ain0.ul = ((INT32U)pDBDataPackage.CB_data[4] << 16) |
                                                      ((INT32U)pDBDataPackage.CB_data[5] <<  8) |
                                                      ((INT32U)pDBDataPackage.CB_data[6]);
            if (pDBDataPackage.CB_data[4] & 0x80)
                m_MCBDataHeadTmp[nMCB].head_use.ain0.ul &= 0x7FFFFF;
            else
                m_MCBDataHeadTmp[nMCB].head_use.ain0.ul += 0x800000;
            //
            if (m_nADBit < 24)
                m_MCBDataHeadTmp[nMCB].head_use.ain0.ul >>= (24 - m_nADBit);
            else
                m_MCBDataHeadTmp[nMCB].head_use.ain0.ul <<= (m_nADBit - 24);
            //
            m_MCBDataHeadTmp[nMCB].head_use.ptsmdata_h = ((INT32U)pDBDataPackage.CB_data[0] << 24) |
                                                          ((INT32U)pDBDataPackage.CB_data[1] << 16) |
                                                          ((INT32U)pDBDataPackage.CB_data[2] <<  8) |
                                                          ((INT32U)pDBDataPackage.CB_data[3]);
            m_MCBDataHeadTmp[nMCB].head_use.ptsmdata_l = nPulseCounter + nDiscardThreshold;
            m_MCBDataHeadTmp[nMCB].head_use.ain1.ul    = 0;
            m_MCBDataHeadTmp[nMCB].head_use.sscnt      = Update_pkg_n[nMCB];
            m_MCBDataHeadTmp[nMCB].head_use.regError_DI= nError_DI;
        }

        // ---------- 6) æ¶ˆè´¹æ‰æ•´ä¸ª SC åŒ… ----------
        local.Consume(SC_pkg_len);
        consumed += SC_pkg_len;
    }
    return true;
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
    //Èç¹ûÅäÖÃÄ£¿éµ÷Õû£¬Ôò°´HMCB¿ØÖÆºĞÄ¬ÈÏË³Ğò
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
	unsigned short nMaxDBBoardi = pDBDataPackage->Max_DB_board;//DBÊıÁ¿(1¸öÆğ)
	nMaxDBBoardi = (nMaxDBBoardi <= MAX_MCBADNUM) ? nMaxDBBoardi : MAX_MCBADNUM;
	//ÉÏ´«µÄÊı¾İµÍ×Ö½ÚÔÚÇ°
	INT8U ADFormat_Bit = pDBDataPackage->ADFormat_Bit;
	//
	unsigned long nADMChData1 = 0;
	unsigned long nADMChData2 = 0;
	unsigned char nTempCh[6] = { 0 };
	//Êı¾İµ÷Õû
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
            //ÊÇ·ñµ÷ÕûÄ£¿éÍ¨µÀË³Ğò
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
                    //2¸ö×Ö½ÚÊı¾İ±íÊ¾Ò»¸öÍ¨µÀÊı¾İ£¬ÇÒµÍ×Ö½ÚÔÚÇ°£¬¸ß×Ö½ÚÔÚºó
                    nADMChData1 = pDBDataPackage->DB_data[j][i*2] + (pDBDataPackage->DB_data[j][i*2+1]<<8);
                }
				//////////////////////////////////////////////////////////////////////
				//Êı¾İ½âÎö°´ÖÇÄÜ£¨=0£©»¹ÊÇ¸ßÄÜ£¨=1£©Ä£Ê½½âÎö´¦Àí
				if(m_bHEorCXMode)
				{
                    //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] + j][i/nRowNum][i%nRowNum] = nADMChData1;//Ò»´Î´¦ÀíÒ»¸öDBline´øµÄÒ»¸öÄ£¿éµÄÊı¾İ
                    //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] - j][i/nRowNum][i%nRowNum] = nADMChData1;//Ò»´Î´¦ÀíÒ»¸öDBline´øµÄÒ»¸öÄ£¿éµÄÊı¾İ
                    pnDBlineDetectorData[nMCB][nDBDMNumIndex][i/nRowNum][i%nRowNum] = nADMChData1;//Ò»´Î´¦ÀíÒ»¸öDBline´øµÄÒ»¸öÄ£¿éµÄÊı¾İ
                    //////////////////////////////////////////////////////////////////
					if((0 == i) && (j < DDataNUM))
					{
                        m_uDBParameter[nMCB][nDBDMNum[nDBLine] + j] = nADMChData1;//Ã¿¸ö¿ØÖÆºĞÄ£¿éµÄµÚÒ»¸öÍ¨µÀÊı¾İ
					}
				}
				else
				{
					//////////////////////////////////////////////////////////////////
					//for ÖÇÄÜ
					if((0 == i) && (j < DDataNUM))
					{
						m_uDBParameter[nMCB][j] = nADMChData1;//Ã¿¸öÄ£¿éµÄµÚÒ»¸öÍ¨µÀÊı¾İ
					}
					//µÚ¶ş¸öÊıÎª´¥·¢±àºÅ£¬ÍùºóÎªÍ¨µÀÊı¾İ
					else if(1 == i)
					{
						//nTriggerCnt = nADMChData1 + 1;//Ó²¼ş·µ»ØµÄ´¥·¢±àºÅ,Ó²¼ş´Ó0¿ªÊ¼¼ÆÊı£¬Èí¼ş´Ó1¿ªÊ¼¼ÆÊı£¬²¹²îÖµ1
					}
					else//(i>=2)
					{
						////long pnDBlineDetectorData[MAX_MCBNUM][MAX_MCBADNUM][MAX_DMCOLNUM][MAX_DMROWNUM];
                        //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] + j][(i-2)/nRowNum][(i-2)%nRowNum] = nADMChData1;//Ò»´Î´¦ÀíÒ»¸öDBline´øµÄÒ»¸öÄ£¿éµÄÊı¾İ
                        //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] - j][(i-2)/nRowNum][(i-2)%nRowNum] = nADMChData1;//Ò»´Î´¦ÀíÒ»¸öDBline´øµÄÒ»¸öÄ£¿éµÄÊı¾İ
                        pnDBlineDetectorData[nMCB][nDBDMNumIndex][(i-2)/nRowNum][(i-2)%nRowNum] = nADMChData1;//Ò»´Î´¦ÀíÒ»¸öDBline´øµÄÒ»¸öÄ£¿éµÄÊı¾İ
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
            //ÊÇ·ñµ÷ÕûÄ£¿éÍ¨µÀË³Ğò
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
                    //5¸ö×Ö½ÚÊı¾İ±íÊ¾2¸öÍ¨µÀÊı¾İ£¬ÇÒµÍ×Ö½ÚÔÚÇ°£¬¸ß×Ö½ÚÔÚºó
                    //20bit  0x54321 + 0x98765 ==> 54 32 19 87 65//5¸ö×Ö½Ú±íÊ¾2¸öÍ¨µÀµÄÊı¾İ
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
				//Êı¾İ½âÎö°´ÖÇÄÜ£¨=0£©»¹ÊÇ¸ßÄÜ£¨=1£©Ä£Ê½½âÎö´¦Àí
				if(m_bHEorCXMode)
				{
					//¸ßÄÜ
                    //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] + j][i*2/nRowNum][i*2%nRowNum] = nADMChData1;//Ò»´Î´¦ÀíÒ»¸öDBline´øµÄÒ»¸öÄ£¿éµÄÊı¾İ
                    //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] + j][(i*2+1)/nRowNum][(i*2+1)%nRowNum] = nADMChData2;//Ò»´Î´¦ÀíÒ»¸öDBline´øµÄÒ»¸öÄ£¿éµÄÊı¾İ
                    //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] - j][i*2/nRowNum][i*2%nRowNum] = nADMChData1;//Ò»´Î´¦ÀíÒ»¸öDBline´øµÄÒ»¸öÄ£¿éµÄÊı¾İ
                    //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] - j][(i*2+1)/nRowNum][(i*2+1)%nRowNum] = nADMChData2;//Ò»´Î´¦ÀíÒ»¸öDBline´øµÄÒ»¸öÄ£¿éµÄÊı¾İ
                    pnDBlineDetectorData[nMCB][nDBDMNumIndex][i*2/nRowNum][i*2%nRowNum] = nADMChData1;//Ò»´Î´¦ÀíÒ»¸öDBline´øµÄÒ»¸öÄ£¿éµÄÊı¾İ
                    pnDBlineDetectorData[nMCB][nDBDMNumIndex][(i*2+1)/nRowNum][(i*2+1)%nRowNum] = nADMChData2;//Ò»´Î´¦ÀíÒ»¸öDBline´øµÄÒ»¸öÄ£¿éµÄÊı¾İ
                    ////////////////////////////////////////////////////////////////
					if((0 == i) && (j < DDataNUM))
					{
						m_uDBParameter[nMCB][nDBDMNum[nDBLine] + j] = nADMChData1;
					}
				}
				else
				{
					////////////////////////////////////////////////////////////////
					//ÖÇÄÜ
					//µÚÒ»¸öÊıÎªÅäÖÃ²ÎÊı
					//µÚ¶ş¸öÊıÎª´¥·¢±àºÅ£¬ÍùºóÎªÍ¨µÀÊı¾İ
					if(0 == i)
					{
						if(j < DDataNUM)
						{
							m_uDBParameter[nMCB][j] = nADMChData1;
						}
						//nTriggerCnt = nADMChData2 + 1;//Ó²¼ş·µ»ØµÄ´¥·¢±àºÅ,Ó²¼ş´Ó0¿ªÊ¼¼ÆÊı£¬Èí¼ş´Ó1¿ªÊ¼¼ÆÊı£¬²¹²îÖµ1
					}
					else//(i>=1)
					{
                        //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] + j][(i-1)*2/nRowNum][(i-1)*2%nRowNum] = nADMChData1;//Ò»´Î´¦ÀíÒ»¸öDBline´øµÄÒ»¸öÄ£¿éµÄÊı¾İ
                        //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] + j][((i-1)*2+1)/nRowNum][((i-1)*2+1)%nRowNum] = nADMChData2;//Ò»´Î´¦ÀíÒ»¸öDBline´øµÄÒ»¸öÄ£¿éµÄÊı¾İ
                        //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] - j][(i-1)*2/nRowNum][(i-1)*2%nRowNum] = nADMChData1;//Ò»´Î´¦ÀíÒ»¸öDBline´øµÄÒ»¸öÄ£¿éµÄÊı¾İ
                        //pnDBlineDetectorData[nMCB][nDBDMNum[nDBLine] - j][((i-1)*2+1)/nRowNum][((i-1)*2+1)%nRowNum] = nADMChData2;//Ò»´Î´¦ÀíÒ»¸öDBline´øµÄÒ»¸öÄ£¿éµÄÊı¾İ
                        pnDBlineDetectorData[nMCB][nDBDMNumIndex][(i-1)*2/nRowNum][(i-1)*2%nRowNum] = nADMChData1;//Ò»´Î´¦ÀíÒ»¸öDBline´øµÄÒ»¸öÄ£¿éµÄÊı¾İ
                        pnDBlineDetectorData[nMCB][nDBDMNumIndex][((i-1)*2+1)/nRowNum][((i-1)*2+1)%nRowNum] = nADMChData2;//Ò»´Î´¦ÀíÒ»¸öDBline´øµÄÒ»¸öÄ£¿éµÄÊı¾İ
					} 
				}
			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////
	//2025.09.17 for Debug
	if( 1 == m_bDebug)
	{
		//´òÓ¡Ç°5´Î²É¼¯µÄÇ°¼¸¸öÍ¨µÀÊı¾İ
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
	//Èç¹ûËùÓĞdblineÒÑ¾­È«²¿¸üĞÂ
	if(nDBLineEnNum > (nDBLine + 1) )
	{
		return;
	}
	///////////////////////////////////////////////
	//¿½±´Êı¾İ×´Ì¬µ½m_pOneDBLineOnePackageStatue
	m_pOneDBLineOnePackageStatue[nMCB].nMCB = pDBDataPackage->nMCB;
	m_pOneDBLineOnePackageStatue[nMCB].ADFormat_Bit = pDBDataPackage->ADFormat_Bit;//AD×ª»»Î»Êı

	m_pOneDBLineOnePackageStatue[nMCB].nSocketCounter = pDBDataPackage->nSocketCounter;//Èí¼ş°ü¹ü¼ÆÊı;
	m_pOneDBLineOnePackageStatue[nMCB].overall_err = pDBDataPackage->overall_err;	//×Ü´íÎó
	//
	m_pOneDBLineOnePackageStatue[nMCB].DB_line_i = pDBDataPackage->DB_line_i;//DB_lineË÷Òı(0~3)
	m_pOneDBLineOnePackageStatue[nMCB].DB_line_num = pDBDataPackage->DB_line_num;//ÓĞĞ§db_line(1,2,4)
	m_pOneDBLineOnePackageStatue[nMCB].cmd_code = pDBDataPackage->cmd_code;//ÊÇ·ñ Check the package type
	m_pOneDBLineOnePackageStatue[nMCB].cmd_mismatched_pkg_skip_en = pDBDataPackage->cmd_mismatched_pkg_skip_en;//1 = Èç¹ûSC°üÍ·µÄcmd²»·û£¬»áÌø¹ı±¾Êı¾İ°ü¼ì²éÏÂÒ»¸öÊı¾İ°ü£¬Ö±µ½·¢ÏÖcmdÆ¥ÅäµÄÊı¾İ°ü£¬²¢Íê³ÉÊı¾İ°ü½âÎö
	//
	m_pOneDBLineOnePackageStatue[nMCB].SC_pkg_header = pDBDataPackage->SC_pkg_header;  //¶¨Î»DWord £¨0x00FF55AA£©
	m_pOneDBLineOnePackageStatue[nMCB].rx_DB_line_i = pDBDataPackage->rx_DB_line_i;	//µ±Ç°Ëù½ÓÊÕµ½Êı¾İµÄDB_lineË÷Òı(0~3)
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
	//16¸öCBÊı¾İ
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
	//ÎŞ´¥·¢¼ÆÊ±Æ÷ÇåÁã
	//m_nWaitDataTrigger[nMCB] = 0;
	////////////////////////////////////////////////////////////////////////////////////////////////
	//Êı¾İµ÷Õû
	UINT nDMNum = m_DMInfo[nMCB].nDMNum;     //Detector Module number of the MCB
	////////////////////////////////////////////////////////////////////////////////////////////////
	//Ä³Ò»¸öMCB(DBLine)ÉÏËùÓĞÄ£¿éµÄÊı¾İ´¦ÀíÍê³É£¬ÔòÍùÈí¼ş·¢Ò»´ÎMCB_MSG_DATAÏûÏ¢
	//Ì½²âÆ÷Êı¾İ£¬Ã¿¸öÍ¨µÀÕ¼4 ¸ö×Ö½Ú¡£Ì½²âÆ÷Êı¾İÅÅÁĞË³ĞòÎª£º
	//ËùÓĞÌ½²âÆ÷Ä£¿é£¨´Ó1 µ½DMINFO.nDMNum£©µÚÒ»ÁĞÌ½²âÆ÷£¨¹²DMINFO.nAllChnNum/DMINFO.nColNum£©µÄÊı¾İ
	//ËùÓĞÌ½²âÆ÷Ä£¿éµÚ¶şÁĞÌ½²âÆ÷µÄÊı¾İ¡­
	//ËùÓĞÌ½²âÆ÷Ä£¿éµÚDMINFO.nColNum ÁĞÌ½²âÆ÷µÄÊı¾İ
	UINT nCh = 0;
	// é€šé“æ’å¸ƒï¼šæ¨¡å—ï¼ˆADM/DM æ§½ï¼‰ä¼˜å…ˆâ€”â€”æ¯æ¨¡å—æ§½å†…"åˆ—Ã—è¡Œ"é€šé“è¿ç»­ã€‚
	// å‘ç”Ÿå™¨/å®è£…æ‹“æ‰‘ä¸‹ä¸€ä¸ªæ¨¡å—æ§½æ‰¿è½½å®Œæ•´ nColNum*nRowNum ä¸ªé€šé“
	// ï¼ˆçœŸæœº CX1664DE æ•°æ®å‘ç”Ÿå™¨éªŒè¯ï¼šæ¯æ§½ç«–è¯» 0..nColNum*nRowNum-1 è¿ç»­ï¼‰ã€‚
	// åŸåˆ—ä¼˜å…ˆæ’å¸ƒä¼šæŠŠåŒä¸€é€»è¾‘é€šé“çš„é‡å¤é•œåƒé“ºæ»¡çº¿æ€§æµï¼Œç½‘æ ¼/ä¿å­˜å‡æ— æ³•å‘ˆç°æ¨¡å—è§†å›¾ã€‚
	for(j = 0 ; j < nDMNum; j++)
	{
		for(i = 0; i < nColNum; i++)
		{
			for(k = 0; k < nRowNum; k++)
			{
				nCh = j*nColNum*nRowNum + i*nRowNum + k;
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
				//For ÎïÆ·»ú×¨ÓÃ: Èç¹ûÅäÖÃÁËm_nAppADBit²ÎÊı£¬ÇÒm_nAppADBit<m_nADBit£¬ÔòÈÏÎªÊÇÓÃÓÚÎïÆ·»ú£¬PÖµ×î´óÖµÎª(65535)
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
		//´òÓ¡Ç°5´Î²É¼¯µÄÇ°¼¸¸öÍ¨µÀÊı¾İ
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
	//HMCBÄ¬ÈÏÀë¿ØÖÆºĞ×îÔ¶µÄÌ½²âÆ÷Ä£¿éÎªµÚÒ»¸öÌ½²âÆ÷Ä£¿é£¬ÓëMCB-H¶¨ÒåÏà·´£¬Ä¬ÈÏ¶ÔËùÓĞHMCB»òDB-Line°´3-MODULE·½°¸µ÷ÕûÍ¨µÀË³Ğò¡£
//	memcpy(m_MCBDataTmp[nIndex].addata, m_MCBData[nIndex].addata, m_DMInfo[nIndex].nAllChnNum *sizeof(UINT));
//	for (i =0; i<int(m_DMInfo[nIndex].nAllChnNum); i++)
//	{
//		m_MCBData[nIndex].addata[i].ul = m_MCBDataTmp[nIndex].addata[i/(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)*m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum
//			+((m_DMInfo[nIndex].nDMNum-1)-(i/m_DMInfo[nIndex].nRowNum)%m_DMInfo[nIndex].nDMNum)*m_DMInfo[nIndex].nRowNum +i%m_DMInfo[nIndex].nRowNum].ul;
//	}
	////////////////////////////////////////////////////////////////////////
	//channel order adjust for all MCB(DB Line)
	for (j =0; j< MAX_CHNADJNUM; j++)
	{
		if (m_nAdjChn[j])  //channel order adjust
		{
			memcpy(m_MCBDataTmp[nIndex].addata, m_MCBData[nIndex].addata, m_DMInfo[nIndex].nAllChnNum *sizeof(UINT));
			switch(m_nAdjChn[j])  //adjust mode: 0-NONE, 1-ROW, 2-COLUMN, 3-MODULE, 4-ALL_ROW
			{
			case 1:
				for (i =0; i<int(m_DMInfo[nIndex].nAllChnNum); i++)
				{
					m_MCBData[nIndex].addata[i].ul = m_MCBDataTmp[nIndex].addata[i/m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nRowNum+
						m_DMInfo[nIndex].nRowNum-1-i%m_DMInfo[nIndex].nRowNum].ul;
				}
				break;
			case 2:
				for (i =0; i<int(m_DMInfo[nIndex].nAllChnNum); i++)
				{
					m_MCBData[nIndex].addata[i].ul = m_MCBDataTmp[nIndex].addata[m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum*(m_DMInfo[nIndex].nColNum-1-(i/(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)))
						+i%(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)].ul;
				}
				break;
			case 3:
				for (i =0; i<int(m_DMInfo[nIndex].nAllChnNum); i++)
				{
					m_MCBData[nIndex].addata[i].ul = m_MCBDataTmp[nIndex].addata[i/(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)*m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum 
						+((m_DMInfo[nIndex].nDMNum-1)-(i/m_DMInfo[nIndex].nRowNum)%m_DMInfo[nIndex].nDMNum)*m_DMInfo[nIndex].nRowNum +i%m_DMInfo[nIndex].nRowNum].ul;
				}
				break;
			case 4:
				for (i =0; i<int(m_DMInfo[nIndex].nAllChnNum); i++)
				{
					m_MCBData[nIndex].addata[i].ul = m_MCBDataTmp[nIndex].addata[i/(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)*(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)+
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
			memcpy(m_MCBDataTmp[nIndex].addata, m_MCBData[nIndex].addata, m_DMInfo[nIndex].nAllChnNum *sizeof(UINT));
			switch(m_nMCBxAdjChn[nIndex][j])  //adjust mode: 0-NONE, 1-ROW, 2-COLUMN, 3-MODULE, 4-ALL_ROW
			{
			case 1:
				for (i =0; i<int(m_DMInfo[nIndex].nAllChnNum); i++)
				{
					m_MCBData[nIndex].addata[i].ul = m_MCBDataTmp[nIndex].addata[i/m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nRowNum+
						m_DMInfo[nIndex].nRowNum-1-i%m_DMInfo[nIndex].nRowNum].ul;
				}
				break;
			case 2:
				for (i =0; i<int(m_DMInfo[nIndex].nAllChnNum); i++)
				{
					m_MCBData[nIndex].addata[i].ul = m_MCBDataTmp[nIndex].addata[m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum*(m_DMInfo[nIndex].nColNum-1-(i/(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)))
						+i%(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)].ul;
				}
				break;
			case 3:
				for (i =0; i<int(m_DMInfo[nIndex].nAllChnNum); i++)
				{
					m_MCBData[nIndex].addata[i].ul = m_MCBDataTmp[nIndex].addata[i/(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)*m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum 
						+((m_DMInfo[nIndex].nDMNum-1)-(i/m_DMInfo[nIndex].nRowNum)%m_DMInfo[nIndex].nDMNum)*m_DMInfo[nIndex].nRowNum +i%m_DMInfo[nIndex].nRowNum].ul;
				}
				break;
			case 4:
				for (i =0; i<int(m_DMInfo[nIndex].nAllChnNum); i++)
				{
					m_MCBData[nIndex].addata[i].ul = m_MCBDataTmp[nIndex].addata[i/(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)*(m_DMInfo[nIndex].nRowNum*m_DMInfo[nIndex].nDMNum)+
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
	//¶àÁĞºÏÒ»£¬¶àÁĞºÏ²¢ÇóËãÊõ¾ùÖµ
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
		memcpy(m_MCBDataTmp[nIndex].addata, m_MCBData[nIndex].addata, m_DMInfo[nIndex].nAllChnNum *sizeof(UINT));
		for (j = 0; j < MAX_SECTIONADJNUM; j++)
		{
			if((m_nSectionFirstADNo[j] > 0) && (m_nSectionLastADNo[j] > 0) && (m_nSectionLastADNo[j] >= m_nSectionFirstADNo[j]))
			{
				nSectionADNum = m_nSectionLastADNo[j] - m_nSectionFirstADNo[j] + 1;
				nSectionRowNum = nSectionADNum*m_DMInfo[nIndex].nRowNum;
				//Ã¿ÅÅ¶¼ĞèÒªµ÷Õû
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
						m_MCBData[nIndex].addata[nPreSectionEndRowNum + i].ul = m_MCBDataTmp[nIndex].addata[nSectionFirstRowNo+i].ul;
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
	////ÏÈ¸ßÄÜ ºóµÍÄÜ£¨ÏÈµÚÒ»ÅÅ ÔÙµÚ¶şÅÅ£©//
	//for (i =0; i < int(m_DMInfo[nIndex].nAllChnNum/2); i++)
	//{
	//	nDMy = i / nDMInfo_nHalfChannel;
	//	nPGz = (i % nDMInfo_nHalfChannel) / 16;
	//	m_MCBData[nIndex].addata[i].ul = m_MCBData[nIndex].addata[i].ul*m_fMCBxDMyPGz[nIndex][nDMy][nPGz];
	//	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	//for ÎïÆ·»ú16Î»×¨ÓÃ
	//	if (m_MCBData[nIndex].addata[i].ul > UINT((1<<16) -1))
	//	{
	//		m_MCBData[nIndex].addata[i].ul = UINT((1<<16) -1);
	//	}
	//	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	//}
	//	////ºóµÍÄÜ
	//	//for (i =0; i < int(m_DMInfo[nIndex].nAllChnNum/2); i++)
	//	//{
	//	//	nDMy = i / nDMInfo_nHalfChannel;
	//	//	nPGz = nDMInfo_nHalfChannel / 16 + (i % nDMInfo_nHalfChannel) / 16;
	//	nPGz += nDMInfo_nHalfChannel / 16;
	//	m_MCBData[nIndex].addata[m_DMInfo[nIndex].nAllChnNum/2 + i].ul = m_MCBData[nIndex].addata[m_DMInfo[nIndex].nAllChnNum/2 + i].ul*m_fMCBxDMyPGz[nIndex][nDMy][nPGz];
	//	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	//for ÎïÆ·»ú16Î»×¨ÓÃ
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
		memcpy(m_MCBDataTmp[nIndex].addata, m_MCBData[nIndex].addata, m_DMInfo[nIndex].nAllChnNum *sizeof(UINT));
		for (j = 0; j < MAX_SECTIONADJNUM; j++)
		{
			if((m_nMCBxSectionFirstADNo[nIndex][j] > 0) && (m_nMCBxSectionLastADNo[nIndex][j] > 0) && (m_nMCBxSectionLastADNo[nIndex][j] >= m_nMCBxSectionFirstADNo[nIndex][j]))
			{
				nSectionADNum = m_nMCBxSectionLastADNo[nIndex][j] - m_nMCBxSectionFirstADNo[nIndex][j] + 1;
				nSectionRowNum = nSectionADNum*m_DMInfo[nIndex].nRowNum;
				//Ã¿ÅÅ¶¼ĞèÒªµ÷Õû
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
						m_MCBData[nIndex].addata[nPreSectionEndRowNum + i].ul = m_MCBDataTmp[nIndex].addata[nSectionFirstRowNo+i].ul;
					}
				}
			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT nError_DI = ((~pDBDataPackage->CB_data[2])& 0xF0)<<4;//°´Î»È¡·´;//Data 2£º¸ß4Î»ÓÃÓÚ´«ÊäDDI
    if(m_bGoudang_S4)
    {
        nError_DI = (nError_DI & 0x0B00) + ((nError_DI&0x0200)<< 1);//DI2ÇåÁãºó£¬°ÑDI1µÄÖµ¸³¸øDI2
    }

	///////////////////////////////////////////////////////////////////
    //¿ØÖÆ°å·µ»ØµÄÓ²¼ş´¥·¢Êı
	long nPulseCounter = 0;
	if(m_bHEorCXMode)
	{
		//¸ßÄÜ
		nPulseCounter = (pDBDataPackage->CB_data[8]<<24)+(pDBDataPackage->CB_data[9]<<16)+(pDBDataPackage->CB_data[10]<<8)+(pDBDataPackage->CB_data[11]);
		nPulseCounter = nPulseCounter - SC_1st_pkg_TriggCnt[nIndex] - INVALID_DATA_PACKET_NUM + m_nDiscardNum[nMCB] + 1;
	}
	else
	{
		//ÖÇÄÜ
		nPulseCounter = pDBDataPackage->SC_pkg_index;
		nPulseCounter = nPulseCounter - INVALID_DATA_PACKET_NUM;
	}
	//
	m_MCBData[nMCB].d_head.head_use.scounter = pDBDataPackage->nSocketCounter;//nSocketCounter;	// socket packet serial number, self-add//Èí¼ş°ü¹ü¼ÆÊı,1¿ªÊ¼
	//Ó²¼şĞòÁĞºÅÈ¡ÖµÄ£Ê½
	//m_MCBData[nMCB].d_head.head_use.pcounter.l = nPulseCounter;//nPulseCounter;	// pulse trigger number//¿ØÖÆ°å·µ»ØµÄÓ²¼ş´¥·¢Êı
	//Ä¬ÈÏÎª1£¬È¡Êµ¼Ê·µ»Ø°ü¹üÊıĞòÁĞºÅ×öÓ²¼şĞòÁĞºÅ
	//¸ÄÎª0£¬È¡Èí¼ş°üĞòÁĞºÅ
	//£¨ÁÙÊ±½â¾ö´æÔÚÓ²¼şĞòÁĞºÅÓĞÖØ¸´µÄÏÖÏó£¨»»¿ØÖÆºĞÈÔ¾É´æÔÚ£©£¬µ¼ÖÂ¶à¸öºĞ¼¶ÁªÊ±´æÔÚÊı¾İ¶Ô²»ÆëµÄÎÊÌâ£©£¨Èí¼ş¶Ë°´»Øµ÷Êı¾İÀïÃæµÄÓ²¼ş´¥·¢Êı´æÍ¼£©
	if(1 == m_bHWSerialNumberMode)
	{
		m_MCBData[nMCB].d_head.head_use.pcounter.l = nPulseCounter;//nPulseCounter;	// pulse trigger number//¿ØÖÆ°å·µ»ØµÄÓ²¼ş´¥·¢Êı
	}
	else
	{
		m_MCBData[nMCB].d_head.head_use.pcounter.l = pDBDataPackage->nSocketCounter;//nPulseCounter;	// pulse trigger number//¿ØÖÆ°å·µ»ØµÄÓ²¼ş´¥·¢Êı
	}
	//m_MCBData[nMCB].d_head.head_use.accEnergy = ((nError_DI&0x0200) ==0) ? 1 : 0;// acc energy sign
	/////////////////////////////////////////////////////////////////
	//m_MCBData[nMCB].d_head.head_use.accEnergy = ((nError_DI&0x0200) ==0) ? 1 : 0;// acc energy sign
	//2024.11.04¸ù¾İµ÷ÊÔ·´À¡£¬¸ßÄÜÊ±¸øÈí¼şÄÜÁ¿Ö¸Ê¾ĞÅºÅÎª0£¬Õı³£Ó¦¸ÃÎª1£¬¹Ê¸ÄÎªÏÂÃæÂß¼­
	m_MCBData[nMCB].d_head.head_use.accEnergy = ((nError_DI&0x0200) ==0) ? 0 : 1;// acc energy sign
	///////////////////////////////////////////////////////////////////
	UINT nDI3 = ((nError_DI&0x0800) == 0) ? 0 : 1;// acc energy sign
	UINT nDI1 = ((nError_DI&0x0200) == 0) ? 0 : 1;// acc energy sign
	//ÊÊÅäÈıÄÜ¼ÓËÙÆ÷£¨DI(3)Îª¸ß£¬ÔòÎª0.7MeV£¬·ñÔòÎª¸ßµÍÄÜ£©
	if(m_bThreeEnergyAcc_En)
	{
		if((0 == nDI3) && (0 == nDI1)) //00³¬µÍ
		{
			m_MCBData[nMCB].d_head.head_use.accEnergy = 2;
		}
		else if((0 == nDI3) && (1 == nDI1)) //01³¬µÍ
		{
			m_MCBData[nMCB].d_head.head_use.accEnergy = 2;
		}
		else if((1 == nDI3) && (0 == nDI1))//10µÍÄÜ
		{
			m_MCBData[nMCB].d_head.head_use.accEnergy = 0;
		}
		else//11¸ßÄÜ
		{
			m_MCBData[nMCB].d_head.head_use.accEnergy = 1;
		}
	}
	///////////////////////////////////////////////////////////////////
	std::string sz_szInfoCBData = ("");
	char tmpCBData[512] ={0};
	//¿ØÖÆ°å·µ»ØµÄÓ²¼ş´¥·¢Êı
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
	//CBÊı¾İ°ü¹Ì¶¨°üº¬12¸ö×Ö½ÚµÄÊı¾İ,12¸ö×Ö½Ú¡£
	//Data 0£¨DB line¶ÀÁ¢£©£º¸ß4Î»´«ÊäSDCLK²É¼¯´íÎóÀàĞÍ£¬µÍ4Î»´«ÊäSDCLK²É¼¯¿í¶È¡£
	//Data 1£¨DB line¶ÀÁ¢£©£ºÏµÍ³Éè¼Æ±£Áô¡£
	//Data 2£º¸ß4Î»ÒÑÓÃÓÚ´«ÊäDDI£»µÍ4Î»±£Áô£¬È«Îª0¡£ÏµÍ³±£Áô¡£
	//Data 3£ºÏµÍ³Éè¼Æ±£Áô¡£
	//Data 4¡«11: Áô¸øÓÃ»§Ê¹ÓÃ¡£
	//m_MCBData[nMCB].d_head.head_use.ain0.ul =		(pDBDataPackage->CB_data[4]<<24)+(pDBDataPackage->CB_data[5]<<16)+(pDBDataPackage->CB_data[6]<<8)+(pDBDataPackage->CB_data[7]);// 24bit ADCÖµ(È¡¸ß24bit + 8Bit (0x07))
	m_MCBData[nMCB].d_head.head_use.ain0.ul =		(pDBDataPackage->CB_data[4]<<16)+(pDBDataPackage->CB_data[5]<<8)+(pDBDataPackage->CB_data[6]);//24bit ADCÖµ(È¡¸ß24bit + 8Bit (0x07))
	//
	if(pDBDataPackage->CB_data[4] & 0x80)
	{
		//×î¸ßÎ»·ûºÅÎ»Îª1£¬ÔòÖ±½ÓÈ¡³ı·ûºÅÎ»ÍâµÄÊı¾İ
		m_MCBData[nMCB].d_head.head_use.ain0.ul &= 0x7FFFFF;
		//
	}
	else
	{
		//×î¸ßÎ»·ûºÅÎ»Îª0£¬ÔòÔö¼Ó0x800000
		m_MCBData[nMCB].d_head.head_use.ain0.ul += 0x800000;
	}
	//ÊÊÅäm_nADBit
	if(m_nADBit < 24)
	{
		m_MCBData[nMCB].d_head.head_use.ain0.ul = (m_MCBData[nMCB].d_head.head_use.ain0.ul)>>(24 - m_nADBit);
	}
	else
	{
		m_MCBData[nMCB].d_head.head_use.ain0.ul = (m_MCBData[nMCB].d_head.head_use.ain0.ul)<<(m_nADBit - 24);
	}
	//
	m_MCBData[nMCB].d_head.head_use.ptsmdata_h =	(pDBDataPackage->CB_data[0]<<24)+(pDBDataPackage->CB_data[1]<<16)+(pDBDataPackage->CB_data[2]<<8)+(pDBDataPackage->CB_data[3]);// ×Ö½Ú0¡«3£ºÏµÍ³±£Áô
	//m_MCBData[nMCB].d_head.head_use.ptsmdata_l =	(pDBDataPackage->CB_data[8]<<24)+(pDBDataPackage->CB_data[9]<<16)+(pDBDataPackage->CB_data[10]<<8)+(pDBDataPackage->CB_data[11]);// CountPos(32bit)
	m_MCBData[nMCB].d_head.head_use.ptsmdata_l = nPulseCounter + INVALID_DATA_PACKET_NUM;
	//
	m_MCBData[nMCB].d_head.head_use.ain1.ul = 0;				// ain1 data(UINT)
	m_MCBData[nMCB].d_head.head_use.sscnt = Update_pkg_n[nMCB];	//nPulseCounter + INVALID_DATA_PACKET_NUM;		// socket packet serial number, from packet
	m_MCBData[nMCB].d_head.head_use.regError_DI = nError_DI;
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//µ÷ÕûÌ½²âÆ÷Ä£¿éÊı¾İÓë¿ØÖÆºĞAin¡¢Di Êı¾İ´íÎ»1 ¸ö´¥·¢µÄÎÊÌâ
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
	//2023.06.16 ¸üĞÂ×îĞÂÊÕµ½Êı¾İµÄ°ü¹üĞòºÅ
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
                    //¿½±´Ç°´ÎÊı¾İ±¸·İÊı¾İ
                    memcpy(&m_MCBDataTmp, &m_MCBDataBackup[nMCB], sizeof(MCBData));//¿½±´Ç°´ÎÊı¾İ±¸·İ
                    //copy current packet head to m_MCBDataTmp????????????????
                    memcpy(&m_MCBDataTmp[nIndex].d_head, &m_MCBData[nMCB].d_head, sizeof(MCBData_H));   //temporarily save current MCBData_H
                    ////////////////////////////////////////
                    //??????????????????????????
                    m_MCBDataTmp[nIndex].d_head.head_use.scounter = m_MCBDataTmp[nIndex].d_head.head_use.scounter + i;//nSocketCounter;	// socket packet serial number, self-add//????????????,1????
                    m_MCBDataTmp[nIndex].d_head.head_use.pcounter.l = m_lPrePulseCnt[nMCB] + 1 + i;//nPulseCounter;	// pulse trigger number//??????????????????????
                    m_MCBDataTmp[nIndex].d_head.head_use.accEnergy = -1;//??????????????????
                    ////////////////////////////////////////
                    parsed_pkg_n[nMCB]++;//????????????????????????????1
                    //????????????????
					////////////////////////////////////////
					//2026.03.19Ôö¼Ó¶ÔCS0200MµÄÖ§³Ö
					//1¡ª¡ªCS0200M£¬ µ÷ÕûÖ÷¿ØÖÆºĞµÄm_MCBData[nIndex].d_head.head_use.accEnergyÎª³öÊø¹â»úµÄ±àºÅ( m_MCBData[nIndex].d_head.head_use.accEnergy = (long)l_angle);
					//³öÊø¹â»úµÄ±àºÅ¹æÔò£º¹â»ú´¥·¢ĞÅºÅ½ÓDI-0£¬¹â»úÑ­»·ĞÅºÅ½ÓDI-1£¬DI-1Îª1Ê±µÄ¹â»ú±àºÅÎª0£¬DI-1Îª0ºóÃ¿Ò»´Î´¥·¢µÄ¹â»ú±àºÅ+1
					//ÉäÏßÔ´ĞòºÅ
					if( 0 == nMCB)
					{
						if(1 == m_nDeviceTypeID)
						{	
							if(1 == m_MCBDataTmp[nIndex].d_head.head_use.accEnergy)
							{
								m_nXRayGenN = 0;
							}
							else
							{
								m_nXRayGenN++;
							}
							m_MCBDataTmp[nIndex].d_head.head_use.accEnergy = (LONG)m_nXRayGenN;
						}
					}
					////////////////////////////////////////
                    gCallBack(MCB_MSG_DATA, WPARAM(&m_MCBDataTmp[nMCB]), nMCB);
                    //
                    memset(tmp,0,512);
                    sprintf(tmp,"HMCB%d ZeroPadding one frame data at %d(socket)/%d(pluse).\n", nMCB, m_MCBDataTmp[nIndex].d_head.head_use.scounter, m_MCBDataTmp[nIndex].d_head.head_use.pcounter.l);
                    sz_szInfo = tmp;
                    WriteLogFile(sz_szInfo);
                    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                    if(1 == ((m_MCBDataTmp[nIndex].d_head.head_use.scounter) % 100))
                    {
                        memset(tmp,0,512);
                        sprintf(tmp,"HMCB%d software socket data counts: %d, hardware pluse counts: %d, differ:%d\n",
                                         nMCB, m_MCBDataTmp[nIndex].d_head.head_use.scounter, m_MCBDataTmp[nIndex].d_head.head_use.pcounter.l,
                                m_MCBDataTmp[nIndex].d_head.head_use.pcounter.l - m_MCBDataTmp[nIndex].d_head.head_use.scounter);
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
	//2026.03.19Ôö¼Ó¶ÔCS0200MµÄÖ§³Ö
	//1¡ª¡ªCS0200M£¬ µ÷ÕûÖ÷¿ØÖÆºĞµÄm_MCBData[nIndex].d_head.head_use.accEnergyÎª³öÊø¹â»úµÄ±àºÅ( m_MCBData[nIndex].d_head.head_use.accEnergy = (long)l_angle);
	//³öÊø¹â»úµÄ±àºÅ¹æÔò£º¹â»ú´¥·¢ĞÅºÅ½ÓDI-0£¬¹â»úÑ­»·ĞÅºÅ½ÓDI-1£¬DI-1Îª1Ê±µÄ¹â»ú±àºÅÎª0£¬DI-1Îª0ºóÃ¿Ò»´Î´¥·¢µÄ¹â»ú±àºÅ+1
	//ÉäÏßÔ´ĞòºÅ
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
	//¼ÇÂ¼·¢ËÍ¸øÈí¼şµÄÊı¾İ°ü°üÍ·ĞÅÏ¢
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
    memcpy(&m_MCBDataBackup[nMCB], &m_MCBData[nMCB], sizeof(MCBData));//Êı¾İ±¸·İ
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
		//Ö´ĞĞXÃüÁî¶ÁÈ¡¼Ä´æÆ÷Öµ
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
//    b1[0] =  ((m_MCBReadySts[0]&0x0f)==0x0f);//Éè±¸¾ÍĞ÷×´Ì¬
//    b = b && ((m_MCBReadySts[0]&0x0f)==0x0f);//Éè±¸¾ÍĞ÷×´Ì¬
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
//    //Èç¹ûÖ»ÓĞÒ»¸ö¿ØÖÆºĞ£¬startÒÑÖ±½ÓÆô¶¯²É¼¯
//    if ((m_MCBRunSts[0]&0x01) == 1)
//    {
//        //TRACE(_T("Master HMCB start OK, begin running...\n"));
//        //WriteLogFile(_T("Master HMCB start OK, begin running...\n"));
//        //²»ĞèCheckRunStatus
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
//    //Èç¹ûÖ»ÓĞÒ»¸ö¿ØÖÆºĞ£¬ÔòÖ±½ÓÍ£Ö¹Ö÷¿ØÖÆºĞ²É¼¯
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
//    //Ö÷¿ØÖÆºĞ×´Ì¬
//    b = ((m_MCBRunSts[nMCB]&0x04)== 0x04);
//    ///////////////////////////////////////////////////////
//    if(!b)
//    {
//        sz_szInfo="Master HMCB not end running and end receive data.\n";
//        std::cout << (sz_szInfo);
//        WriteLogFile(sz_szInfo);
//        //ÖØ·¢Í£Ö¹²É¼¯??
//    }
//    //
//    b = b && ((m_MCBRunSts[0]&0x04)== 0x04);//Éè±¸¾ÍĞ÷×´Ì¬
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
//        //´òÓ¡×îºóÒ»´ÎCB×´Ì¬
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
