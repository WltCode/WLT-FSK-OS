/********************************************************************
*
*
*
*
*
*
*********************************************************************/
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



#include "hidapi.h"
//#include "libusb/hidapi.h"
#include <stdio.h>

int main() {
	int count = 0;
	int result = 0;
	int open_com_flag = 0;
	int bbb;
	int ret = 0;
	int _cmd=0;
	int i=0;
	unsigned char times = 0x1;
	unsigned char CardSN[4]={0};
	unsigned char CardPW[6]={0xff,0xff,0xff,0xff,0xff,0xff};
	unsigned char CardBLOCK[16]={0};
CloseComport();
	FindDevCnt(&count);
	printf("test result : %d , count : %d\n", result, count);

	if(count != 0) {
		printf("Andrew open comport start \n");
		if(OpenComport(1,0) == 0)
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
	if(open_com_flag == 1)
	{
		while(1) 
		{
			printf("\n\nPlease Input the Number CMD below:\n 1->OpenComport  2->Read CardSN 3->Read Blcok Data 4->Read Sector Data 9->close \n Input:");
			usleep(20000);
			scanf("%d",&_cmd);

			//bbb = ControlBuzzer(times);
			//if(bbb == 1)
			//{
			//	printf("Control Buzzer Success! \n times=%d\n",times);
		        //}
			//printf("Read card ret : %d \n", bbb);


			memset(CardSN,0,4);
			ret = ReadSerialNo(&CardSN[0]);
			if(ret == 1)
			{
				printf("Read SN(COM1) Success! \n CardNo=%d %d %d %d \n",CardSN[0],CardSN[1],CardSN[2],CardSN[3]);
		        }
			printf("Read card ret : %d \n", ret);
			
			memset(CardBLOCK,0,16);
			ret = ReadBlockData(&CardSN[0],0X01,0x60,&CardPW[0],&CardBLOCK[0]);
			if(ret == 1)
			{
				printf("Read Block(COM1) Success! \n");
				for (i = 0; i < 16; i++)
					printf("%02hhx ", CardBLOCK[i]);
				printf("\n");
		        }
			printf("Read card ret : %d \n", ret);

			usleep(20000);
		}

	}

	return 0;
}

