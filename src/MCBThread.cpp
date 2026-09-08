// MCBThread.cpp : implementation file
//

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include "stdafx.h"
#else
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <mutex>
#endif
#include "MCBThread.h"
#include "MCBCtrl.h"
#include "MCBCtrlC.h"

#ifdef _DEBUG
#define DEBUG_NEW new
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//extern boost::shared_mutex mutex1;
//extern mutex _mutex;
/////////////////////////////////////////////////////////////////////////////
// CMCBThread
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
IMPLEMENT_DYNCREATE(CMCBThread, CWinThread)
#endif
CMCBThread::CMCBThread()
{
	m_pMCB = new (CMCBCtrlC);
}

CMCBThread::~CMCBThread()
{
	delete m_pMCB;
}

bool CMCBThread::InitInstance()
{
	// TODO:  perform and per-thread initialization here
//	m_pMCB->m_pParentWnd =GetMainWnd();
    return true;
}

int CMCBThread::ExitInstance()
{
	// TODO:  perform any per-thread cleanup here
	m_pMCB->CloseMCB();
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	TRACE(_T("MCB Communication Thread Terminating!\n"));
    return CWinThread::ExitInstance();
#endif
}
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
BEGIN_MESSAGE_MAP(CMCBThread, CWinThread)
	//{{AFX_MSG_MAP(CMCBThread)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_THREAD_MESSAGE(WM_MCBTHREAD_CMD, MCBCommand)
	ON_THREAD_MESSAGE(WM_MCBTHREAD_TIMER, MCBTimer)
END_MESSAGE_MAP()
#endif
/////////////////////////////////////////////////////////////////////////////
// CMCBThread message handlers

void CMCBThread::MCBCommand(WPARAM wParameter, LPARAM lParameter)
{
    //boost::unique_lock<boost::shared_mutex> read_lock(mutex1);
	UINT wParam = (UINT)(wParameter);
	LONG lParam = (LONG)(lParameter);

    std::string str = ("\n");
    std::string cmd_str = ("");
    //std::cout << str;
    m_pMCB->WriteLogFile(str);
    //
	Conv_FromAppCmd_code_to_String(wParam, cmd_str);
    //
    char tmp[512] = {0};
    sprintf(tmp,"HMCB.dll Received a Command from the APP: CMD=0x%04x(%s).\n", wParam, cmd_str.c_str());
    str = tmp;
    //std::cout << str;
	m_pMCB->WriteLogFile(str);
	//
	switch(wParam) 
	{
		case MCB_CMD_INITIAL:
			m_pMCB->InitialAllMCB();
			break;
        case MCB_CMD_CONNECTMCB:
        {
            if(m_pMCB->m_bEnMCBx4)
            {
                m_pMCB->InitialMCB_HMCB4((unsigned short) lParam);
            }
            else {
                m_pMCB->InitialMCB((unsigned short) lParam);
            }

        }
            break;
		case MCB_CMD_CLOSE:
			m_pMCB->CloseMCB();
			break;
		case MCB_CMD_START:
			m_pMCB->Start(lParam);
			break;
		case MCB_CMD_STOP:
			m_pMCB->Stop(0);
			break;
        case MCB_CMD_SETMODE:
            m_pMCB->SetMode((unsigned short) lParam);
			break;
		case MCB_CMD_SETRANGE:
			m_pMCB->SetRange((unsigned char *) (lParam));
			break;
		case MCB_CMD_SET2TYPERANGE:
			//m_pMCB->Set2Range((unsigned char *) (lParam));
			break;
        case MCB_CMD_SETRANGEMODE:
            m_pMCB->SetRange((unsigned short) lParam);
			break;
		case MCB_CMD_TRIGSET:
			m_pMCB->SetTrig(lParam);
			break;
		case MCB_CMD_SETDEMODE:
			//m_pMCB->SetDeMode(lParam);
			break;
		case MCB_CMD_SETNSTYPE:
			//m_pMCB->SetNsType(lParam);
			break;
		case MCB_CMD_SETPOTMT:
			m_pMCB->SetPOTMT(lParam);
			break;
		case MCB_CMD_SETPOTMW:
			m_pMCB->SetPOTMW(lParam);
			break;
		case MCB_CMD_SETINTTM:
			m_pMCB->SetINTTm(lParam);
			break;
		case MCB_CMD_AOUTDASET:
			m_pMCB->SetDA(DWORD(lParam), 0);
			break;
		case MCB_CMD_TESTMODE:
            m_pMCB->SetTMode(bool(lParam));
			break;
		case MCB_CMD_SETPTFDIV:
			m_pMCB->SetPTFDiv(UINT(lParam));
			break;
		case MCB_CMD_SETFNDMPARA:
			//m_pMCB->SetFNDMParam();
			break;
		case MCB_CMD_SETALLDATA:
			m_pMCB->SetSendAllData(lParam);
			break;
		case MCB_CMD_SETNEWIP:
			m_pMCB->SetNewIPAddress((unsigned char *) (lParam), 0);
			break;
		case MCB_CMD_DUMPALLECSREG:
			m_pMCB->DumpAllMCBAllTheECSRegisters((UINT) lParam);
			break;
 		default:
			;
	}
	//return 0;
}

	//��������ת�������ַ���
void CMCBThread::Conv_FromAppCmd_code_to_String(UINT cmd_code,  std::string& str)
{
	switch(cmd_code)
	{					 
		case MCB_CMD_INITIAL:
            str="MCB_CMD_INITIAL";
			break;
		case MCB_CMD_CONNECTMCB:
            str = "MCB_CMD_CONNECTMCB";
			break;
		case MCB_CMD_CLOSE:
            str = "MCB_CMD_CLOSE";
			break;
		case MCB_CMD_START:
            str = "MCB_CMD_START";
			break;
		case MCB_CMD_STOP:
            str = "MCB_CMD_STOP";
			break;
		case MCB_CMD_SETMODE:
            str = "MCB_CMD_SETMODE";
			break;
		case MCB_CMD_SETRANGE:
            str = "MCB_CMD_SETRANGE";
			break;
		case MCB_CMD_SET2TYPERANGE:
            str = "MCB_CMD_SET2TYPERANGE";
			break;
		case MCB_CMD_SETRANGEMODE:
            str = "MCB_CMD_SETRANGEMODE";
			break;
		case MCB_CMD_TRIGSET:
            str = "MCB_CMD_TRIGSET";
			break;
		case MCB_CMD_SETDEMODE:
            str = "MCB_CMD_SETDEMODE";
			break;
		case MCB_CMD_SETNSTYPE:
            str = "MCB_CMD_SETNSTYPE";
			break;
		case MCB_CMD_SETPOTMT:
            str = "MCB_CMD_SETPOTMT";
			break;
		case MCB_CMD_SETPOTMW:
            str = "MCB_CMD_SETPOTMW";
			break;
		case MCB_CMD_SETINTTM:
            str = "MCB_CMD_SETINTTM";
			break;
		case MCB_CMD_AOUTDASET:
            str = "MCB_CMD_AOUTDASET";
			break;
		case MCB_CMD_TESTMODE:
            str = "MCB_CMD_TESTMODE";
			break;
		case MCB_CMD_SETPTFDIV:
            str = "MCB_CMD_SETPTFDIV";
			break;
		case MCB_CMD_SETFNDMPARA:
            str = "MCB_CMD_SETFNDMPARA";
			break;
		case MCB_CMD_SETALLDATA:
            str = "MCB_CMD_SETALLDATA";
			break;
		case MCB_CMD_SETNEWIP:
            str = "MCB_CMD_SETNEWIP";
			break;
		case MCB_CMD_DUMPALLECSREG:
            str = "MCB_CMD_DUMPALLECSREG";
			break;
 		default:
            str = "Unknown Command code";
			break;
    }
}

//0.2ms elapse
void CMCBThread::MCBTimer(WPARAM wParam,LPARAM lParam)
{
   // boost::unique_lock<boost::shared_mutex> /*try_*/lock(mutex1);
    if(m_pMCB)
      m_pMCB->OnTimer();
	//return 0;
}

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
BOOL CMCBThread::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_TIMECHANGE)
	{
		m_pMCB->UpdateNewTime();
	}

	return CWinThread::PreTranslateMessage(pMsg);
}
#else

#endif
