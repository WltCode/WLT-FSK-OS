
// Hid_Usb_testDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Hid_Usb_test.h"
#include "Hid_Usb_testDlg.h"
#include "afxdialogex.h"
//#include "Hid_Usb_Send.h"
#include "hidapi.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CHid_Usb_testDlg 对话框




CHid_Usb_testDlg::CHid_Usb_testDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CHid_Usb_testDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CHid_Usb_testDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, card_no);
	DDX_Control(pDX, IDC_EDIT3, block_no1);
	DDX_Control(pDX, IDC_EDIT6, block_no2);
	DDX_Control(pDX, IDC_EDIT4, pwd1);
	DDX_Control(pDX, IDC_EDIT7, pwd2);
	DDX_Control(pDX, IDC_EDIT5, data_in);
	DDX_Control(pDX, IDC_EDIT8, data_out);
	DDX_Control(pDX, IDC_EDIT10, pwd3);
	DDX_Control(pDX, IDC_EDIT13, pwd4);
	DDX_Control(pDX, IDC_EDIT11, data_in_1);
	DDX_Control(pDX, IDC_EDIT14, data_out_1);
	DDX_Control(pDX, IDC_EDIT9, block_no3);
	DDX_Control(pDX, IDC_EDIT12, block_no4);
	DDX_Control(pDX, IDC_COMBO1, Com_No_box);

	DDX_Control(pDX, IDC_EDIT15, data_in_2);
	DDX_Control(pDX, IDC_EDIT16, data_in_3);
	DDX_Control(pDX, IDC_EDIT17, data_in_4);
	DDX_Control(pDX, IDC_EDIT18, data_out_2);
	DDX_Control(pDX, IDC_EDIT19, data_out_3);
	DDX_Control(pDX, IDC_EDIT20, data_out_4);


	DDX_Control(pDX, IDC_EDIT23, datasize5);
	DDX_Control(pDX, IDC_EDIT24, address5);
	DDX_Control(pDX, IDC_EDIT25, data_in_5);

	DDX_Control(pDX, IDC_EDIT26, datasize6);
	DDX_Control(pDX, IDC_EDIT27, address6);
	DDX_Control(pDX, IDC_EDIT28, data_out_5);










}

BEGIN_MESSAGE_MAP(CHid_Usb_testDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CHid_Usb_testDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON1, &CHid_Usb_testDlg::OnBnClickedButton1)
	ON_EN_CHANGE(IDC_EDIT1, &CHid_Usb_testDlg::OnEnChangeEdit1)
	ON_BN_CLICKED(IDC_BUTTON2, &CHid_Usb_testDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CHid_Usb_testDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON5, &CHid_Usb_testDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON6, &CHid_Usb_testDlg::OnBnClickedButton6)
	ON_BN_CLICKED(IDC_BUTTON7, &CHid_Usb_testDlg::OnBnClickedButton7)
	ON_CBN_DROPDOWN(IDC_COMBO1, &CHid_Usb_testDlg::OnDropdownCombo1)
	ON_BN_CLICKED(IDC_BUTTON9, &CHid_Usb_testDlg::OnBnClickedButton9)
	ON_BN_CLICKED(IDC_BUTTON10, &CHid_Usb_testDlg::OnBnClickedButton10)
END_MESSAGE_MAP()


// CHid_Usb_testDlg 消息处理程序

BOOL CHid_Usb_testDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();






	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码




	open_com_flag = 0;
	SetDlgItemText(IDC_BUTTON7, "打开设备");



	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CHid_Usb_testDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CHid_Usb_testDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CHid_Usb_testDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CHid_Usb_testDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
}







void CHid_Usb_testDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	byte wBuffer[4];
	


	if(open_com_flag == 1)
	{
		
		//ReadCard_No(&wBuffer[0],Com_No_box.GetCurSel()+1);

		int Result;
		Result = ReadSerialNo(&wBuffer[0]);
		if(Result==0xaa)
		{
			MessageBox("打开发卡器失败");
		}
		else if(Result==0x55)
		{
			MessageBox("请放入卡片");
		}



		sn_no[0] = wBuffer[0];
		sn_no[1] = wBuffer[1];
		sn_no[2] = wBuffer[2];
		sn_no[3] = wBuffer[3];

		byte card_idnum;
		card_idnum = wBuffer[3];
		wBuffer[3] = wBuffer[0];
		wBuffer[0] = card_idnum;

		card_idnum = wBuffer[2];
		wBuffer[2] = wBuffer[1];
		wBuffer[1] = card_idnum;





		CString m_Ecard_num,card_num;
		char dest[9];
		memset(&dest[0],0,9);
		ByteToAscii(&dest[0],wBuffer[0]);
		ByteToAscii(&dest[2],wBuffer[1]);
		ByteToAscii(&dest[4],wBuffer[2]);
		ByteToAscii(&dest[6],wBuffer[3]);
		//dest[9] = '\0';
		card_num.Format(_T("%s"),dest);

		card_no.SetWindowText(card_num);

	}
	else
	{
		MessageBox("请选择设置号，并打开设备");
	}
	
}


void CHid_Usb_testDlg::OnEnChangeEdit1()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}







void CHid_Usb_testDlg::WStringToString(char *dest, const CString &src)
{
	register int n = src.GetLength();

	for(register int i=0; i<n; i++)
		dest[i] = (unsigned char)src.GetAt(i);
	dest[n] = '\0';
}

void CHid_Usb_testDlg::ByteToAscii(char *dest, char c)
{
	register unsigned char a = c & 0xf0;
	dest[0] = HexToChar(a >> 4);
	a = c & 0xf;
	dest[1] = HexToChar(a);
}

bool CHid_Usb_testDlg::ParseWString(unsigned char *dest, const CString &s)
{
	register int n = s.GetLength();

	register int time = 0;

	unsigned char tmp[2];

	int j = 0;

	char t;

	for(int i=0; i<n; i++)
	{
		t = (char)s.GetAt(i);

		switch(t)
		{
		case ' ':
			if(time == 1)
				goto BAD_FORMAT;
			break;

		case ',':
			if(time == 2)
			{
				time = 0;
				dest[j++] = tmp[0] * 16 + tmp[1];
				break;
			}
			else
				goto BAD_FORMAT;

		default:
			
			tmp[time] = CharToHex(t);
			if(tmp[time] == 0xff)
				goto BAD_FORMAT;
			time++;
			break;
			BAD_FORMAT:
			MessageBox("ASCII串格式错误，请参见提示格式！");
			return false;
		}
	}

	if(time != 2)
		goto BAD_FORMAT;

	dest[j++] = tmp[0] * 16 + tmp[1];

	return true;
}



void CHid_Usb_testDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	if(open_com_flag == 1)
	{


		byte wBuffer[16];
		byte PEmpBlockNo;
		byte PEmpSectorPassword[6];
		//data_in.SetWindowText(data_in_E);
		int sum_val,ata_length,b;




		block_no1.GetWindowText(block_no1_E);
		ata_length = block_no1_E.GetLength();
		sum_val = 0;
		for(b=0;b<ata_length;b++)
		{
			sum_val = sum_val*10+CharToHex(block_no1_E.GetAt(b));
		}
		PEmpBlockNo = sum_val;




		pwd1.GetWindowText(pwd1_E);

	
		PEmpSectorPassword[0]=CharToHex(pwd1_E.GetAt(0))*16+CharToHex(pwd1_E.GetAt(1));
		PEmpSectorPassword[1]=CharToHex(pwd1_E.GetAt(2))*16+CharToHex(pwd1_E.GetAt(3));
		PEmpSectorPassword[2]=CharToHex(pwd1_E.GetAt(4))*16+CharToHex(pwd1_E.GetAt(5));
		PEmpSectorPassword[3]=CharToHex(pwd1_E.GetAt(6))*16+CharToHex(pwd1_E.GetAt(7));
		PEmpSectorPassword[4]=CharToHex(pwd1_E.GetAt(8))*16+CharToHex(pwd1_E.GetAt(9));
		PEmpSectorPassword[5]=CharToHex(pwd1_E.GetAt(10))*16+CharToHex(pwd1_E.GetAt(11));
	

		int Result;
		Result = ReadBlockData(&sn_no[0],PEmpBlockNo,0x60,&PEmpSectorPassword[0],&wBuffer[0]);
		if(Result==0xaa)
		{
			MessageBox("打开发卡器失败");
		}
		else if(Result==0x55)
		{
			MessageBox("请放入卡片");
		}

		//Readblock_No(PEmpBlockNo,&PEmpSectorPassword[0],&wBuffer[0],Com_No_box.GetCurSel()+1);


		CString m_Ecard_num,card_num;
		char dest[33];
		memset(&dest[0],0,33);
		ByteToAscii(&dest[0],wBuffer[0]);
		ByteToAscii(&dest[2],wBuffer[1]);
		ByteToAscii(&dest[4],wBuffer[2]);
		ByteToAscii(&dest[6],wBuffer[3]);

		ByteToAscii(&dest[8],wBuffer[4]);
		ByteToAscii(&dest[10],wBuffer[5]);
		ByteToAscii(&dest[12],wBuffer[6]);
		ByteToAscii(&dest[14],wBuffer[7]);

		ByteToAscii(&dest[16],wBuffer[8]);
		ByteToAscii(&dest[18],wBuffer[9]);
		ByteToAscii(&dest[20],wBuffer[10]);
		ByteToAscii(&dest[22],wBuffer[11]);


		ByteToAscii(&dest[24],wBuffer[12]);
		ByteToAscii(&dest[26],wBuffer[13]);
		ByteToAscii(&dest[28],wBuffer[14]);
		ByteToAscii(&dest[30],wBuffer[15]);
		//dest[9] = '\0';
		card_num.Format(_T("%s"),dest);

		data_in.SetWindowText(card_num);

	}
	else
	{
		MessageBox("请选择设置号，并打开设备");
	}




}


void CHid_Usb_testDlg::OnBnClickedButton3()
{
	// TODO: 在此添加控件通知处理程序代码

	if(open_com_flag == 1)
	{


		byte wBuffer[16];
		byte PEmpBlockNo;
		byte PEmpSectorPassword[6];

		int sum_val,ata_length,b;



		block_no1.GetWindowText(block_no1_E);
		ata_length = block_no1_E.GetLength();
		sum_val = 0;
		for(b=0;b<ata_length;b++)
		{
			sum_val = sum_val*10+CharToHex(block_no1_E.GetAt(b));
		}
		PEmpBlockNo = sum_val;


		pwd2.GetWindowText(pwd2_E);

	
		PEmpSectorPassword[0]=CharToHex(pwd2_E.GetAt(0))*16+CharToHex(pwd2_E.GetAt(1));
		PEmpSectorPassword[1]=CharToHex(pwd2_E.GetAt(2))*16+CharToHex(pwd2_E.GetAt(3));
		PEmpSectorPassword[2]=CharToHex(pwd2_E.GetAt(4))*16+CharToHex(pwd2_E.GetAt(5));
		PEmpSectorPassword[3]=CharToHex(pwd2_E.GetAt(6))*16+CharToHex(pwd2_E.GetAt(7));
		PEmpSectorPassword[4]=CharToHex(pwd2_E.GetAt(8))*16+CharToHex(pwd2_E.GetAt(9));
		PEmpSectorPassword[5]=CharToHex(pwd2_E.GetAt(10))*16+CharToHex(pwd2_E.GetAt(11));


		data_out.GetWindowText(data_out_E);

		wBuffer[0] = CharToHex(data_out_E.GetAt(0))*16+CharToHex(data_out_E.GetAt(1));
		wBuffer[1] = CharToHex(data_out_E.GetAt(2))*16+CharToHex(data_out_E.GetAt(3));
		wBuffer[2] = CharToHex(data_out_E.GetAt(4))*16+CharToHex(data_out_E.GetAt(5));
		wBuffer[3] = CharToHex(data_out_E.GetAt(6))*16+CharToHex(data_out_E.GetAt(7));
		wBuffer[4] = CharToHex(data_out_E.GetAt(8))*16+CharToHex(data_out_E.GetAt(9));
		wBuffer[5] = CharToHex(data_out_E.GetAt(10))*16+CharToHex(data_out_E.GetAt(11));
		wBuffer[6] = CharToHex(data_out_E.GetAt(12))*16+CharToHex(data_out_E.GetAt(13));
		wBuffer[7] = CharToHex(data_out_E.GetAt(14))*16+CharToHex(data_out_E.GetAt(15));
		wBuffer[8] = CharToHex(data_out_E.GetAt(16))*16+CharToHex(data_out_E.GetAt(17));
		wBuffer[9] = CharToHex(data_out_E.GetAt(18))*16+CharToHex(data_out_E.GetAt(19));
		wBuffer[10] = CharToHex(data_out_E.GetAt(20))*16+CharToHex(data_out_E.GetAt(21));
		wBuffer[11] = CharToHex(data_out_E.GetAt(22))*16+CharToHex(data_out_E.GetAt(23));
		wBuffer[12] = CharToHex(data_out_E.GetAt(24))*16+CharToHex(data_out_E.GetAt(25));
		wBuffer[13] = CharToHex(data_out_E.GetAt(26))*16+CharToHex(data_out_E.GetAt(27));
		wBuffer[14] = CharToHex(data_out_E.GetAt(28))*16+CharToHex(data_out_E.GetAt(29));
		wBuffer[15] = CharToHex(data_out_E.GetAt(30))*16+CharToHex(data_out_E.GetAt(31));


		//Writeblock_No(PEmpBlockNo,&PEmpSectorPassword[0],&wBuffer[0],Com_No_box.GetCurSel()+1);
		

		int Result;
		Result = WriteBlockData(&sn_no[0],PEmpBlockNo,0x60,&PEmpSectorPassword[0],&wBuffer[0]);
		if(Result==0xaa)
		{
			MessageBox("打开发卡器失败");
		}
		else if(Result==0x55)
		{
			MessageBox("请放入卡片");
		}




	}
	else
	{
		MessageBox("请选择设置号，并打开设备");
	}







}


void CHid_Usb_testDlg::OnBnClickedButton5()
{
	// TODO: 在此添加控件通知处理程序代码

	if(open_com_flag == 1)
	{



		byte wBuffer[16*4];
		byte PEmpBlockNo;
		byte PEmpSectorPassword[6];

		//data_in.SetWindowText(data_in_E);


		int sum_val,ata_length,b;
		block_no3.GetWindowText(block_no3_E);
		ata_length = block_no3_E.GetLength();
		sum_val = 0;
		for(b=0;b<ata_length;b++)
		{
			sum_val = sum_val*10+CharToHex(block_no3_E.GetAt(b));
		}
		PEmpBlockNo = sum_val;




		pwd3.GetWindowText(pwd3_E);

	
		PEmpSectorPassword[0]=CharToHex(pwd3_E.GetAt(0))*16+CharToHex(pwd3_E.GetAt(1));
		PEmpSectorPassword[1]=CharToHex(pwd3_E.GetAt(2))*16+CharToHex(pwd3_E.GetAt(3));
		PEmpSectorPassword[2]=CharToHex(pwd3_E.GetAt(4))*16+CharToHex(pwd3_E.GetAt(5));
		PEmpSectorPassword[3]=CharToHex(pwd3_E.GetAt(6))*16+CharToHex(pwd3_E.GetAt(7));
		PEmpSectorPassword[4]=CharToHex(pwd3_E.GetAt(8))*16+CharToHex(pwd3_E.GetAt(9));
		PEmpSectorPassword[5]=CharToHex(pwd3_E.GetAt(10))*16+CharToHex(pwd3_E.GetAt(11));
	

		
		//BPWD_Readblock_No(PEmpBlockNo,&PEmpSectorPassword[0],&wBuffer[0],Com_No_box.GetCurSel()+1);

		int Result;
		Result = ReadSectorData(&sn_no[0],PEmpBlockNo,0x60,&PEmpSectorPassword[0],&wBuffer[0]);
		if(Result==0xaa)
		{
			MessageBox("打开发卡器失败");
		}
		else if(Result==0x55)
		{
			MessageBox("请放入卡片");
		}


		CString m_Ecard_num,card_num;
		char dest[33];
		memset(&dest[0],0,33);
		ByteToAscii(&dest[0],wBuffer[0]);
		ByteToAscii(&dest[2],wBuffer[1]);
		ByteToAscii(&dest[4],wBuffer[2]);
		ByteToAscii(&dest[6],wBuffer[3]);

		ByteToAscii(&dest[8],wBuffer[4]);
		ByteToAscii(&dest[10],wBuffer[5]);
		ByteToAscii(&dest[12],wBuffer[6]);
		ByteToAscii(&dest[14],wBuffer[7]);

		ByteToAscii(&dest[16],wBuffer[8]);
		ByteToAscii(&dest[18],wBuffer[9]);
		ByteToAscii(&dest[20],wBuffer[10]);
		ByteToAscii(&dest[22],wBuffer[11]);


		ByteToAscii(&dest[24],wBuffer[12]);
		ByteToAscii(&dest[26],wBuffer[13]);
		ByteToAscii(&dest[28],wBuffer[14]);
		ByteToAscii(&dest[30],wBuffer[15]);
		//dest[9] = '\0';
		card_num.Format(_T("%s"),dest);

		data_in_1.SetWindowText(card_num);


		memset(&dest[0],0,33);
		ByteToAscii(&dest[0],wBuffer[0+16*1]);
		ByteToAscii(&dest[2],wBuffer[1+16*1]);
		ByteToAscii(&dest[4],wBuffer[2+16*1]);
		ByteToAscii(&dest[6],wBuffer[3+16*1]);

		ByteToAscii(&dest[8],wBuffer[4+16*1]);
		ByteToAscii(&dest[10],wBuffer[5+16*1]);
		ByteToAscii(&dest[12],wBuffer[6+16*1]);
		ByteToAscii(&dest[14],wBuffer[7+16*1]);

		ByteToAscii(&dest[16],wBuffer[8+16*1]);
		ByteToAscii(&dest[18],wBuffer[9+16*1]);
		ByteToAscii(&dest[20],wBuffer[10+16*1]);
		ByteToAscii(&dest[22],wBuffer[11+16*1]);


		ByteToAscii(&dest[24],wBuffer[12+16*1]);
		ByteToAscii(&dest[26],wBuffer[13+16*1]);
		ByteToAscii(&dest[28],wBuffer[14+16*1]);
		ByteToAscii(&dest[30],wBuffer[15+16*1]);
		//dest[9] = '\0';
		card_num.Format(_T("%s"),dest);

		data_in_2.SetWindowText(card_num);


		memset(&dest[0],0,33);
		ByteToAscii(&dest[0],wBuffer[0+16*2]);
		ByteToAscii(&dest[2],wBuffer[1+16*2]);
		ByteToAscii(&dest[4],wBuffer[2+16*2]);
		ByteToAscii(&dest[6],wBuffer[3+16*2]);

		ByteToAscii(&dest[8],wBuffer[4+16*2]);
		ByteToAscii(&dest[10],wBuffer[5+16*2]);
		ByteToAscii(&dest[12],wBuffer[6+16*2]);
		ByteToAscii(&dest[14],wBuffer[7+16*2]);

		ByteToAscii(&dest[16],wBuffer[8+16*2]);
		ByteToAscii(&dest[18],wBuffer[9+16*2]);
		ByteToAscii(&dest[20],wBuffer[10+16*2]);
		ByteToAscii(&dest[22],wBuffer[11+16*2]);


		ByteToAscii(&dest[24],wBuffer[12+16*2]);
		ByteToAscii(&dest[26],wBuffer[13+16*2]);
		ByteToAscii(&dest[28],wBuffer[14+16*2]);
		ByteToAscii(&dest[30],wBuffer[15+16*2]);
		//dest[9] = '\0';
		card_num.Format(_T("%s"),dest);

		data_in_3.SetWindowText(card_num);




		memset(&dest[0],0,33);
		ByteToAscii(&dest[0],wBuffer[0+16*3]);
		ByteToAscii(&dest[2],wBuffer[1+16*3]);
		ByteToAscii(&dest[4],wBuffer[2+16*3]);
		ByteToAscii(&dest[6],wBuffer[3+16*3]);

		ByteToAscii(&dest[8],wBuffer[4+16*3]);
		ByteToAscii(&dest[10],wBuffer[5+16*3]);
		ByteToAscii(&dest[12],wBuffer[6+16*3]);
		ByteToAscii(&dest[14],wBuffer[7+16*3]);

		ByteToAscii(&dest[16],wBuffer[8+16*3]);
		ByteToAscii(&dest[18],wBuffer[9+16*3]);
		ByteToAscii(&dest[20],wBuffer[10+16*3]);
		ByteToAscii(&dest[22],wBuffer[11+16*3]);


		ByteToAscii(&dest[24],wBuffer[12+16*3]);
		ByteToAscii(&dest[26],wBuffer[13+16*3]);
		ByteToAscii(&dest[28],wBuffer[14+16*3]);
		ByteToAscii(&dest[30],wBuffer[15+16*3]);
		//dest[9] = '\0';
		card_num.Format(_T("%s"),dest);

		data_in_4.SetWindowText(card_num);

	}
	else
	{
		MessageBox("请选择设置号，并打开设备");
	}





}


void CHid_Usb_testDlg::OnBnClickedButton6()
{
	// TODO: 在此添加控件通知处理程序代码

			// TODO: 在此添加控件通知处理程序代码
	byte wBuffer[16*4];
	byte PEmpBlockNo;
	byte PEmpSectorPassword[6];

	int sum_val,ata_length,b;
	block_no4.GetWindowText(block_no4_E);
	ata_length = block_no4_E.GetLength();
	sum_val = 0;
	for(b=0;b<ata_length;b++)
	{
		sum_val = sum_val*10+CharToHex(block_no4_E.GetAt(b));
	}
	PEmpBlockNo = sum_val;


	pwd4.GetWindowText(pwd4_E);

	
	PEmpSectorPassword[0]=CharToHex(pwd4_E.GetAt(0))*16+CharToHex(pwd4_E.GetAt(1));
	PEmpSectorPassword[1]=CharToHex(pwd4_E.GetAt(2))*16+CharToHex(pwd4_E.GetAt(3));
	PEmpSectorPassword[2]=CharToHex(pwd4_E.GetAt(4))*16+CharToHex(pwd4_E.GetAt(5));
	PEmpSectorPassword[3]=CharToHex(pwd4_E.GetAt(6))*16+CharToHex(pwd4_E.GetAt(7));
	PEmpSectorPassword[4]=CharToHex(pwd4_E.GetAt(8))*16+CharToHex(pwd4_E.GetAt(9));
	PEmpSectorPassword[5]=CharToHex(pwd4_E.GetAt(10))*16+CharToHex(pwd4_E.GetAt(11));


	data_out_1.GetWindowText(data_out_1_E);

	wBuffer[0] = CharToHex(data_out_1_E.GetAt(0))*16+CharToHex(data_out_1_E.GetAt(1));
	wBuffer[1] = CharToHex(data_out_1_E.GetAt(2))*16+CharToHex(data_out_1_E.GetAt(3));
	wBuffer[2] = CharToHex(data_out_1_E.GetAt(4))*16+CharToHex(data_out_1_E.GetAt(5));
	wBuffer[3] = CharToHex(data_out_1_E.GetAt(6))*16+CharToHex(data_out_1_E.GetAt(7));
	wBuffer[4] = CharToHex(data_out_1_E.GetAt(8))*16+CharToHex(data_out_1_E.GetAt(9));
	wBuffer[5] = CharToHex(data_out_1_E.GetAt(10))*16+CharToHex(data_out_1_E.GetAt(11));
	wBuffer[6] = CharToHex(data_out_1_E.GetAt(12))*16+CharToHex(data_out_1_E.GetAt(13));
	wBuffer[7] = CharToHex(data_out_1_E.GetAt(14))*16+CharToHex(data_out_1_E.GetAt(15));
	wBuffer[8] = CharToHex(data_out_1_E.GetAt(16))*16+CharToHex(data_out_1_E.GetAt(17));
	wBuffer[9] = CharToHex(data_out_1_E.GetAt(18))*16+CharToHex(data_out_1_E.GetAt(19));
	wBuffer[10] = CharToHex(data_out_1_E.GetAt(20))*16+CharToHex(data_out_1_E.GetAt(21));
	wBuffer[11] = CharToHex(data_out_1_E.GetAt(22))*16+CharToHex(data_out_1_E.GetAt(23));
	wBuffer[12] = CharToHex(data_out_1_E.GetAt(24))*16+CharToHex(data_out_1_E.GetAt(25));
	wBuffer[13] = CharToHex(data_out_1_E.GetAt(26))*16+CharToHex(data_out_1_E.GetAt(27));
	wBuffer[14] = CharToHex(data_out_1_E.GetAt(28))*16+CharToHex(data_out_1_E.GetAt(29));
	wBuffer[15] = CharToHex(data_out_1_E.GetAt(30))*16+CharToHex(data_out_1_E.GetAt(31));


	data_out_2.GetWindowText(data_out_2_E);

	wBuffer[0+1*16] = CharToHex(data_out_2_E.GetAt(0))*16+CharToHex(data_out_2_E.GetAt(1));
	wBuffer[1+1*16] = CharToHex(data_out_2_E.GetAt(2))*16+CharToHex(data_out_2_E.GetAt(3));
	wBuffer[2+1*16] = CharToHex(data_out_2_E.GetAt(4))*16+CharToHex(data_out_2_E.GetAt(5));
	wBuffer[3+1*16] = CharToHex(data_out_2_E.GetAt(6))*16+CharToHex(data_out_2_E.GetAt(7));
	wBuffer[4+1*16] = CharToHex(data_out_2_E.GetAt(8))*16+CharToHex(data_out_2_E.GetAt(9));
	wBuffer[5+1*16] = CharToHex(data_out_2_E.GetAt(10))*16+CharToHex(data_out_2_E.GetAt(11));
	wBuffer[6+1*16] = CharToHex(data_out_2_E.GetAt(12))*16+CharToHex(data_out_2_E.GetAt(13));
	wBuffer[7+1*16] = CharToHex(data_out_2_E.GetAt(14))*16+CharToHex(data_out_2_E.GetAt(15));
	wBuffer[8+1*16] = CharToHex(data_out_2_E.GetAt(16))*16+CharToHex(data_out_2_E.GetAt(17));
	wBuffer[9+1*16] = CharToHex(data_out_2_E.GetAt(18))*16+CharToHex(data_out_2_E.GetAt(19));
	wBuffer[10+1*16] = CharToHex(data_out_2_E.GetAt(20))*16+CharToHex(data_out_2_E.GetAt(21));
	wBuffer[11+1*16] = CharToHex(data_out_2_E.GetAt(22))*16+CharToHex(data_out_2_E.GetAt(23));
	wBuffer[12+1*16] = CharToHex(data_out_2_E.GetAt(24))*16+CharToHex(data_out_2_E.GetAt(25));
	wBuffer[13+1*16] = CharToHex(data_out_2_E.GetAt(26))*16+CharToHex(data_out_2_E.GetAt(27));
	wBuffer[14+1*16] = CharToHex(data_out_2_E.GetAt(28))*16+CharToHex(data_out_2_E.GetAt(29));
	wBuffer[15+1*16] = CharToHex(data_out_2_E.GetAt(30))*16+CharToHex(data_out_2_E.GetAt(31));


	data_out_3.GetWindowText(data_out_3_E);

	wBuffer[0+2*16] = CharToHex(data_out_3_E.GetAt(0))*16+CharToHex(data_out_3_E.GetAt(1));
	wBuffer[1+2*16] = CharToHex(data_out_3_E.GetAt(2))*16+CharToHex(data_out_3_E.GetAt(3));
	wBuffer[2+2*16] = CharToHex(data_out_3_E.GetAt(4))*16+CharToHex(data_out_3_E.GetAt(5));
	wBuffer[3+2*16] = CharToHex(data_out_3_E.GetAt(6))*16+CharToHex(data_out_3_E.GetAt(7));
	wBuffer[4+2*16] = CharToHex(data_out_3_E.GetAt(8))*16+CharToHex(data_out_3_E.GetAt(9));
	wBuffer[5+2*16] = CharToHex(data_out_3_E.GetAt(10))*16+CharToHex(data_out_3_E.GetAt(11));
	wBuffer[6+2*16] = CharToHex(data_out_3_E.GetAt(12))*16+CharToHex(data_out_3_E.GetAt(13));
	wBuffer[7+2*16] = CharToHex(data_out_3_E.GetAt(14))*16+CharToHex(data_out_3_E.GetAt(15));
	wBuffer[8+2*16] = CharToHex(data_out_3_E.GetAt(16))*16+CharToHex(data_out_3_E.GetAt(17));
	wBuffer[9+2*16] = CharToHex(data_out_3_E.GetAt(18))*16+CharToHex(data_out_3_E.GetAt(19));
	wBuffer[10+2*16] = CharToHex(data_out_3_E.GetAt(20))*16+CharToHex(data_out_3_E.GetAt(21));
	wBuffer[11+2*16] = CharToHex(data_out_3_E.GetAt(22))*16+CharToHex(data_out_3_E.GetAt(23));
	wBuffer[12+2*16] = CharToHex(data_out_3_E.GetAt(24))*16+CharToHex(data_out_3_E.GetAt(25));
	wBuffer[13+2*16] = CharToHex(data_out_3_E.GetAt(26))*16+CharToHex(data_out_3_E.GetAt(27));
	wBuffer[14+2*16] = CharToHex(data_out_3_E.GetAt(28))*16+CharToHex(data_out_3_E.GetAt(29));
	wBuffer[15+2*16] = CharToHex(data_out_3_E.GetAt(30))*16+CharToHex(data_out_3_E.GetAt(31));


	data_out_4.GetWindowText(data_out_4_E);

	wBuffer[0+3*16] = CharToHex(data_out_4_E.GetAt(0))*16+CharToHex(data_out_4_E.GetAt(1));
	wBuffer[1+3*16] = CharToHex(data_out_4_E.GetAt(2))*16+CharToHex(data_out_4_E.GetAt(3));
	wBuffer[2+3*16] = CharToHex(data_out_4_E.GetAt(4))*16+CharToHex(data_out_4_E.GetAt(5));
	wBuffer[3+3*16] = CharToHex(data_out_4_E.GetAt(6))*16+CharToHex(data_out_4_E.GetAt(7));
	wBuffer[4+3*16] = CharToHex(data_out_4_E.GetAt(8))*16+CharToHex(data_out_4_E.GetAt(9));
	wBuffer[5+3*16] = CharToHex(data_out_4_E.GetAt(10))*16+CharToHex(data_out_4_E.GetAt(11));
	wBuffer[6+3*16] = CharToHex(data_out_4_E.GetAt(12))*16+CharToHex(data_out_4_E.GetAt(13));
	wBuffer[7+3*16] = CharToHex(data_out_4_E.GetAt(14))*16+CharToHex(data_out_4_E.GetAt(15));
	wBuffer[8+3*16] = CharToHex(data_out_4_E.GetAt(16))*16+CharToHex(data_out_4_E.GetAt(17));
	wBuffer[9+3*16] = CharToHex(data_out_4_E.GetAt(18))*16+CharToHex(data_out_4_E.GetAt(19));
	wBuffer[10+3*16] = CharToHex(data_out_4_E.GetAt(20))*16+CharToHex(data_out_4_E.GetAt(21));
	wBuffer[11+3*16] = CharToHex(data_out_4_E.GetAt(22))*16+CharToHex(data_out_4_E.GetAt(23));
	wBuffer[12+3*16] = CharToHex(data_out_4_E.GetAt(24))*16+CharToHex(data_out_4_E.GetAt(25));
	wBuffer[13+3*16] = CharToHex(data_out_4_E.GetAt(26))*16+CharToHex(data_out_4_E.GetAt(27));
	wBuffer[14+3*16] = CharToHex(data_out_4_E.GetAt(28))*16+CharToHex(data_out_4_E.GetAt(29));
	wBuffer[15+3*16] = CharToHex(data_out_4_E.GetAt(30))*16+CharToHex(data_out_4_E.GetAt(31));




	
	//BPWD_Writeblock_No(PEmpBlockNo,&PEmpSectorPassword[0],&wBuffer[0],Com_No_box.GetCurSel()+1);


	int Result;
	Result = WriteSectorData(&sn_no[0],PEmpBlockNo,0x60,&PEmpSectorPassword[0],&wBuffer[0]);
	if(Result==0xaa)
	{
		MessageBox("打开发卡器失败");
	}
	else if(Result==0x55)
	{
		MessageBox("请放入卡片");
	}

	




}

void CHid_Usb_testDlg::OnBnClickedButton7()
{
	if(open_com_flag == 0)
	{
		if(OpenComport(Com_No_box.GetCurSel()+1,0) == FALSE)
		{
			open_com_flag = 0;
			SetDlgItemText(IDC_BUTTON7, "打开设备");
		}
		else
		{
			open_com_flag = 1;
			SetDlgItemText(IDC_BUTTON7, "关闭设备");

		}
	}
	else
	{
		open_com_flag = 0;
		CloseComport();
		SetDlgItemText(IDC_BUTTON7, "打开设备");
	}

}




void CHid_Usb_testDlg::OnDropdownCombo1()
{
	// TODO: 在此添加控件通知处理程序代码
			int Com_No;

	Com_No_box.ResetContent();

	FindDevCnt(&Com_No);
	if(Com_No>0)
	{
		for(int i=0;i<Com_No;i++)
		{	
			CString s;
			s.Format("%d",i+1);
			Com_No_box.AddString("COM"+s);
		}
	}



}


void CHid_Usb_testDlg::OnBnClickedButton9()
{
	// TODO: 在此添加控件通知处理程序代码
	byte wBuffer[16*4];
	byte PEmpBlockNo;
	byte PEmpSectorPassword[3];
	unsigned long address;
	int sum_val,ata_length,b;
	datasize5.GetWindowText(datasize5_E);
	ata_length = datasize5_E.GetLength();
	sum_val = 0;
	for(b=0;b<ata_length;b++)
	{
		sum_val = sum_val*10+CharToHex(datasize5_E.GetAt(b));
	}
	PEmpBlockNo = sum_val;


	address5.GetWindowText(address5_E);
	ata_length = address5_E.GetLength();
	sum_val = 0;
	for(b=0;b<ata_length;b++)
	{
		sum_val = sum_val*10+CharToHex(address5_E.GetAt(b));
	}
	address = sum_val;

	int Result;
	Result = ReadFlashData(address,PEmpBlockNo,&wBuffer[0]);



	CString m_Ecard_num,card_num;

	char dest[49];

	memset(&dest[0],0,49);

	for(b=0;b<PEmpBlockNo;b++)
	{
		ByteToAscii(&dest[2*b],wBuffer[b]);
	}

	//dest[9] = '\0';
	card_num.Format(_T("%s"),dest);

	data_in_5.SetWindowText(card_num);


}


void CHid_Usb_testDlg::OnBnClickedButton10()
{
	// TODO: 在此添加控件通知处理程序代码
	byte wBuffer[16*4];
	byte PEmpBlockNo;
	byte PEmpSectorPassword[3];
	unsigned long address;
	int sum_val,ata_length,b;
	datasize6.GetWindowText(datasize6_E);
	ata_length = datasize6_E.GetLength();
	sum_val = 0;
	for(b=0;b<ata_length;b++)
	{
		sum_val = sum_val*10+CharToHex(datasize6_E.GetAt(b));
	}
	PEmpBlockNo = sum_val;


	address6.GetWindowText(address6_E);


	ata_length = address6_E.GetLength();
	sum_val = 0;
	for(b=0;b<ata_length;b++)
	{
		sum_val = sum_val*10+CharToHex(address6_E.GetAt(b));
	}
	address = sum_val;



	data_out_5.GetWindowText(data_out_5_E);

	for(b=0;b<PEmpBlockNo;b++)
	{
		wBuffer[b] = CharToHex(data_out_5_E.GetAt(2*b))*16+CharToHex(data_out_5_E.GetAt(2*b+1));
	}

	int Result;
	Result = WriteFlashData(address,PEmpBlockNo,&wBuffer[0]);



}
