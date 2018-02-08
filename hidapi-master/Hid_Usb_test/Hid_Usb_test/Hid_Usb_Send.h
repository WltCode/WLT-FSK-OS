// Hid_Usb_Send.h : Hid_Usb_Send DLL 的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"		// 主符号


// CHid_Usb_SendApp
// 有关此类实现的信息，请参阅 Hid_Usb_Send.cpp
//

extern "C" _declspec(dllexport) int ReadCard(byte*PEmpNo);
extern "C" _declspec(dllexport) int Readblock(int PEmpBlockNo,byte*PEmpSectorPassword,byte*PEmpBlockdata);
extern "C" _declspec(dllexport) int bell(byte pEmptimes);
extern "C" _declspec(dllexport) int Writeblock(int PEmpBlockNo,byte*PEmpSectorPassword,byte*PEmpBlockdata);


extern "C" _declspec(dllexport) int BPWD_Readblock(int PEmpBlockNo,byte*PEmpSectorPassword,byte*PEmpBlockdata);
extern "C" _declspec(dllexport) int BPWD_Writeblock(int PEmpBlockNo,byte*PEmpSectorPassword,byte*PEmpBlockdata);



extern "C" _declspec(dllexport) int FindDevCnt(int*GetDevCnt);

extern "C" _declspec(dllexport) int ReadCard_No(byte*PEmpNo,int com_no);
extern "C" _declspec(dllexport) int Readblock_No(int PEmpBlockNo,byte*PEmpSectorPassword,byte*PEmpBlockdata,int com_no);
extern "C" _declspec(dllexport) int bell_No(byte pEmptimes,int com_no);
extern "C" _declspec(dllexport) int Writeblock_No(int PEmpBlockNo,byte*PEmpSectorPassword,byte*PEmpBlockdata,int com_no);

extern "C" _declspec(dllexport) int BPWD_Readblock_No(int PEmpBlockNo,byte*PEmpSectorPassword,byte*PEmpBlockdata,int com_no);
extern "C" _declspec(dllexport) int BPWD_Writeblock_No(int PEmpBlockNo,byte*PEmpSectorPassword,byte*PEmpBlockdata,int com_no);



extern "C" _declspec(dllexport) int ControlBuzzer(byte pEmptimes);
extern "C" _declspec(dllexport) BOOL OpenComport(DWORD port, unsigned long baudrate);
extern "C" _declspec(dllexport) void CloseComport();
extern "C" _declspec(dllexport) BOOL ReadVersion(BYTE* ver);
extern "C" _declspec(dllexport) int ReadSerialNo(BYTE* sn);
extern "C" _declspec(dllexport) int ReadBlockData(unsigned char *sn,    //	硬卡號
unsigned char blkNo,           //	要读取的块号
unsigned char keyType,         //	密碼类型(KeyA,KeyB)
unsigned char *keyValue,       //	密碼内容
unsigned char *pdata); 
extern "C" _declspec(dllexport) int WriteBlockData(unsigned char *sn,             //	硬卡号
unsigned char blkNo,           //	要读取的块号
unsigned char keyType,         //	密碼类型(KeyA,KeyB)
unsigned char *keyValue,       //	密碼内容
unsigned char *pdata); 
extern "C" _declspec(dllexport) int ReadSectorData(unsigned char *sn,    //	硬卡号
unsigned char secNo,           //	要读取的扇区号
unsigned char keyType,         //	密碼类型(KeyA,KeyB)
unsigned char *keyValue,       //	密碼内容
unsigned char *pdata); 
extern "C" _declspec(dllexport) int WriteSectorData(unsigned char *sn,             //	硬卡号
unsigned char secNo,           //	要读取的扇区号
unsigned char keyType,         //	密碼类型(KeyA,KeyB)
unsigned char *keyValue,       //	密碼内容
unsigned char *pdata); 


extern "C" _declspec(dllexport) int ReadFlashData(
unsigned long address,           //	要读取的块号
unsigned short datasize,         //	密碼类型(KeyA,KeyB)
unsigned char *pdata);			//
extern "C" _declspec(dllexport) int WriteFlashData(
unsigned long address,           //	要读取的块号
unsigned short datasize,         //	密碼类型(KeyA,KeyB)
unsigned char *pdata);		   //


class CHid_Usb_SendApp : public CWinApp
{
public:
	CHid_Usb_SendApp();

// 重写
public:
	virtual BOOL InitInstance();





	DECLARE_MESSAGE_MAP()







};



