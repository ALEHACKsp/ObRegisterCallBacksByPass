
// RestoreAccessApp.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CRestoreAccessAppApp: 
// �йش����ʵ�֣������ RestoreAccessApp.cpp
//

class CRestoreAccessAppApp : public CWinApp
{
public:
	CRestoreAccessAppApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CRestoreAccessAppApp theApp;