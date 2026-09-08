// IniFile.cpp: implementation of the CIniFile class.
//
//////////////////////////////////////////////////////////////////////
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include "stdafx.h"
#else

#endif
#include "IniFile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define DEBUG_NEW new
#endif

#ifdef VC6_BUILD		 // Build project with VC6 
#define _tstoi _ttoi  // use atoi when use VC6
#endif

#include <algorithm>
#include <numeric>
#include <string.h>
#include <dirent.h>
#include <sys/io.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIniFile::CIniFile()
{
	m_sFileName = "ini.ini";
}

CIniFile::~CIniFile()
{
    if(FileContainer.size() > 0)
	{
        FileContainer.clear();
	}
}

void CIniFile::SetFileName(std::string lpszFileName)
{
	m_sFileName = lpszFileName;
	ReadIniFile();
}

void CIniFile::SetInt(std::string Section, std::string Item, int Value)
{
    std::string strtemp;
    strtemp = std::to_string(Value);
	//���浽FileContainer
	WriteFileString(Section, Item, strtemp);
	//���浽ini�ļ�
//	WriteIniFile();
}

void CIniFile::SetString(std::string Section, std::string Item, std::string Value)
{
	//���浽FileContainer
	WriteFileString(Section, Item, Value);
	//���浽ini�ļ�
//	WriteIniFile();
}

std::string CIniFile::GetString(std::string Section, std::string Item, std::string Value)
{
	return GetFileString(Section, Item, Value);
}

int CIniFile::GetInt(std::string Section, std::string Item, int Value)
{
    std::string strtemp, str;
    strtemp =std::to_string(Value);
	str =GetFileString(Section, Item, strtemp); // return atoi()
    if (str.length()>2 && ((str[0]=='0')&& (str[1]=='x' || str[1]=='X')) ) //HEX Value
	{
		int vtmp, v =0;
        for (int i=2; i<str.length(); i++)
		{
			vtmp = ConvertHexCharToInt(char(str[i]));
			if (vtmp ==-1)
			{
				break;
			}
			v = (v<<4) +vtmp;
		}
		return v;
	}
	else //not HEX value
	{
        return atoi(str.c_str());
	}
}


double CIniFile::GetDouble(std::string Section, std::string Item, double dbValue)
{
    std::string strtemp, str;
    strtemp = std::to_string(dbValue);
	str = GetFileString(Section, Item, strtemp); // return atoi()
    return atof(str.c_str());
}


bool CIniFile::ReadIniFile()
{
    if(access(m_sFileName.c_str(),F_OK)!=0)
    {

        return false;
    }

    IniFile.open(m_sFileName, std::ios::in);

    if(!IniFile.is_open())
	{

        return false;
	}

    std::string strline;
    FileContainer.clear();

	//��IniFile�ļ����ݶ���FileContainer
    char tmp[1024] = {0};
    while(IniFile.getline(tmp,sizeof(tmp)))
    {
        strline = tmp;
        memset(tmp,0,sizeof(tmp));
        trim_left(strline);
		if (strline !="")
		{
            //strline.MakeUpper();
            FileContainer.push_back(strline);
		}
	}

    IniFile.close();
    return true;
}

bool CIniFile::WriteIniFile(std::string lpszFileName)
{
    if(access(m_sFileName.c_str(),F_OK)!=0)
    {
        return false;
    }

    IniFile.open(lpszFileName, std::ios::out|std::ios::trunc);
    if(!IniFile.is_open())
        return false;
	
    std::string strline;

	//��FileContainerд��IniFile�ļ�
    for(int i = 0; i< FileContainer.size(); i++)
	{
		strline = FileContainer[i];
		if (strline !="")
		{
            if(strline.at(0)=='[' && i!=0)
			{
                IniFile<<"\n";
			}
            IniFile<<strline+"\n";
		}
	}

    IniFile.close();

    return true;
}

std::string CIniFile::GetFileString(std::string Section, std::string Item, std::string Value)
{
	//ReadIniFile();//�����ļ�

    trim_left(Section);
    trim_left(Item);

    if(FileContainer.size() < 0 || Section =="" || Item =="")
		return Value;//�ļ��򿪳������ļ�Ϊ�գ�Section��Item�գ�����Ĭ��ֵ

    int i = 0;
    int iFileLines = FileContainer.size();
    std::string strline,str;

	while(i<iFileLines)
	{
        strline = FileContainer.at(i++);

        trim_left(strline);
/*
		if (strline =="")
		{
			FileContainer.RemoveAt(i-1);
			i--;
			iFileLines = FileContainer.GetSize();
			continue;
		}
*/
        if(strline.at(0)=='[')//����Section����һ������Ϊ[
		{
            str = strline;
            Left(str,"]");//ȥ��]�ұ�
            Right(str,"[");//ȥ��[����

            trim_left(str);
            trim_right(str);

			if(Section == str)//�ҵ�Section
			{
				while(i<iFileLines)
				{
                    strline = FileContainer.at(i++);
                    trim_left(strline);

/*
					if (strline =="")
					{
						FileContainer.RemoveAt(i-1);
						i--;
						iFileLines = FileContainer.GetSize();
						continue;
					}
*/
                    if(strline.at(0)=='[')
                        return Value;//����������һ��[]�����Ҳ���,����Ĭ��ֵ
                    str = strline;
                    Left(str,"=");//ȥ��=�ұ�

                    trim_left(str);
                    trim_right(str);

					if(Item == str)//�ҵ�Item
					{
                        str = strline;
                        Right(str,"=");//ȥ��=����

                        trim_left(str);
                        trim_right(str);

						if (str =="")
						{
							str =Value;
						}

						return str;
					}
				}
				return Value;//�Ҳ���,����Ĭ��ֵ
			}

		}

	}
	return Value;//�Ҳ���,����Ĭ��ֵ
}

void CIniFile::WriteFileString(std::string Section, std::string Item, std::string Value)
{
    int i = 0;
    int iFileLines = FileContainer.size();
    std::string strline,str;

    trim_left(Section);
    trim_left(Item);

	if(Section =="" || Item =="")
		return ;//��Section��Item������

	while(i < iFileLines)
	{
        strline = FileContainer.at(i++);

        trim_left(strline);
/*
		if (strline =="")
		{
			continue;
		}
*/
        if(strline.at(0)=='[')//����Section����һ������Ϊ[
		{
            str = strline;
            Left(str,"]");
            Right(str,"[");
            trim_left(str);
            trim_right(str);

			if(Section == str)//�ҵ�Section
			{
				while(i<iFileLines)
				{
                    strline = FileContainer.at(i++);
                    //strline.TrimLeft();
                    trim_left(strline);

/*
					if (strline =="")
					{
						continue;
					}
*/
                    if(strline.at(0)=='[')//����������һ��[]�����Ҳ���Item
					{
                        strline = Item + "=" + Value;
                        FileContainer.insert(FileContainer.begin() + (i-1), strline);
						return;
					}
					
                    Left(strline,"=");
                    str = strline;

                    trim_left(str);
                    trim_right(str);

					if(Item == str)//�ҵ�Item
					{
                        strline = Item + "=" + Value;
						FileContainer[i-1] = strline;
						return;
					}
				}
				//�Ҳ���Item
                strline = Item + "=" + Value;
                FileContainer.insert(FileContainer.begin()+i,strline);
				return;

			}

		}

	}
	//�Ҳ���Section
	//ֱ������������Section��Item��Value
    FileContainer.push_back("[" + Section + "]");
    FileContainer.push_back(Item + "=" + Value);
	return;
}


int CIniFile::ConvertHexCharToInt(char ch)
{
	if ((ch>='0')&&(ch<='9'))  return ch-0x30; 
	else if ((ch>='A')&&(ch<='F'))  return ch-'A'+10; 
	else if ((ch>='a')&&(ch<='f'))  return ch-'a'+10; 
	else return (-1);
}

void CIniFile::trim_left(std::string& pstr)
{
    std::string l_strtemp;
    std::string::iterator pfind = std::find_if(pstr.begin(),pstr.end(),[=](const char ch){
        if(ch != ' '&&ch != '\n'&&ch != '\t'&&ch != '\r')
        {
            return true;
        }
        return false;
    });

    if(pfind!=pstr.end())
    {
        l_strtemp.assign(pfind,pstr.end());
        pstr = l_strtemp;
    }
}

void CIniFile::trim_right(std::string& pstr)
{
    std::string l_strtemp;
    std::string::reverse_iterator pfind = std::find_if(pstr.rbegin(),pstr.rend(),[=](const char ch){
        if(ch != ' '&&ch != '\n'&&ch != '\t'&&ch != '\r')
        {
            return true;
        }
        return false;
    });

    if(pfind!=pstr.rend())
    {
        std::string::iterator pend(pfind.base());
        l_strtemp.assign(pstr.begin(),pend);
        pstr = l_strtemp;
    }
}
void CIniFile::Left(std::string & pstr,const std::string & sign)
{
    std::string l_strtemp="";
    auto pos= pstr.find(sign);
    if(pos==std::string::npos)
    {

    }
    else
    {
        l_strtemp.assign(pstr.begin(),pstr.begin()+pos);
        pstr = l_strtemp;
    }
}

void CIniFile::Left(std::string &pstr, const int &pos)
{
    std::string l_strtemp="";
    if(pstr == "") return ;
    l_strtemp.assign(pstr.begin(),pstr.begin()+pos);
    pstr = l_strtemp;
}

void CIniFile::Right(std::string & pstr,const std::string & sign)
{
    std::string l_strtemp="";
    auto pos= pstr.find(sign);
    if(pos==std::string::npos)
    {

    }
    else
    {
        l_strtemp.assign(pstr.begin()+pos+1,pstr.end());
        pstr = l_strtemp;
    }
}
