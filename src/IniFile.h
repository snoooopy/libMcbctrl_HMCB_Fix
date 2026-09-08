/*****************************************************************
 
	文件名:读取当前目录下CIniFile类文件
	日期:2004-1-18
	作者:chao_jie
	E_Mail:chaojie2003@yahoo.com.cn
 
	功能:系统提供的WriteProfileInt()等函数，写到注册表或
		 系统ini文件里，调用比较麻烦，而且会占用注册表资
		 源，下载了网上的ini类用起来又不太顺手，所以参照
		 写了一个读取当前目录下ini文件的类。程序有详细的
		 注释，欢迎批评指正。
	修改：WB Zhu，2005-4-21

*****************************************************************/

/*//////////////////////////////////////////////////////////////////
使用方法:
1.先把IniFile.h，IniFile.ccp加入Porject，在要使用该类的头文件中加入
	#include "IniFile.h"，及CIniFile IniFile;

2.读取和写入:
	string = IniFile.GetString("section","item","默认");
	int = IniFile.GetInt("section","item",666);

	IniFile.SetString("section","item","设置值");
	IniFile.SetInt("section","item",666);

3.ini文件名及路径在初始化时，用SetFileName()设置

注意事项:ini文件如果太长，读取速度会变慢，可以使用多个ini文件

//////////////////////////////////////////////////////////////////*/
// IniFile.h: interface for the CIniFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_INIFILE_H__33CAF5C6_7820_43EB_8D84_42CC1856C35C__INCLUDED_)
#define AFX_INIFILE_H__33CAF5C6_7820_43EB_8D84_42CC1856C35C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include<iostream>
#include<fstream>
#include <list>
#include <vector>
class CIniFile  
{
public:
    void SetFileName(std::string lpszFileName);

    void WriteFileString(std::string Section, std::string Item, std::string Value);
    std::string GetFileString(std::string Section, std::string Item, std::string Value);

    std::string GetString(std::string Section, std::string Item, std::string Value);
    int GetInt(std::string Section, std::string Item, int Value);
	//
    double GetDouble(std::string Section, std::string Item, double dbValue);


    void SetString(std::string Section, std::string Item, std::string Value);
    void SetInt(std::string Section, std::string Item, int Value);

    bool WriteIniFile(std::string lpszFileName);
    bool ReadIniFile();

	CIniFile();
	virtual ~CIniFile();

	//ini file
    std::string m_sFileName;
    std::fstream IniFile;
    bool  bFileExist;
    std::vector<std::string> FileContainer;

protected:
    int ConvertHexCharToInt(char ch);
public:
    void Right(std::string & pstr,const std::string & sign);
    void Left(std::string & pstr,const std::string & sign);
    void Left(std::string & pstr,const int & pos);
    void trim_right(std::string& pstr);
    void trim_left(std::string& pstr);
};

#endif // !defined(AFX_INIFILE_H__33CAF5C6_7820_43EB_8D84_42CC1856C35C__INCLUDED_)
