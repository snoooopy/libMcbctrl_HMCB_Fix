/***
* nucutil.h - 完成字符串转换功能，命名空间_nuc_util,命名空间中默认的代码页为65001(UTF-8)
* author:wang qiang	
* company:nuctech
****/

#if _MSC_VER > 1000
#pragma once
#endif

#define WCH2ACH WChar2AChar
#define ACH2WCH AChar2WChar

#ifndef _INC_CNUCSTRCONVERTER_INCLUDED
#define _INC_CNUCSTRCONVERTER_INCLUDED

namespace _nuc_util 
{
UINT const CodePage = 65001;		//默认的代码页utf-8

class CNUCStrConverter
{
public:	
	CNUCStrConverter()
	{
		m_wstr = NULL;
		m_str = NULL;
		m_pWChar = NULL;
		m_pChar = NULL;

	};
	~CNUCStrConverter()
	{
		if (m_wstr != NULL) 
		{
			::SysFreeString(m_wstr);
			m_wstr = NULL;
		}

		if (m_str != NULL) 
		{
			delete [] m_str;
			m_str = NULL;
		}

		if(m_pChar != NULL)
		{
			delete [] m_pChar;
			m_pChar = NULL;
		}

		if(m_pWChar != NULL)
		{
			delete [] m_pWChar;
			m_pWChar = NULL;
		}
	};

	//Convert char * to BSTR 
	/*  Params:
	 *  pSrc				源字符串    		[in] 
	 *  CodePage			代码页	            [in] default value is CP_ACP:系统默认的代码页
	 *
	 *  Return Values:
	 *  BSTR				
	 *  
	 *  Notes: 在对象析构时为返回值BSTR分配的内存会被释放掉,另外同一个对象第二次调用该方法时,上一次返回的BSTR分配的内容会被释放掉
	 *         所以调用者每次调用该方法时，需要小心，以免出现野指针。这样做的目的是不需要调用者释放内容，以免调用者忘记 造成内存泄露 。
	 */
	inline BSTR ConvertStringToBSTR(const char* pSrc, UINT CodePage=CP_ACP)
	{
		if(!pSrc) return NULL;
		
		if (m_wstr != NULL) 
		{
			::SysFreeString(m_wstr);
			m_wstr = NULL;
		}
		
		DWORD cwch;		

		if(cwch = ::MultiByteToWideChar(CodePage, 0, pSrc, -1, NULL, 0))//get size minus NULL terminator
		{
			cwch--;
			m_wstr = ::SysAllocStringLen(NULL, cwch);
			
			if(m_wstr)
			{
				if(!::MultiByteToWideChar(CodePage, 0, pSrc, -1, m_wstr, cwch))
				{
					if(ERROR_INSUFFICIENT_BUFFER == ::GetLastError())
						return m_wstr;
					::SysFreeString(m_wstr);//must clean up
					m_wstr = NULL;
				}
			}			
		}
		
		return m_wstr;
	};

	//Convert BSTR to char *  
	/*  Params:
	 *  pSrc				源字符串    		[in] 
	 *  CodePage			代码页	            [in] default value is CP_ACP:系统默认的代码页
	 *
	 *  Return Values:
	 *  char*				
	 *  
	 *  Notes: 在对象析构时为返回值char*分配的内存会被释放掉,另外同一个对象第二次调用该方法时,上一次返回的char*分配的内容会被释放掉
	 *         所以调用者每次调用该方法时，需要小心，以免出现野指针。这样做的目的是不需要调用者释放内容，以免调用者忘记 造成内存泄露 。
	 */
	inline char* ConvertBSTRToString(BSTR pSrc, UINT CodePage=CP_ACP)
	{			
		if(!pSrc) return NULL;

		if(m_str != NULL)
		{
			delete [] m_str;
			m_str = NULL;
		}
		
		DWORD cb, cwch = ::SysStringLen(pSrc);//convert even embeded NULL		
		
		if(cb = ::WideCharToMultiByte(CodePage, 0, pSrc, cwch + 1, NULL, 0, 0, 0))
		{
			m_str = new char[cb];
			if(m_str)
			{
				m_str[cb - 1]  = '\0';
				
				if(!::WideCharToMultiByte(CodePage, 0, pSrc, cwch + 1, m_str, cb, 0, 0))
				{
					delete []m_str;//clean up if failed;
					m_str = NULL;
				}
			}
		}

		return m_str;
	};

	//Convert wchar * to char *  
	/*  Params:
	 *  pSrc				源字符串    		[in] 
	 *  CodePage			代码页	            [in] default value is CP_ACP:系统默认的代码页
	 *
	 *  Return Values:
	 *  char*				
	 *  
	 *  Notes: 在对象析构时为返回值char*分配的内存会被释放掉,另外同一个对象第二次调用该方法时,上一次返回的char*分配的内容会被释放掉
	 *         所以调用者每次调用该方法时，需要小心，以免出现野指针。这样做的目的是不需要调用者释放内容，以免调用者忘记 造成内存泄露 。
	 */

	inline char* WChar2AChar(const WCHAR *pSrc, UINT CodePage=CP_ACP)
	{			
		if(!pSrc) return NULL;
		
		if(m_pChar != NULL)
		{
			delete [] m_pChar;
			m_pChar = NULL;
		}

		int nlen = 	WideCharToMultiByte(CodePage,
								0,
								pSrc,
								-1, 
								NULL,
								0,
								NULL,
								NULL);

		//if alen == 0, some unknown codepage was probably used.
		if(nlen == 0) 
		{
			return NULL;
		}

		//Use mfc convert routine
		m_pChar = new char[nlen+2]; 		

		WideCharToMultiByte(CodePage,
							0,
							pSrc,
							-1,
							m_pChar,
							nlen, 
							NULL,
							NULL
							);	
		m_pChar[nlen] = '\0';
		return m_pChar;
	};



	//Convert char * to wchar* 
	/*  Params:
	 *  pSrc				源字符串    		[in] 
	 *  CodePage			代码页	            [in] default value is CP_ACP:系统默认的代码页
	 *
	 *  Return Values:
	 *  WCHAR*				
	 *  
	 *  Notes: 在对象析构时为返回值WCHAR*分配的内存会被释放掉,另外同一个对象第二次调用该方法时,上一次返回的WCHAR*分配的内容会被释放掉
	 *         所以调用者每次调用该方法时，需要小心，以免出现野指针。这样做的目的是不需要调用者释放内容，以免调用者忘记 造成内存泄露 。
	 */
	inline WCHAR* AChar2WChar(const char* pSrc, UINT CodePage=CP_ACP)
	{
		if(!pSrc) return NULL;
		
		if (m_pWChar != NULL) 
		{
			delete [] m_pWChar;
			m_pWChar = NULL;
		}

		//Use api convert routine
		int nLen = MultiByteToWideChar( CodePage,
										0,
										pSrc,
										-1,
										NULL,
										0);

		//if wlen == 0, some unknown codepage was probably used.		
		if(nLen == 0)
		{
			return NULL;
		}

		m_pWChar = new WCHAR[nLen+2];

		MultiByteToWideChar(CodePage,
							0,
							pSrc,
							-1,
							m_pWChar,
							nLen);
		m_pWChar[nLen] = _T('\0');
		
		return m_pWChar;
	};
	
private:
	wchar_t*		m_wstr;
	char*			m_str;
	WCHAR *			m_pWChar;
	char*			m_pChar;
};

}

#endif

// _INC_COMUTIL
