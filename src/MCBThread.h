#if !defined(AFX_MCBTHREAD_H__1AFBE446_D98D_4D04_98BB_8C30B58CEBB2__INCLUDED_)
#define AFX_MCBTHREAD_H__1AFBE446_D98D_4D04_98BB_8C30B58CEBB2__INCLUDED_

#include "MCBCtrl.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MCBThread.h : header file
//
#include <iostream>
#define WM_MCBTHREAD_CMD   (WM_USER +0x200)
#define WM_MCBTHREAD_TIMER (WM_USER +0x201)
/////////////////////////////////////////////////////////////////////////////
// CMCBThread thread
class CMCBCtrlC;
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
class CMCBThread : public CWinThread
{
    DECLARE_DYNCREATE(CMCBThread)
#else
class CMCBThread
{
#endif
public:
	CMCBThread();           // protected constructor used by dynamic creation

// Attributes

    CMCBCtrlC* m_pMCB = NULL;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMCBThread)
	public:
    virtual bool InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
protected:
#else
public:
#endif
	virtual ~CMCBThread();

	// Generated message map functions
	//{{AFX_MSG(CMCBThread)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	afx_msg void MCBCommand(WPARAM wParameter, LPARAM lParameter);
    afx_msg void MCBTimer(WPARAM wParam,LPARAM lParam);

    DECLARE_MESSAGE_MAP()

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
#else
public:
    void MCBCommand(WPARAM wParameter, LPARAM lParameter);
    void MCBTimer(WPARAM wParam,LPARAM lParam);
#endif
public:
    //°ÑÃüÁîÂë×ª»»ÃüÁî×Ö·û´®
    void Conv_FromAppCmd_code_to_String(UINT cmd_code, std::string& str);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MCBTHREAD_H__1AFBE446_D98D_4D04_98BB_8C30B58CEBB2__INCLUDED_)
