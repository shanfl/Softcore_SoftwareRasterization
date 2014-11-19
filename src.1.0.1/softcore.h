// softcore.h : softcore 应用程序的主头文件
//
#pragma once

#ifndef __AFXWIN_H__
	#error 在包含用于 PCH 的此文件之前包含“stdafx.h” 
#endif

#include "resource.h"       // 主符号


// CsoftcoreApp:
// 有关此类的实现，请参阅 softcore.cpp
//

class CsoftcoreApp : public CWinApp
{
public:
	CsoftcoreApp();


// 重写
public:
	virtual BOOL InitInstance();

// 实现

public:
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnIdle(LONG lCount);

	virtual int Run();


public:
	void setIdleState(bool idle){m_canIdle = idle;};
	bool getIdleState()const{return m_canIdle;};
protected:
	//always refresh the screen or can idle
	bool m_canIdle;
};

extern CsoftcoreApp theApp;
