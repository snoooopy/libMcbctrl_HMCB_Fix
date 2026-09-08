#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include "stdafx.h"
#else
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "nuctool.h"
#include<boost/asio.hpp>
#include "logAdapter.h"
using namespace boost::asio;

#endif
#include "including.h"
#include "MCB_param.h"
#include <ctime>
#include <vector>
#include <netinet/tcp.h>
#include <stdio.h>
#include <thread>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define gWrRdReptN	(10)
#define gCmdReptN	(1)

/* ECS���������ݰ�*/
typedef struct ETH_CTRL_packet_format{
    INT8U  ver_type;
    INT8U  cmd_flag;
    INT8U  ID;
    INT8U  length;
    INT32U   addr;
    INT8U  buf[256];
}RBCP_PKG_TYPE;

RBCP_PKG_TYPE	_rbcp_buf;
INT8U 			_rbcp_packet_ID;

//������socket
//unsigned int					_ETH_CTRL_socket[MAX_MCBNUM];
//std::vector<unsigned int>       _ETH_CTRL_socket;
std::vector<ip::tcp::socket *> _ETH_CTRL_socket;
struct sockaddr_in		_ETH_CTRL_addr;

//���ݴ���socket
unsigned int                  _ETH_Data_socket;
struct sockaddr_in		_ETH_Data_addr;


//-----------------------------------
//      Ethernet base function
//-----------------------------------

INT32S ETH_init(ULONG S_addr, unsigned short nMCB)
{
    INT32S Ret = 0;
    //
    //Create CTRL Socket
    //_ETH_CTRL_socket[nMCB] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
 /*
    unsigned int nFd  = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if ( nFd == -1 )
    {
        printf("Error when calling socket(): %ld\n", errno);
//        WSACleanup();
        printf("Press any key to exit!\n");
        //getchar();
        return -1;
    }

    _ETH_CTRL_socket.push_back(nFd);
    char tmp[512] = {0};
    sprintf(tmp,"%s",inet_ntoa(*((struct in_addr*)&S_addr)));
    std::string strIp = tmp;


    memset(&_ETH_CTRL_addr,0,sizeof(_ETH_CTRL_addr));
    _ETH_CTRL_addr.sin_family = AF_INET;
    _ETH_CTRL_addr.sin_port = htons(ETH_CTRL_PORT);
    _ETH_CTRL_addr.sin_addr.s_addr = S_addr;

    Ret = connect(_ETH_CTRL_socket[nMCB],(struct sockaddr*)&_ETH_CTRL_addr, sizeof(_ETH_CTRL_addr));

    if ( Ret == -1 )
    {
        printf("Failed to connect to TCP server (CTRL port)!\n");
        printf("Press any key to exit!\n");
        //getchar();
        return -1;
    }

    u_long iMode = 1;
    Ret = ioctl(_ETH_CTRL_socket[nMCB], FIONBIO, &iMode);
*/
    ///////////////////////////////////////////////////////////////////

    char tmp[512] = {0};
    sprintf(tmp,"%s",inet_ntoa(*((struct in_addr*)&S_addr)));
    std::string strIp = tmp;
    ip::tcp::endpoint ep(ip::address_v4::from_string(strIp),ETH_CTRL_PORT);
    boost::system::error_code ec;
    /* FIX: the socket must bind to an io_service that outlives it. A local
     * io_service dies with ETH_init(), and later close()/dtor on the socket
     * dereferences the freed io_service (general protection fault). */
    static io_service g_ctrl_iosrv;
    ip::tcp::socket * sket = new ip::tcp::socket(g_ctrl_iosrv);

    //sket->non_blocking(true);

    /* FIX: bounded connect (3s). The old synchronous connect() blocked
     * ~156s after cable unplug (kernel SYN retransmission black hole;
     * measured 11:59:00->12:01:16 in HMCBCtrl.log), so auto-reconnect
     * ran at most one attempt per ~2.5min - a re-plugged cable took
     * minutes to recover. */
    g_ctrl_iosrv.restart(); /* a previous run_for() may have stopped it */
    sket->async_connect(ep, [&](const boost::system::error_code& e){ ec = e; });
    if (g_ctrl_iosrv.run_for(std::chrono::seconds(3)) == 0) /* 0 = timeout */
    {
        boost::system::error_code ignore_ec;
        sket->close(ignore_ec); /* cancel the pending async_connect   */
        g_ctrl_iosrv.poll();    /* drain the aborted handler NOW while */
                               /* its [&] captures are still on stack  */
        delete sket;
        return -1;
    }
    boost::asio::socket_base::keep_alive option(true);
    sket->set_option(option);
    int keepalive = 1;
    int keepidle = 6;
    int keepinterval = 6;
    int keepcount = 60;
    setsockopt(sket->native_handle(), SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));
    setsockopt(sket->native_handle(), SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle ));
    setsockopt(sket->native_handle(), SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval ));
    setsockopt(sket->native_handle(), SOL_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount ));

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sket->native_handle(), SOL_SOCKET, SO_RCVTIMEO, (void *)&tv , sizeof(tv));

    if(ec)
    {
        std::cout<<boost::system::system_error(ec).what()<< std::endl;
        /* FIX: free the failed socket. asio keeps the fd open after a
         * failed connect; without this every retry leaked one fd until
         * EMFILE and the device could never be reached again. */
        boost::system::error_code ignore_ec;
        sket->close(ignore_ec);
        delete sket;
        return -1;
    }
    //std::cout << " ETH_CTRL_PORT  : " << ETH_CTRL_PORT << std::endl;
    /* FIX: store by slot nMCB instead of push_back. push_back mixed with
     * erase() shifted indices after reconnects, leaving I/O on a dead
     * slot-0 socket while the fresh connection sat unused at the tail. */
    if (_ETH_CTRL_socket.size() <= nMCB)
        _ETH_CTRL_socket.resize(nMCB + 1, nullptr);
    if (_ETH_CTRL_socket[nMCB])
    {
        boost::system::error_code ignore_ec2;
        _ETH_CTRL_socket[nMCB]->close(ignore_ec2);
        delete _ETH_CTRL_socket[nMCB];
    }
    _ETH_CTRL_socket[nMCB] = sket;

    ///////////////////////////////////////////////////////////////////


    return 0;
}

INT32S ETH_WSAStartup()
{
    INT32S Ret = 0;
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    WSADATA  Ws;
    //Init Windows Socket   /* �趨��ʹ�õ�Winsock��汾 */
    if ( WSAStartup(MAKEWORD(2,2), &Ws) != 0 )
    {
        printf("Error when calling WSAStartup()!\n");
        printf("Press any key to exit!\n");
        getchar();
        return -1;
    }
#else
#endif
    return 0;
}


void ETH_WSACleanup()
{
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    WSACleanup();
#else
#endif
}


void ETH_close(unsigned short nMCB)
{
   // return;
    /* �ر�Socket�����socket���� */
    //closesocket(_ETH_Data_socket);
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    closesocket(_ETH_CTRL_socket[nMCB]);
#else
    if(_ETH_CTRL_socket.size()<=0)
        return;
    auto fd_mcb = _ETH_CTRL_socket[nMCB];
    fd_mcb->close();
    _ETH_CTRL_socket.erase(_ETH_CTRL_socket.begin()+nMCB,_ETH_CTRL_socket.begin()+nMCB+1);
    delete fd_mcb;
#endif
    //WSACleanup();
}

void ETH_CTRL_close(unsigned short nMCB)
{
   // return;
    /* �ر�Socket�����socket���� */
    //closesocket(_ETH_Data_socket);
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    closesocket(_ETH_CTRL_socket[nMCB]);
#else
    if(_ETH_CTRL_socket.size()<=0)
        return;
    if(nMCB>=_ETH_CTRL_socket.size())
        return;
    /* FIX: unconditional release. The old TCP_INFO gate spared sockets
     * that still looked ESTABLISHED with lost==0 (e.g. idle link with a
     * silent peer), so a dead socket stayed in its slot and the freshly
     * reconnected one was never used -> 'cannot reconnect after replug'. */
    auto fd_mcb = _ETH_CTRL_socket[nMCB];
    if (fd_mcb)
    {
        boost::system::error_code ignore_ec;
        fd_mcb->close(ignore_ec);
        delete fd_mcb;
    }
    /* FIX: null the slot instead of erase() - erase shifts later entries
     * and desyncs [nMCB] indexing; ETH_CTRL_IsValid() treats null as dead. */
    _ETH_CTRL_socket[nMCB] = nullptr;
#endif
    //WSACleanup();
}

//Return value: 0 = No error, others = Find errors.
INT8U  ETH_CTRL_write(INT32U  addr,INT8U * pdata, INT8U  len, unsigned short nMCB)
{

    INT32S  i;
    INT32S  tr_len =0;
    INT32S	wr_err_cnt = 0;
    //
    /* FIX: guard - the slot may be empty/null (never connected, or
     * closed on link failure); the old bare [nMCB] deref was UB. */
    if (nMCB >= _ETH_CTRL_socket.size() || !_ETH_CTRL_socket[nMCB]
        || !_ETH_CTRL_socket[nMCB]->is_open())
        return -1;
    _rbcp_packet_ID++;

    _rbcp_buf.ver_type = 0xFF;
    _rbcp_buf.cmd_flag = 0x00; //write
    _rbcp_buf.ID = _rbcp_packet_ID;
    _rbcp_buf.addr = htonl(addr);
    _rbcp_buf.length = len;
    //
    for(i=0;i<len;i++) _rbcp_buf.buf[i] = *(pdata+i);
    //////////////////////////////////////////////////
    //�ظ�ִ�д���
    INT8U  RePrcCnt = 0;
    //////////////////////////////////////////////////
    do{

        struct tcp_info info;
        int leninfo = sizeof(info);
        getsockopt(_ETH_CTRL_socket[nMCB]->native_handle(), IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&leninfo);

        if (info.tcpi_state == TCP_ESTABLISHED && info.tcpi_lost==0 )
        {


        }
        else
        {
            printf("Connection closed\n");
            return -1;
        }


        boost::system::error_code ec;
        int buflength = len+8;
        loger::getInstance()->writeLog(Info,"_ETH_CTRL_socket[nMCB]->write_some  begin ");
        tr_len = _ETH_CTRL_socket[nMCB]->write_some(buffer((void*)&_rbcp_buf,buflength), ec);
        loger::getInstance()->writeLog(Info,"_ETH_CTRL_socket[nMCB]->write_some  end ");
        if(ec)
        {
            std::cout<<boost::system::system_error(ec).what()<< std::endl;
            //return -1;
        }

        //
        if(tr_len < 0)
        {
            wr_err_cnt++;
            if(wr_err_cnt > gWrRdReptN)
            {
                break;
            }
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            Sleep(10);
#else
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
        }
        RePrcCnt++;
    }while((tr_len == -1) && (RePrcCnt < gCmdReptN));

    //
    if(tr_len != len+8)
    {
        printf("Failed 01 to send data to TCP control port (tx len = %d).\n",tr_len);
        loger::getInstance()->writeLog(Error,"Failed 01 to send data to TCP control port (tx len = " +std::to_string(tr_len));
        return -1;
    }
    //
    return 0;
}

//Return value: 0 = No error, others = Find errors.
INT8U  ETH_CTRL_read(INT32U  addr,INT8U * pdata, INT8U  len, unsigned short nMCB)
{
    /* FIX: also reject null slot / closed socket (was bare UB) */
    if (nMCB >= _ETH_CTRL_socket.size() || !_ETH_CTRL_socket[nMCB]
        || !_ETH_CTRL_socket[nMCB]->is_open())
    {
        return -1;
    }
    INT32S  i;
    INT32S  tr_len =0;

    _rbcp_packet_ID++;

    _rbcp_buf.ver_type = 0xFF;
    _rbcp_buf.cmd_flag = 0xC0; //read
    _rbcp_buf.ID = _rbcp_packet_ID;
    _rbcp_buf.addr = htonl(addr);
    _rbcp_buf.length = len;
    //////////////////////////////////////////////////
    //�ظ�ִ�д���
    INT8U  RePrcCnt = 0;
    //////////////////////////////////////////////////
    ///
    do
    {
        //std::cout << " ETH_CTRL_read  PID : "  << std::this_thread::get_id() << std::endl;
//        tr_len = write(_ETH_CTRL_socket[nMCB], (const char *)(&_rbcp_buf), 8 );
//        for(int i=0;i<8;i++)
//        {
//            fflush(stdout);
//            write(_ETH_CTRL_socket[nMCB], (const char *)(&_rbcp_buf)+i, 1 );
//            usleep(5);
//            tr_len++;
//        }

        struct tcp_info info;
        int leninfo = sizeof(info);
        getsockopt(_ETH_CTRL_socket[nMCB]->native_handle(), IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&leninfo);

        if (info.tcpi_state == TCP_ESTABLISHED && info.tcpi_lost==0 )
        {


        }
        else
        {
            printf("Connection closed\n");
            loger::getInstance()->writeLog(Error,"Connection closed");
            return -1;
        }

        boost::system::error_code ec;
        loger::getInstance()->writeLog(Info,"_ETH_CTRL_socket[nMCB]->write_some  begin  ");
        tr_len = _ETH_CTRL_socket[nMCB]->write_some(buffer((void*)&_rbcp_buf,8), ec);
        loger::getInstance()->writeLog(Info,"_ETH_CTRL_socket[nMCB]->write_some  end  ");
        if(nMCB >0)
        {
            int  n=0;
        }
        if(ec)
        {
            std::cout<< " ETH_CTRL_read ####### :  " << boost::system::system_error(ec).what()<< std::endl;
            exit(-1);
            //return -1;
        }
        //
        if(tr_len != 8)
        {
            printf("Failed 02 to send data to TCP control port (tx len = %d mcb_num = %d).\n",tr_len,nMCB);
            exit(-1);
            return -1;
        }

        //
       // fflush(stdout);
        //delay_in_millisecond(10);
       // usleep(10*1000);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        //

       // tr_len = recv(_ETH_CTRL_socket[nMCB], (char *)(&_rbcp_buf),len+8,0);
//        while (_ETH_CTRL_socket[nMCB]->available()<len+8) {
//            std::this_thread::sleep_for(std::chrono::milliseconds(6));
//        }

        loger::getInstance()->writeLog(Info,"_ETH_CTRL_socket[nMCB]->read_some  begin ");
        if(_ETH_CTRL_socket[nMCB]->available()>0)
        {
            tr_len = _ETH_CTRL_socket[nMCB]->read_some(buffer(&_rbcp_buf,len+8),ec);
        }
        else
        {
            loger::getInstance()->writeLog(Error,"_ETH_CTRL_socket["+ std::to_string(nMCB)+ "]->read_some  end nodata");
            return -2;
        }
        loger::getInstance()->writeLog(Info,"_ETH_CTRL_socket[nMCB]->read_some  end ");
        if(ec)
        {
            std::cout<<boost::system::system_error(ec).what()<< std::endl;
            //return -1;
        }

        //
        if ( tr_len > 0 )
        {
            //printf("Bytes received: %d\n", tr_len);
        }
        else if ( tr_len == 0 )
        {
            printf(" tr_len == 0 \n");
            //loger::getInstance()->writeLog(Info," tr_len == 0");
            return -1;
        }
        else
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            printf("recv failed: %d\n", WSAGetLastError());

        //
        i = WSAGetLastError();
#else
        {
            printf("recv failed: %d\n", errno);
//            struct tcp_info info;
//            int leninfo = sizeof(info);
//            getsockopt(_ETH_CTRL_socket[nMCB], IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&leninfo);

//            if (info.tcpi_state == TCP_ESTABLISHED && info.tcpi_lost==0 )
//            {
//                if(errno == 11)
//                {
//                    //usleep(10*1000);
//                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
//                    tr_len = recv(_ETH_CTRL_socket[nMCB], (char *)(&_rbcp_buf),len+8,0);
//                }

//            }
            //return -1;
        }

#endif
        //
        RePrcCnt++;
    }while((tr_len == -1) && (RePrcCnt < gCmdReptN));

    if(tr_len != len+8)
    {
        printf("Failed 03 to send data to TCP control port (rx len = %d).\n",tr_len);
        loger::getInstance()->writeLog(Error,"Failed 03 to send data to TCP control port (rx len = "+ std::to_string(tr_len)+")");
        return -1;
    }


    //do
    //{
    //	tr_len = send(_ETH_CTRL_socket[nMCB], (const char *)(&_rbcp_buf), 8 , 0);
    //}
    //while(tr_len == -1);
    //
    //if(tr_len != 8)
    //{
    //	printf("Failed to send data to TCP control port (tx len = %d).\n",tr_len);
    //	return -1;
    //}

    //do{
    //	tr_len = recv(_ETH_CTRL_socket[nMCB], (char *)(&_rbcp_buf),len+8, 0);
    //	//
 //       if ( tr_len > 0 )
 //           printf("Bytes received: %d\n", tr_len);
 //       else if ( tr_len == 0 )
 //           printf("Connection closed\n");
 //       else
 //           printf("recv failed: %d\n", WSAGetLastError());
    //	//
    //	i = WSAGetLastError();
    //}
    //while(tr_len == -1);
    //if(tr_len != len+8)
    //{
    //	printf("Failed to send data to TCP control port (rx len = %d).\n",tr_len);
    //	return -1;
    //}
    //
    //loger::getInstance()->writeLog(Info," _rbcp_buf.buf : ");
    for(i=0;i<len;i++)
    {
        *(pdata+i) = _rbcp_buf.buf[i] ;
        //loger::getInstance()->writeLog(Info,std::to_string(_rbcp_buf.buf[i]) + ",");
    }
    if((_rbcp_buf.cmd_flag & 0x1) != 0)
    {
        printf("ECS read error: time out!\n");
        loger::getInstance()->writeLog(Error,"ECS read error: time out! ");
        return -1;
    }
    if((_rbcp_buf.cmd_flag & 0x2) != 0)
    {
        printf("ECS read error: wrong address! Read addr = 0x%08X, return addr = 0x%08X\n",addr,_rbcp_buf.addr);
        loger::getInstance()->writeLog(Error,"ECS read error: wrong address! Read addr = "+ std::to_string(addr)+ "return addr = " + std::to_string(_rbcp_buf.addr) );
        return -1;
    }

    return 0;

}


//Read data
//parameter:
//	timeout: ��ȴ�ʱ�䡣Ϊ0��ʾ��ȴ�ʱ�����������ֵ��ʾ��Ӧ����ȴ�ʱ�䣬��λS
//return��
//  �����ָ�����ȵ����ݶ�ȡ������ʱ���������ء���ʱ������ֵ���ڲ���len��
//  ����-1����ʾ����ִ�г�����

INT32S ETH_Data_Read( INT8U * buf, INT32S len, INT32U timeout, FILE* fp)
{
    INT32U s_t,c_t,d_t;
    INT32S rd_len,size;
    s_t = (INT32U) time(0);
    //
    time_t t_st;
    DWORD tTick1 = NucTool::GetTickCount();
    DWORD tTick2 = NucTool::GetTickCount();
    //
    rd_len = 0;  //�Ѷ�ȡ���ݳ���
    size = 0;  //recv(����������һ�ζ�ȡ�����ݳ���

    while(1)
    {
        size = recv(_ETH_Data_socket, (char*)&buf[rd_len], len-rd_len, 0);

        if(size < 0 )
        {
            size = size;
            tTick2 = NucTool::GetTickCount();
            if((tTick2 - tTick1) > 100)
            {
                tTick1 = tTick2;
                //printf("Socket buffer is empty!\n");
                if(fp)
                {
                    time(&t_st);
                    struct tm * timeinfo = localtime(&t_st);
                    struct timespec ts;
                    clock_gettime(CLOCK_MONOTONIC, &ts);
                    //fprintf(fp,"Socket buffer is empty!\n");
                    fprintf(fp, "%d-%02d-%02d %02d:%02d:%02d.%03d Socket buffer is empty! recv failed: %d\n",
                            timeinfo->tm_year, timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,  ts.tv_nsec/1000000 , errno);
                }
            }
        }
        else if(size == 0 )
        {
            printf("Error when calling ETH_Data_Read(). Possible reason: the socket has been closed!\n");
            if(fp)
                fprintf(fp, "Error when calling ETH_Data_Read(). Possible reason: the socket has been closed!\n");
            return -1;
        }
        else
        {
            rd_len = rd_len + size;
            /////////////////////////////////
            if(fp)
            {
                time(&t_st);
                struct tm * timeinfo = localtime(&t_st);
                struct timespec ts;
                clock_gettime(CLOCK_MONOTONIC, &ts);
                //fprintf(fp,"Socket buffer is empty!\n");
                fprintf(fp, "%d-%02d-%02d %02d:%02d:%02d.%03d  The number of bytes received is: %d\n",
                        timeinfo->tm_year + 1900 , timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,  ts.tv_nsec/1000000 , size);
            }
            /////////////////////////////////
            if (rd_len == len)
                return rd_len;
        }

        c_t = (INT32U) time(0);
        d_t = c_t - s_t;
        if (d_t > timeout)
        {
            printf("Timeout error when calling ETH_Data_Read()\n");
            if(fp)	fprintf(fp, "Timeout error when calling ETH_Data_Read()\n");
            return -1;
        }
    }
}

//
//������_ETH_Data_socket���ݶ˿ڶ�ȡ���ݣ�ֱ����ջ����е����ݡ�
INT32S ETH_DATA_read_buffer_residual_data(unsigned short nMCB)
{
    INT8U  buf[512];
    INT32S rd_size, rd_err_cnt=0;
    while(1){
        //recv ���������ӵ��׽��ֻ�󶨵��������׽��ֽ�������
        //���δ�������� recv �������յ����ֽ����� buf ����ָ��Ļ��������������յĴ����ݡ�
        //��������������رգ��򷵻�ֵΪ�㡣
        //���򣬽�����SOCKET_ERRORֵ�����ҿ���ͨ������ WSAGetLastError �������ض��Ĵ�����롣
        rd_size = recv(_ETH_Data_socket, (char*)buf, 256, 0);  //��ȡ����
        if(rd_size <= 0 )
        {
            rd_err_cnt++;
            if(rd_err_cnt > 2)
            {
                return rd_size;
            }
            /////////////////////////
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            Sleep(10);
#else
             std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
        }
    }
}

//������_ETH_CTRL_socket��ȡ���ݣ�ֱ����ջ����е����ݡ�
INT32S ETH_CTRL_read_buffer_residual_data(unsigned short nMCB)
{
    /* FIX: guard against empty/null slot (was bare UB) */
    if (nMCB >= _ETH_CTRL_socket.size() || !_ETH_CTRL_socket[nMCB])
        return -1;
    INT8U  buf[512];
    INT32S rd_size, rd_err_cnt=0;
    while(1){
        //recv ���������ӵ��׽��ֻ�󶨵��������׽��ֽ�������
        //���δ�������� recv �������յ����ֽ����� buf ����ָ��Ļ��������������յĴ����ݡ�
        //��������������رգ��򷵻�ֵΪ�㡣
        //���򣬽�����SOCKET_ERRORֵ�����ҿ���ͨ������ WSAGetLastError �������ض��Ĵ�����롣
        //rd_size = recv(_ETH_CTRL_socket[nMCB], (char*)buf, 256, 0);  //��ȡ����

        struct tcp_info info;
        int leninfo = sizeof(info);
        getsockopt(_ETH_CTRL_socket[nMCB]->native_handle(), IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&leninfo);

        if (info.tcpi_state == TCP_ESTABLISHED && info.tcpi_lost==0 )
        {


        }
        else
        {
            printf("Connection closed\n");
            loger::getInstance()->writeLog(Error,"Connection closed ");
            return -1;
        }

        boost::system::error_code ec;
        if(_ETH_CTRL_socket[nMCB]->available()<=0) return 0;
        //loger::getInstance()->writeLog(Info,"_ETH_CTRL_socket[nMCB]->read_some  begin");
        rd_size = _ETH_CTRL_socket[nMCB]->read_some(buffer(&buf,256),ec);
        //loger::getInstance()->writeLog(Info,"_ETH_CTRL_socket[nMCB]->read_some  end");
        if(ec)
        {
            std::cout<<boost::system::system_error(ec).what()<< std::endl;
            return -1;
        }
        //std::cout << "ETH_CTRL_read_buffer_residual_data" << std::endl;
        if(rd_size <= 0 )
        {
            rd_err_cnt++;
            if(rd_err_cnt > 2)
            {
                return rd_size;
            }
            /////////////////////////
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            Sleep(10);
#else
            //usleep(10*1000);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
#endif
        }
    }
}


bool checksocket_status(unsigned short nMCB)
{
    /* FIX: guard against empty/null slot (was bare UB) */
    if (nMCB >= _ETH_CTRL_socket.size() || !_ETH_CTRL_socket[nMCB])
        return false;
    struct tcp_info info;
    int leninfo = sizeof(info);
    getsockopt(_ETH_CTRL_socket[nMCB]->native_handle(), IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&leninfo);
    if (info.tcpi_state == TCP_ESTABLISHED && info.tcpi_lost==0 )
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool ETH_CTRL_IsValid(unsigned short nMCB)
{
    // return ( (_ETH_CTRL_socket.size()>0 && _ETH_CTRL_socket[nMCB] != NULL && (_ETH_CTRL_socket[nMCB]->is_open())) ? 0 : 1);

    // 确保 nMCB 在 vector 范围内
    if (nMCB >= _ETH_CTRL_socket.size()) {
        std::cerr << "ETH_CTRL_IsValid: Index " << nMCB << " out of range!" << std::endl;
        return false;
    }

    // 确保指针有效
    if (_ETH_CTRL_socket[nMCB] == nullptr) {
        std::cerr << "ETH_CTRL_IsValid: Socket pointer is NULL for index " << nMCB << std::endl;
        return false;
    }

    // 确保 socket 仍然是打开的
    if (!_ETH_CTRL_socket[nMCB]->is_open()) {
        std::cerr << "ETH_CTRL_IsValid: Socket at index " << nMCB << " is closed" << std::endl;
        return false;
    }

    return true;
}

