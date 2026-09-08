// RequestSocket.cpp : implementation file
//
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include "stdafx.h"
#else
#include <thread>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#endif
#include "RequestSocket.h"
#include "MCBCtrl.h"
#include "MCBCtrlC.h"

#ifdef _DEBUG
#define DEBUG_NEW new
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//extern boost::shared_mutex mutex1;
/////////////////////////////////////////////////////////////////////////////
// CRequestSocket

CRequestSocket::CRequestSocket(CMCBCtrlC* pMCBCtrlC, int nIndex, int nPort)
{
	m_pMCBCtrlC = pMCBCtrlC;
	m_nIndex =nIndex;
	m_nSocketPort = nPort;
}

CRequestSocket::~CRequestSocket()
{
}

// Do not edit the following lines, which are needed by ClassWizard.
#if 0
BEGIN_MESSAGE_MAP(CRequestSocket, CSocket)
	//{{AFX_MSG_MAP(CRequestSocket)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
#endif	// 0

/////////////////////////////////////////////////////////////////////////////
// CRequestSocket member functions

void CRequestSocket::OnReceive(/*int nErrorCode*/)
{
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	CSocket::OnReceive(nErrorCode);
    // TODO: Add your specialized code here and/or call the base class
#endif
	m_pMCBCtrlC->FetchMsg(this, m_nIndex, m_nSocketPort);
}

void CRequestSocket::OnClose(int nErrorCode) 
{
	// TODO: Add your specialized code here and/or call the base class
	m_pMCBCtrlC->ConnectClose(m_nIndex, 0);
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    CSocket::OnClose(nErrorCode);
#endif
}

int CRequestSocket::Send(const void* lpBuf, int nBufLen, int nFlags)
{
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	// TODO: Add your specialized code here and/or call the base class
	int retl, idx, nlen;

	idx =0;
	nlen = nBufLen;
	while (nBufLen>0)
	{
		if ((retl =CSocket::Send((char *)(lpBuf)+idx, nBufLen, nFlags)) ==SOCKET_ERROR)
		{
			return SOCKET_ERROR;
		}
		nBufLen -=retl;
		idx +=retl;
    }
#else
    int retl, idx, nlen;
    idx =0;
    nlen = nBufLen;
    while(nBufLen>0) //boost �ƺ�������Ҫ���ֲ���
    {
        //boost::system::error_code ec;
        //size_t retl = m_socket->write_some(buffer(lpBuf,nBufLen), ec);
        qint64 retl = m_socket->write((char*)lpBuf,nBufLen);

//        if(ec)
//        {
//            std::cout<<boost::system::system_error(ec).what()<< std::endl;
//            return -1;
//        }
        if(retl == -1)
        {
            std::cout << " send message error ������" << std::endl;
            return -1;
        }
        nBufLen -=retl;
        idx +=retl;
        std::cout << "writed "<< retl << std::endl;
    }
#endif
    return nlen;
}


#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
bool CRequestSocket::OnMessagePending()
{

	MSG msg;
	if(::PeekMessage(&msg, NULL, WM_TIMER, WM_TIMER, PM_NOREMOVE))
	{
		if (msg.wParam == (UINT) m_nTimerID)
		{
			// Remove the message and call CancelBlockingCall.
			::PeekMessage(&msg, NULL, WM_TIMER, WM_TIMER, PM_REMOVE);
			CancelBlockingCall();
			return FALSE;  // No need for idle time processing.
		}
	}
	return CSocket::OnMessagePending();

    return true;

}

bool CRequestSocket::SetTimeOut(int nTimeOut)
{
	m_nTimerID = (int)(SetTimer(NULL,0,nTimeOut,NULL));
	return m_nTimerID;
}

bool CRequestSocket::KillTimeOut()
{
	return KillTimer(NULL,m_nTimerID);
}
#else
int CRequestSocket::Receive(void *lpBuf, int nBufLen, int nFlags)
{
    //nBufLen =292;
    //boost::unique_lock<boost::mutex> lock(mutex1);
    memset(lpBuf,0,nBufLen);
    if(m_id != std::this_thread::get_id())
    {
        std::cout << " ���߳���  ��������������������������" << std::endl;
        exit(-1);
    }
    else
    {
        std::cout << " CRequestSocket::Receive  : PID  " << std::this_thread::get_id() << std::endl;
    }
    if(m_socket->bytesAvailable()<=0) return -1;
   // std::cout << "  available  " << m_socket->available() << std::endl;
    int  len=m_socket->read((char*)lpBuf,nBufLen);

    if(len<0)
    {
        std::cout<<" read message error !!! "  << std::endl;
        return -1;
    }

    return len;
}

void CRequestSocket::Close()
{
   // boost::unique_lock<boost::shared_mutex> read_lock(mutex1);
    if(m_socket->isOpen())
    {
        m_socket->close();
        //m_socket->close();
    }
}

bool CRequestSocket::Create(int nSocketPort, int nSocketType, std::string lpszSocketAddress)
{
    //io_service iosrv;
    //m_socket = new ip::tcp::socket(iosrv);
    return true;
}

bool CRequestSocket::SetSockOpt(int nOptionName, const void *lpOptionValue, int nOptionLen, int nLevel)
{


    return true;
}

bool CRequestSocket::GetSockOpt(int nOptionName, void *lpOptionValue, int *lpOptionLen, int nLevel)
{
    return true;
}

bool CRequestSocket::Connect(const std::string &strHostAddr, const int &nHostPort)
{
    fflush(stdout);
   // boost::unique_lock<boost::mutex> lock(mutex1);
    ip::tcp::endpoint ep(ip::address_v4::from_string(strHostAddr),nHostPort);
    boost::system::error_code ec;
    if(m_socket != NULL)
    {
        m_socket->close();
        delete m_socket;
        m_socket = NULL;
    }
    m_socket = new QTcpSocket;
    m_socket->connectToHost(QString::fromStdString(strHostAddr),nHostPort);

    if(!m_socket->isOpen())
    {
        std::cout<<" scoket connect to host error  "<< std::endl;
        return false;
    }
    else
    {
//        std::thread th([&]{
//            while(m_socket !=NULL)
//            {
//                std::cout << " CRequestSocket::Connect  PID : "  << std::this_thread::get_id() << std::endl;
//                m_id = std::this_thread::get_id();
//              // boost::unique_lock<boost::shared_mutex> /*try_*/lock(mutex1);
//                if(m_socket!=NULL&& m_socket->isOpen())
//                {
//                    std::cout << " m_socket->available()   " << m_nIndex << "  "  << m_socket->bytesAvailable() << std::endl;
//                    if(m_socket->isOpen()&& m_socket->bytesAvailable()>0)
//                    {
//                        fflush(stdout);
//                        OnReceive(0);
//                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
//                    }
//                    else
//                    {
//                        std::this_thread::sleep_for(std::chrono::milliseconds(15));
//                    }
//                }
//               // lock.unlock();
//            }
//        });
//        th.detach();
        connect(m_socket,SIGNAL(readyRead()),this,SLOT(OnReceive()));
        return true;
    }
    return true;
}
#endif
