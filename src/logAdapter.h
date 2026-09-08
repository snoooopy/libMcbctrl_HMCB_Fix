#ifndef LOGADAPTER_H
#define LOGADAPTER_H
#include <iostream>
#include <memory>
#include <string>
//#include <grpcpp/grpcpp.h>
#include<fstream>
#include<iostream>
//#include <grpc/support/log.h>
#include <sys/prctl.h>
#include <unistd.h>

enum logType
{
    Info,
    Error,
    Debug,
    Warn
};

class logAdapter
{
public:
    logAdapter(/*const std::string & ip ="127.0.0.1",int port = 5050,int model =1*/);
    int writeLog(logType type, const std::string & modename,const std::string & funcname, const std::string & msg);
private:
    int  m_model =0;
    std::fstream m_fd;

    bool m_Write = false;
};

class loger
{
public:
    static loger* getInstance();
    static void deleteInstance();
    void setAdapter(logAdapter * padapter);
    int writeLog(logType type, const std::string & msg,const std::string funcname = __builtin_FUNCTION());
    size_t get_executable_path( char* processdir,char* processname, size_t len);
private:
    loger(){};
    logAdapter * m_adapter;
    static loger* m_instance;




    //static std::mutex m_mutex;
};
#endif // LOGADAPTER_H
