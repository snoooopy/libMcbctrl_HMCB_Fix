#include "logAdapter.h"
#include <mutex>
#include <iostream>
#include <sys/prctl.h>
#include <filesystem>
#include <cstring>
#include <limits.h>
//#include <source_location>
loger* loger::m_instance = NULL;
//std::mutex m_mutex;
extern std::string  g_LogFilePath;
 extern bool m_bMCBxInInitialing;
logAdapter::logAdapter(/*const std::string &ip, int port,int model*/)
{

        char *strpath = getenv("USE_HMCB_TEST_LOG") ;

        if(strpath !=NULL)
        {
            std::string strEnv = strpath;
            m_Write = (strEnv=="1");
        }

        if(!m_Write) return;
        std::string str = "HMCB_TEST";
        str = (g_LogFilePath.empty()?"/home/nuctech/":g_LogFilePath) + str + ".log";
        if(std::filesystem::exists(str))
        {
            if (std::filesystem::file_size(str) > 20*1024*1024)
            {
                std::filesystem::remove(str+".bak");
                std::filesystem::copy_file(str, str+".bak");
                std::filesystem::remove(str);
            }
        }

        m_fd.open(str,std::ios::out|std::ios::app);
}

int logAdapter::writeLog(logType type, const std::string & modename,const std::string & funcname, const std::string & msg)
{

        if(m_bMCBxInInitialing) return -1;
        if(!m_Write) return -1;
        if(!m_fd.is_open())
        {
            std::string str = "HMCB_TEST";
            str = (g_LogFilePath.empty()?"/home/nuctech/":g_LogFilePath) + str + ".log";
            m_fd.open(str,std::ios::out|std::ios::app);
        }
        std::string strType;
        switch (type) {
        case Info:
            strType = "Info";
            break;
        case Error:
            strType = "Error";
            break;
        case Debug:
            strType = "Debug";
            break;
        case Warn:
            strType = "Warn";
            break;
        default:
            strType = "Warn";
            break;
        }
        time_t t;
        time(&t);
        time_t ta = t;
        struct tm * timeinfo = localtime(&t)/*gmtime(&t)*/;
        char szUCT[50] = { 0 };
        strftime(szUCT, 50, "%Y-%m-%d %H:%M:%S", timeinfo);
        std::string strTime=asctime(timeinfo);
        m_fd << strType <<"   "  << szUCT/*strTime*/ << "  "<< modename << "  " << funcname << "  " << msg << std::endl;
        m_fd.flush();

    return 0;
}

loger *loger::getInstance()
{
    if(m_instance!=NULL)
    {
        return m_instance;
    }
    else
    {
        //std::unique<std::mutex> clock(std::move(m_mutex));
        if(m_instance == NULL)
        {
           m_instance = new loger();
           logAdapter * tmp = new logAdapter();
           m_instance->setAdapter(tmp);
        }
        return m_instance;
    }

}

void loger::deleteInstance()
{
    //std::unique<std::mutex> clock(std::move(m_mutex));
    if(m_instance!=NULL)
    {
        delete m_instance;
        m_instance = NULL;
    }
}

void loger::setAdapter(logAdapter *padapter)
{
    m_adapter = padapter;
}

int loger::writeLog(logType type, const std::string & msg ,const std::string funcname)
{
    char path[PATH_MAX]={0};
    char processname[1024]={0};
    get_executable_path(path,processname,sizeof(path));
    std::string modename = processname;
    m_adapter->writeLog(type,modename,funcname,msg);
    return 0;
}

size_t loger::get_executable_path( char* processdir,char* processname, size_t len)
{
        char* path_end;
        if(readlink("/proc/self/exe", processdir,len) <=0)
                return -1;
        path_end = strrchr(processdir,  '/');
        if(path_end == NULL)
                return -1;
        ++path_end;
        strcpy(processname, path_end);
        *path_end = '\0';
        return (size_t)(path_end - processdir);
}
