// MCBCtrl.cpp : Defines the initialization routines for the DLL.
//
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include "stdafx.h"
#include <afxdllx.h>
#else
#include "replenish_type.hpp"
#include <unistd.h>
#include <thread>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <mutex>
#endif

#include "MCBCtrl.h"
#include "MCBCtrlC.h"
#include "MCBThread.h"
#ifdef _MANAGED
#error Please read instructions in MCBCtrl.cpp to compile with /clr
// If you want to add /clr to your project you must do the following:
//	1. Remove the above include for afxdllx.h
//	2. Add a .cpp file to your project that does not have /clr thrown and has
//	   Precompiled headers disabled, with the following text:
//			#include <afxwin.h>
//			#include <afxdllx.h>
#endif

#ifdef _DEBUG
#define DEBUG_NEW new
#endif

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
static AFX_EXTENSION_MODULE MCBCtrlDLL = { NULL, NULL };
#endif

#ifdef _MANAGED
#pragma managed(push, off)
#endif

//CMCBCtrlC mcb;
CMCBThread* gpMCBThread;
bool gbMCBTimer;
PFCALLBACK gCallBack =NULL;
//extern boost::shared_mutex mutex1;
//extern mutex _mutex;

std::string g_strIniFilePath ="";
UINT MCBTimer(void * pParam);
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
HINSTANCE ghInstance;
extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, void * lpReserved)
{
	// Remove this if you use lpReserved
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("MCBCTRL.DLL Initializing!\n");
		
		// Extension DLL one-time initialization
		if (!AfxInitExtensionModule(MCBCtrlDLL, hInstance))
			return 0;

		// Insert this DLL into the resource chain
		// NOTE: If this Extension DLL is being implicitly linked to by
		//  an MFC Regular DLL (such as an ActiveX Control)
		//  instead of an MFC application, then you will want to
		//  remove this line from DllMain and put it in a separate
		//  function exported from this Extension DLL.  The Regular DLL
		//  that uses this Extension DLL should then explicitly call that
		//  function to initialize this Extension DLL.  Otherwise,
		//  the CDynLinkLibrary object will not be attached to the
		//  Regular DLL's resource chain, and serious problems will
		//  result.

		new CDynLinkLibrary(MCBCtrlDLL);
		gpMCBThread =NULL;
		gbMCBTimer =FALSE;
		ghInstance = hInstance;
		
		// Sockets initialization
		WSADATA  Ws;
		//Init Windows Socket   /* �趨��ʹ�õ�Winsock��汾 */
		WSAStartup(MAKEWORD(2,2), &Ws);
		// NOTE: If this Extension DLL is being implicitly linked to by
		//  an MFC Regular DLL (such as an ActiveX Control)
		//  instead of an MFC application, then you will want to
		//  remove the following lines from DllMain and put them in a separate
		//  function exported from this Extension DLL.  The Regular DLL
		//  that uses this Extension DLL should then explicitly call that
		//  function to initialize this Extension DLL.
		if (!AfxSocketInit())
		{
			return FALSE;
		}
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("MCBCTRL.DLL Terminating!\n");
		// Terminate the library before destructors are called
		AfxTermExtensionModule(MCBCtrlDLL);
	}
	return 1;   // ok
}
#endif

#ifdef _MANAGED
#pragma managed(pop)
#endif

void SetMCBCommand(WPARAM wParam, LPARAM lParam) 
{
 //std::cout << " SetMCBCommand  PID : "  << std::this_thread::get_id() << std::endl;
    std::string str;
    std::string tmp;
    str = "MCB Receive SetMCBCommand: CMD=" +std::to_string(wParam);
   // std::cout << str << std::endl;
//    if(gpMCBThread ==NULL)
//    {
//        gpMCBThread = new CMCBThread;
//        gCallBack = (PFCALLBACK)lParam;
//        gbMCBTimer =true;
//        std::thread th(MCBTimer,gpMCBThread);
//        th.detach();
//    }
    if(wParam ==MCB_CMD_SETCONFIGFILE)
    {
        if (lParam ==NULL)
        {
            str ="Configuration file is invalid! ";
            // ROS_ERROR_STREAM(str);
            return ;
        }
        g_strIniFilePath = (char*)lParam;
    }
    if (gpMCBThread ==NULL && wParam !=MCB_CMD_INITIAL)
	{
		str ="ERROR! MCB is not initialed!\n";
        std::cout << str << std::endl;
		return;
	}

	switch(wParam) 
	{
        case MCB_CMD_SETCONFIGFILE:
            if (lParam ==NULL)
            {
                str ="Configuration file is invalid! ";
               // ROS_ERROR_STREAM(str);
                return ;
            }
            g_strIniFilePath = (char*)lParam;
            //g_bIsSendError = false;
    //            cerr<< "g_strIniFilePath:"<<g_strIniFilePath<<endl;
            break;
		case MCB_CMD_INITIAL:
        #if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
			if (gpMCBThread ==NULL)
			{
				if (lParam ==NULL) 
				{
					str ="ERROR! Initial MCB must set callback function!\n";
                    std::cout << str << std::endl;
					return;
				}
				gpMCBThread =(CMCBThread*)AfxBeginThread(RUNTIME_CLASS(CMCBThread));
				gCallBack = (PFCALLBACK)lParam;
                gbMCBTimer =true;
				AfxBeginThread((AFX_THREADPROC)MCBTimer, gpMCBThread);
			}
            gpMCBThread->PostThreadMessage(WM_MCBTHREAD_CMD, MCB_CMD_INITIAL, 0);
#else
            {
                if (gpMCBThread ==NULL)
                {
                    if (lParam ==NULL)
                    {
                        str ="ERROR! Initial MCB must set callback function!\n";
                        std::cout << str << std::endl;
                        return;
                    }
                    gpMCBThread = new CMCBThread;
                    gCallBack = (PFCALLBACK)lParam;
                    gbMCBTimer =true;
                    std::thread th(MCBTimer,gpMCBThread);
                    th.detach();
                }
                //gpMCBThread->PostThreadMessage(WM_MCBTHREAD_CMD, MCB_CMD_INITIAL, 0);
                std::thread th_tmp([&]{
                   //boost::unique_lock<boost::mutex> lock(mutex1);
                    if(gpMCBThread ==NULL)
                        std::cout << " NULL NULL NULL NULL  " << std::endl;
                    gpMCBThread->MCBCommand(MCB_CMD_INITIAL, 0);
                });
                th_tmp.detach();
            }
#endif
			break;
		case MCB_CMD_CLOSE:
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
			if (gpMCBThread)
			{
				gpMCBThread->PostThreadMessage(WM_MCBTHREAD_CMD, MCB_CMD_CLOSE, 0);
                gbMCBTimer =false;
				//Sleep(200); // wait for connection close;
				gpMCBThread->PostThreadMessage(WM_QUIT, 0, 0);
				gpMCBThread =NULL;
				//gCallBack =NULL;
				Sleep(400); // wait for connection close;
            }
#else
            if (gpMCBThread)
            {
                //gpMCBThread->PostThreadMessage(WM_MCBTHREAD_CMD, MCB_CMD_CLOSE, 0);
                std::thread th([=]{
                    gpMCBThread->MCBCommand(MCB_CMD_CLOSE, 0);
                });
                th.detach();
                gbMCBTimer =false;
                //Sleep(200); // wait for connection close;
                //gpMCBThread->PostThreadMessage(WM_QUIT, 0, 0); // �@��linux�汾����Ҫ���]�д��ڣ��M�̽Y�������� ssx 2023��10��17��10:50:22
                delete gpMCBThread;
                gpMCBThread =NULL;
                //gCallBack =NULL;
                usleep(400*1000); // wait for connection close;
            }
#endif
			break;
		case MCB_CMD_GETDMCHNNUM:
			//*((UINT *)lParam) =gpMCBThread->m_pMCB->GetDMCHNNum(*((UINT *)lParam));
			break;
		case MCB_CMD_GETDMADBIT:
			*((INT *)lParam) =gpMCBThread->m_pMCB->m_nADBit;
			if (gpMCBThread->m_pMCB->m_bFcto16ADBit && gpMCBThread->m_pMCB->m_nADBit>16)
			{
				*((INT *)lParam) =16;
			}
			break;
		case MCB_CMD_GETMCBNUM:
			*((UINT *)lParam) =gpMCBThread->m_pMCB->GetMCBNum();
			break;
		case MCB_CMD_GETDMINFO:
			gpMCBThread->m_pMCB->GetDMInfo((DMINFO *)lParam);
			break;
		case MCB_CMD_GETDMEXINFO:
			gpMCBThread->m_pMCB->GetDMExInfo((DMEXINFO *)lParam);
			break;
		case MCB_CMD_GETFNDMPARA:
			//gpMCBThread->m_pMCB->GetFNDMPara((FNDMPARA *)lParam);
			break;
		case MCB_CMD_SETFNDMPARA:
			//memcpy(&(gpMCBThread->m_pMCB->m_FndmPara), (const void *)lParam, sizeof(gpMCBThread->m_pMCB->m_FndmPara));
			//gpMCBThread->m_pMCB->UpdateFNDReg();
			//gpMCBThread->PostThreadMessage(WM_MCBTHREAD_CMD, MCB_CMD_SETFNDMPARA, 0);
			break;
		case MCB_CMD_SETRANGE:
			{
				unsigned char * pRange;
				unsigned short nMCB;
				pRange = (unsigned char *) (lParam);
				nMCB =*pRange;
				if (nMCB >= gpMCBThread->m_pMCB->m_nMCBNum)
				{
					nMCB = gpMCBThread->m_pMCB->m_nMCBNum -1;
				}
				memset(gpMCBThread->m_pMCB->m_cDMRange[nMCB], 0, sizeof(gpMCBThread->m_pMCB->m_cDMRange[nMCB]));
				memcpy(gpMCBThread->m_pMCB->m_cDMRange[nMCB], (const void *)lParam, gpMCBThread->m_pMCB->m_DMInfo[nMCB].nDMNum +1);
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
                gpMCBThread->PostThreadMessage(WM_MCBTHREAD_CMD, MCB_CMD_SETRANGE, (LPARAM)(gpMCBThread->m_pMCB->m_cDMRange[nMCB]));
#else
                std::thread th([=]{
                    gpMCBThread->MCBCommand(MCB_CMD_SETRANGE, (LPARAM)(gpMCBThread->m_pMCB->m_cDMRange[nMCB]));
                });
                th.detach();
#endif
        }
			break;
		case MCB_CMD_SET2TYPERANGE:
			//{
			//	unsigned char * pRange;
			//	unsigned short nMCB;
			//	pRange = (unsigned char *) (lParam);
			//	nMCB =*pRange;
			//	if (nMCB >= gpMCBThread->m_pMCB->m_nMCBNum)
			//	{
			//		nMCB = gpMCBThread->m_pMCB->m_nMCBNum -1;
			//	}
			//	memset(gpMCBThread->m_pMCB->m_cDMRange[nMCB], 0, sizeof(gpMCBThread->m_pMCB->m_cDMRange[nMCB]));
			//	memcpy(gpMCBThread->m_pMCB->m_cDMRange[nMCB], (const void *)lParam, gpMCBThread->m_pMCB->m_DMInfo[nMCB].nDMNum*2 +1);
			//	gpMCBThread->PostThreadMessage(WM_MCBTHREAD_CMD, MCB_CMD_SET2TYPERANGE, (LPARAM)(gpMCBThread->m_pMCB->m_cDMRange[nMCB]));
			//}
            break;
        ////////////////////////////////////////MXG_ADD FOR YH's Demmands begin//////////////////////
        case MCB_CMD_GETDVREADY:
                *((UINT *)lParam) =gpMCBThread->m_pMCB->m_bDVReady?1:0;
                break;
        case MCB_CMD_GETRANGE:
                *((UINT *)lParam) =gpMCBThread->m_pMCB->m_nADMCMDREG[0][0]&0x07;
                break;
        case MCB_CMD_GETRANGEMODE:
                *((UINT *)lParam) = 0;
                //*((UINT *)lParam) =gpMCBThread->m_pMCB->m_nRangeMode;
                // void CMCBCtrlC::SetRange(unsigned short nRange)
                break;
        case MCB_CMD_GETPOTMT:
                *((UINT *)lParam) =gpMCBThread->m_pMCB->m_reg.regPOTMT;
                break;
        case MCB_CMD_GETPOTMW:
                *((UINT *)lParam) =gpMCBThread->m_pMCB->m_reg.regPOTMW;
                break;
        case MCB_CMD_GETINTTM:
                *((UINT *)lParam) =gpMCBThread->m_pMCB->m_reg.regINTT;
                break;
        case MCB_CMD_TRIGGET:
                *((UINT *)lParam) =gpMCBThread->m_pMCB->m_nTrig;
                break;
        case MCB_CMD_GETRUNSTATUS:  //whether MCB is collecting data
                *((UINT *)lParam) =gpMCBThread->m_pMCB->m_bStart?1:0;
                break;
        case MCB_CMD_GETREADCOUNT:  //whether MCB is collecting data
                *((UINT *)lParam) =gpMCBThread->m_pMCB->m_MCBData[0].d_head.head_use.scounter;
                break;
        ////////////////////////////////////////MXG_ADD FOR YH's Demmands end//////////////////////
		default:
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            gpMCBThread->PostThreadMessage(WM_MCBTHREAD_CMD, wParam, lParam);
#else
            std::thread th([=]{
                gpMCBThread->MCBCommand(wParam, lParam);
            });
            th.detach();
#endif
    }
}
//0.2s elapse
//2s elapse
UINT MCBTimer(void * pParam)
{
   // return -1;
   // boost::unique_lock<boost::mutex> lock(mutex1);
	while (gbMCBTimer)
	{
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        Sleep(200);
#else
       // usleep(200*1000);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
#endif
        if (gpMCBThread)
		{
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            gpMCBThread->PostThreadMessage(WM_MCBTHREAD_TIMER, 0, 0);
#else
            std::thread th([=]{
                gpMCBThread->MCBTimer(0,0);
            });
            th.detach();
#endif
        }
	}
    std::cout << "MCB Timer Thread Terminating!\n";
	return 0;
}
