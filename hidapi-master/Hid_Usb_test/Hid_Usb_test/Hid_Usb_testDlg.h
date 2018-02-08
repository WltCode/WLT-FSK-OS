
// Hid_Usb_testDlg.h : 头文件
//

#pragma once
#include "afxwin.h"


// CHid_Usb_testDlg 对话框
class CHid_Usb_testDlg : public CDialogEx
{
// 构造
public:
	CHid_Usb_testDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_HID_USB_TEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnEnChangeEdit1();
	CEdit card_no;
	CEdit block_no1;
	CEdit block_no2;
	CEdit pwd1;
	CEdit pwd2;
	CEdit data_in;
	CEdit data_out;



	
	CString card_no_E;
	CString block_no1_E;
	CString block_no2_E;
	CString pwd1_E;
	CString pwd2_E;
	CString data_in_E;
	CString data_out_E;

	void WStringToString(char *dest, const CString &src);
	void ByteToAscii(char *dest, char c);
	bool ParseWString(unsigned char *dest, const CString &s);
	inline char HexToChar(unsigned char c);
	inline unsigned char CharToHex(char c);

	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	CEdit pwd3;
	CEdit pwd4;

	CString pwd3_E;
	CString pwd4_E;

	CEdit data_in_1;
	CEdit data_out_1;
	CEdit block_no3;
	CEdit block_no4;
	CString block_no3_E;
	CString block_no4_E;
	CString data_in_1_E;
	CString data_out_1_E;


	afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedButton6();

	afx_msg void OnBnClickedButton7();
	CComboBox Com_No_box;



	bool open_com_flag;
	unsigned char sn_no[4];



	afx_msg void OnDropdownCombo1();
	CEdit data_in_2;
	CEdit data_in_3;
	CEdit data_in_4;
	CEdit data_out_2;
	CEdit data_out_3;
	CEdit data_out_4;

	CString data_in_2_E;
	CString data_out_2_E;

	CString data_in_3_E;
	CString data_out_3_E;

	CString data_in_4_E;
	CString data_out_4_E;




	afx_msg void OnBnClickedButton9();
	afx_msg void OnBnClickedButton10();

	CEdit datasize5;
	CEdit datasize6;
	CString datasize5_E;
	CString datasize6_E;


	CEdit address5;
	CEdit address6;
	CString address5_E;
	CString address6_E;

	CEdit data_in_5;
	CEdit data_out_5;
	CString data_in_5_E;
	CString data_out_5_E;


};



inline char CHid_Usb_testDlg::HexToChar(unsigned char c)
{
	c &= 0x0f;
	if(c < 10)
		return (char)(c + '0');
	else
		return (char)(c - 10 + 'A');
}

inline unsigned char CHid_Usb_testDlg::CharToHex(char c)
{
	register unsigned char uc = (unsigned char)c;

	if(uc >= '0' && uc <= '9')
		uc -= '0';

	else if(uc >= 'a' && uc <= 'f')
		uc = (uc - 'a') + 10;
	else if(uc >= 'A' && uc <= 'Z')
		uc = (uc - 'A') + 10;
	else
		uc = 0xff;
	return uc;
}

