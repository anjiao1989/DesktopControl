
// DeskCtrlInstaller.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CDeskCtrlInstallerApp: 
// �йش����ʵ�֣������ DeskCtrlInstaller.cpp
//

class CDeskCtrlInstallerApp : public CWinApp
{
public:
	CDeskCtrlInstallerApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CDeskCtrlInstallerApp theApp;