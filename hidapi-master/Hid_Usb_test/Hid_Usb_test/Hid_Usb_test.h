
// Hid_Usb_test.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CHid_Usb_testApp:
// �йش����ʵ�֣������ Hid_Usb_test.cpp
//

class CHid_Usb_testApp : public CWinApp
{
public:
	CHid_Usb_testApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CHid_Usb_testApp theApp;