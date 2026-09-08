// RequestSocket.cpp : implementation file
//
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include "stdafx.h"
#else
#include <thread>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <mutex>
#endif
#include "RequestSocket.h"
#include "MCBCtrl.h"
#include "MCBCtrlC.h"
//#include "slog.h"
#include "logAdapter.h"
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
    /* FIX: stop the receive thread BEFORE freeing m_socket. It used to be
     * detached and kept touching the destroyed object after ConnectClose()
     * deleted m_pDataSocket -> segfault a few seconds after cable unplug. */
    StopRecvThread();
    std::unique_lock<std::mutex> lock(m_mutex_scoket);
    if(m_socket != NULL)
    {
        boost::system::error_code ignore_ec;
        m_socket->close(ignore_ec);
        delete m_socket;
        m_socket = NULL;
    }
}

/* FIX: stop the receive thread and wait for its exit (<=1s, bounded by
 * SO_RCVTIMEO). Guard against self-join just in case. */
void CRequestSocket::StopRecvThread()
{
    m_bStopThread = true;
    if (m_recvThread.joinable() && m_id != std::this_thread::get_id())
        m_recvThread.join();
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

void CRequestSocket::OnReceive(int nErrorCode) 
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
    std::unique_lock<std::mutex> lock(m_mutex_scoket);
    while(nBufLen>0) //boost �ƺ�������Ҫ���ֲ���
    {

        struct tcp_info info;
        int leninfo = sizeof(info);
        getsockopt(m_socket->native_handle(), IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&leninfo);

        if (info.tcpi_state == TCP_ESTABLISHED && info.tcpi_lost==0 )
        {


        }
        else
        {
            printf("Connection closed\n");
            return -1;
        }

        boost::system::error_code ec;
        //loger::getInstance()->writeLog(Info,"m_socket->write_some  begin");
        size_t retl = m_socket->write_some(buffer(lpBuf,nBufLen), ec);
        //loger::getInstance()->writeLog(Info,"m_socket->write_some  end");
        if(ec)
        {
            std::cout<<boost::system::system_error(ec).what()<< std::endl;
            return -1;
        }
        nBufLen -=retl;
        idx +=retl;
        std::cout << "writed "<< retl << std::endl;
    }
    lock.unlock();
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
     std::unique_lock<std::mutex> lock(m_mutex_scoket);
    boost::system::error_code ec;
    memset(lpBuf,0,nBufLen);
//    if(m_id != std::this_thread::get_id())
//    {
//        std::cout << " ���߳���  ��������������������������" << std::endl;
//        exit(-1);
//    }
//    else
//    {
//        std::cout << " CRequestSocket::Receive  : PID  " << std::this_thread::get_id() << std::endl;
//    }
    if(m_socket==NULL)
    {
        lock.unlock();
        return -1;
    }
    if(m_socket->available()<=0)
    {
        lock.unlock();
        return m_socket->available();
    }

    struct tcp_info info;
    int leninfo = sizeof(info);
    getsockopt(m_socket->native_handle(), IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&leninfo);

    if (info.tcpi_state == TCP_ESTABLISHED && info.tcpi_lost==0 )
    {


    }
    else
    {
        lock.unlock();
        printf("Connection closed\n");
        loger::getInstance()->writeLog(Error,"Connection closed" + std::to_string(__LINE__) );
        return -1;
    }
    //loger::getInstance()->writeLog(Info,"m_socket->read_some  begin" + std::to_string(__LINE__) );
    int  len=m_socket->read_some(buffer(lpBuf,nBufLen),ec);
    //loger::getInstance()->writeLog(Info,"m_socket->read_some  end" + std::to_string(__LINE__));
    if(ec)
    {
        std::cout<<boost::system::system_error(ec).what()<< std::endl;
        lock.unlock();
        return -1;
    }
    lock.unlock();
    return len;
}

void CRequestSocket::Close()
{
    /* FIX: real close - stop receive thread, then close & free socket */
    StopRecvThread();
    std::unique_lock<std::mutex> lock(m_mutex_scoket);
    if(m_socket != NULL)
    {
        boost::system::error_code ignore_ec;
        m_socket->close(ignore_ec);
        delete m_socket;
        m_socket = NULL;
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
   // boost::unique_lock<boost::mutex> lock(mutex1);
    /* FIX: stop previous receive thread before touching m_socket */
    StopRecvThread();
    std::unique_lock<std::mutex> lock(m_mutex_scoket);
    ip::tcp::endpoint ep(ip::address_v4::from_string(strHostAddr),nHostPort);
    boost::system::error_code ec;
    if(m_socket != NULL)
    {
        boost::system::error_code ignore_ec;
        m_socket->close(ignore_ec);
        delete m_socket;
        m_socket = NULL;
    }
    m_socket = new ip::tcp::socket(m_iosrv); /* FIX: member io_service (local one died with Connect()) */
    m_socket->connect(ep,ec);

    boost::asio::socket_base::keep_alive option(true);
    m_socket->set_option(option);

    int keepalive = 1;
    int keepidle = 6;
    int keepinterval = 6;
    int keepcount = 60;
    setsockopt(m_socket->native_handle(), SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));
    setsockopt(m_socket->native_handle(), SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle ));
    setsockopt(m_socket->native_handle(), SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval ));
    setsockopt(m_socket->native_handle(), SOL_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount ));

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(m_socket->native_handle(), SOL_SOCKET, SO_RCVTIMEO, (void *)&tv , sizeof(tv));


    //if(m_socket->is_open())
    //m_socket->set_option(ip::tcp::socket::reuse_address(true));
    lock.unlock();
    if(ec)
    {
        std::cout<<boost::system::system_error(ec).what()<< std::endl;
        /* FIX: free the failed socket instead of leaking it */
        std::unique_lock<std::mutex> lock2(m_mutex_scoket);
        boost::system::error_code ignore_ec;
        m_socket->close(ignore_ec);
        delete m_socket;
        m_socket = NULL;
        return false;
    }
    else
    {
        /* FIX: capture this explicitly, keep the handle (no detach) and
         * honour the atomic stop flag so the thread never outlives us */
        m_bStopThread = false;
        m_recvThread = std::thread([this]{
            while(!m_bStopThread && m_socket !=NULL)
            {
                m_id = std::this_thread::get_id();
//                if(m_socket!=NULL&& m_socket->is_open())
//                {

                    //if(m_socket!=NULL&&m_socket->is_open()&& m_socket->available()>0)
                    {
                        OnReceive(0);
                        std::this_thread::sleep_for(std::chrono::microseconds(200));
                    }
//                    else
//                    {
//                        std::this_thread::sleep_for(std::chrono::milliseconds(3));
//                    }
               // }
                //lock.unlock();
            }
        });
        return true;
    }
    return true;
}

int CRequestSocket::Available()
{
    std::unique_lock<std::mutex> lock(m_mutex_scoket);
    int ret =0;
    if(m_socket !=NULL)
    {
        ret = m_socket->available();
        lock.unlock();
        return ret;
    }

    lock.unlock();
    return ret;
}

int CRequestSocket::CheckSocketStatus()
{
    struct tcp_info info;
    int leninfo = sizeof(info);
    getsockopt(m_socket->native_handle(), IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&leninfo);
    if (info.tcpi_state == TCP_ESTABLISHED && info.tcpi_lost==0 )
    {
        return 0;
    }
    else
    {
        return -1;
    }
}
#endif
