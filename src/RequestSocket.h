#if !defined(AFX_REQUESTSOCKET_H__2C15F241_9554_42E2_8ED8_084638F59067__INCLUDED_)
#define AFX_REQUESTSOCKET_H__2C15F241_9554_42E2_8ED8_084638F59067__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RequestSocket.h : header file
//
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)

#else
#include<boost/asio.hpp>
#include<mutex>
#include<atomic>
#include<thread>

using namespace boost::asio;
#endif

class CMCBCtrlC;

/////////////////////////////////////////////////////////////////////////////
// CRequestSocket command target
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
class CRequestSocket : public CSocket
#else
class CRequestSocket
#endif
{
// Attributes
public:
	CMCBCtrlC* m_pMCBCtrlC;
	int m_nIndex;
	int m_nSocketPort;

// Operations
public:
	CRequestSocket(CMCBCtrlC* pMCBCtrlC, int nIndex, int nPort);
	virtual ~CRequestSocket();

// Overrides
public:
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    virtual bool OnMessagePending();
#else

#endif
	virtual void OnReceive(int nErrorCode);
	virtual void OnClose(int nErrorCode);
	virtual int Send(const void* lpBuf, int nBufLen, int nFlags = 0);



#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
// Implementation
protected:
	int m_nTimerID;
private:

    bool KillTimeOut();
    bool SetTimeOut(int nTimeOut);
#else
public:
    virtual int Receive(void* lpBuf, int nBufLen, int nFlags = 0);
    virtual void Close();
    virtual bool Create(int nSocketPort = 0, int nSocketType=SOCK_STREAM,std::string lpszSocketAddress="");
    virtual bool SetSockOpt(int nOptionName, const void* lpOptionValue,int nOptionLen, int nLevel = SOL_SOCKET);
    virtual bool GetSockOpt(int nOptionName, void* lpOptionValue, int* lpOptionLen, int nLevel = SOL_SOCKET);
    virtual bool Connect(const std::string & strHostAddr,const int & nHostPort);
    virtual int Available();

    virtual int CheckSocketStatus();//检测socket 链接状态
private:
    io_service m_iosrv; /* FIX: must outlive m_socket - its dtor deregisters from the io_service */
    ip::tcp::socket *m_socket=NULL;
    std::thread::id m_id;
    std::mutex m_mutex_scoket;
    std::atomic<bool> m_bStopThread{false}; /* FIX: recv-thread stop flag */
    std::thread m_recvThread;               /* FIX: keep handle, no detach */
    void StopRecvThread();
#endif
public:
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REQUESTSOCKET_H__2C15F241_9554_42E2_8ED8_084638F59067__INCLUDED_)
