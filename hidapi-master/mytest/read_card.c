/*******************************************************
 Windows HID simplification

 Alan Ott
 Signal 11 Software

 8/22/2009

 Copyright 2009
 
 This contents of this file may be used by anyone
 for any reason without any conditions and may be
 used as a starting point for your own applications
 which use HIDAPI.
********************************************************/

#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include "hidapi.h"



// Headers needed for sleeping.
#ifdef _WIN32
	#include <windows.h>
#else
	#include <unistd.h>
#endif

int main(int argc, char* argv[])
{
	//int res;
	//unsigned char buf[256];
	#define MAX_STR 255
	wchar_t wstr[MAX_STR];
	int i;
	unsigned char CardSN[4]={0};

	struct hid_device_info *devs, *cur_dev;
	int _cmd=0;
	
/*	if (hid_init())
		return -1;

	devs = hid_enumerate(0x0, 0x0);
	cur_dev = devs;	
	while (cur_dev) {
		printf("Device Found\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls", cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
		printf("\n");
		printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
		printf("  Product:      %ls\n", cur_dev->product_string);
		printf("  Release:      %hx\n", cur_dev->release_number);
		printf("  Interface:    %d\n",  cur_dev->interface_number);
		printf("\n");
		cur_dev = cur_dev->next;
	}
	hid_free_enumeration(devs);

	// Set up the command buffer.
	memset(buf,0x00,sizeof(buf));
	buf[0] = 0x01;
	buf[1] = 0x81;
*/
	int count = 0;
	int result_card = 0;
	int open_com_flag = 0;
	FindDevCnt(&count);
	printf("Andrew card test result : %d , count : %d \n", result_card, count);
	if(count != 0) {
		printf("Andrew open comport start \n");
		if(OpenComport(1,0) == false)
                {
			//printf("Andrew fail result : %d", OpenComport(1,0));
                        open_com_flag = 0;
			printf("Andrew open comport fail \n");
                }   
                else
                {   
                        open_com_flag = 1;
			printf("Andrew open comport success \n");
                }   
	}

	unsigned char times = 0x1;
	int bbb;
	int ret = 0;
//	int bbb = ControlBuzzer(times);

	while(exit) {
		printf("\n\nPlease Input the Number CMD below:\n 1->OpenComport  2->Read CardSN 3->Read Blcok Data 4->Read Sector Data 9->close \n Input:");
		scanf("%d",&_cmd);

		//CloseComport();
		//if(OpenComport(1, 0) == 1) {
		printf("wait card touch!!\n");
		ret = ReadSerialNo(&CardSN[0]);
		bbb = ControlBuzzer(times);
		if(ret == 1){
			printf("Read Card(COM1) Success! \n CardNo=%d %d %d %d \n",CardSN[0],CardSN[1],CardSN[2],CardSN[3]);
                }
		printf("Read card ret : %d \n", ret);

		//}

	}

	return 0;
}
