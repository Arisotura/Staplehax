#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <3ds/types.h>
#include "svc.h"
#include "srv.h"


#define MAX_SESSIONS 16

Handle HandleTable[2+MAX_SESSIONS];
u32 NumHandles = 0;

u32 ncolor = 0;


u32 kerneltest()
{
	*(volatile u32*)0xFFFD6204 = ncolor;
	return 0;
}

int main(int argc, char** argv)
{
	Result res;
	Handle replytarget = 0;
	u32 curhandle = 0;
	
	srvInit();
	
	srvRegisterService(&HandleTable[1], "hb:HB", MAX_SESSIONS);
	srvGetProcSemaphore(&HandleTable[0]);
	NumHandles = 2;
	
	/*srvSubscribe(0x101);
	srvSubscribe(0x202);
	srvSubscribe(0x203);
	srvSubscribe(0x200);
	srvSubscribe(0x103);
	srvSubscribe(0x102);
	srvSubscribe(0x104);
	srvSubscribe(0x105);
	srvSubscribe(0x106);
	srvSubscribe(0x204);
	srvSubscribe(0x205);
	srvSubscribe(0x20A);
	srvSubscribe(0x201);
	srvSubscribe(0x206);
	u32 dorp=0;*/

	for (;;) 
	{ 
		res = svcReplyAndReceive(&curhandle, HandleTable, NumHandles, replytarget);
		if (res == 0xC920181A) // session terminated
		{
			if (curhandle < 2)
				goto exit; // dunno if that's correct
				
			svcCloseHandle(HandleTable[curhandle]);
			NumHandles--;
			if (NumHandles > 2 && NumHandles != curhandle)
				HandleTable[curhandle] = HandleTable[NumHandles];
				
			replytarget = 0;
		}
		else if (res)
			svcBreak();
		else if (curhandle == 0) // semaphore -- notification
		{
			u32 notif;
			res = srvReceiveNotification(&notif);
			if (res || notif == 0x100)
				goto exit;
				
			/*if (notif == 0x200)
			{
				//if (dorp){
				ncolor = 0x01FF00FF;
				for(;;) { svcBackdoor(kerneltest); svcSleepThread(100000000); }//}
				//dorp = 1;
			}*/
				
			replytarget = 0;
		}
		else if (curhandle == 1) // new session incoming
		{
			if (NumHandles >= MAX_SESSIONS+2)
				svcBreak();
				
			res = svcAcceptSession(&HandleTable[NumHandles], HandleTable[1]);
			if (res)
				svcBreak();
				
			replytarget = 0;
			NumHandles++;
		}
		else // command incoming
		{
			u32* cmdbuf = getThreadCommandBuffer();
			switch (cmdbuf[0] >> 16)
			{
				case 1:
					ncolor = cmdbuf[1];
					svcBackdoor(kerneltest);
					cmdbuf[0] = 0x10040;
					cmdbuf[1] = 0;
					break;
					
				default:
					cmdbuf[0] = (cmdbuf[0] & 0xFFFF0000) | 0x0040;
					cmdbuf[1] = 0xFFFFFFFF;
					break;
			}
			
			replytarget = HandleTable[curhandle];
		}
	}
	
exit:
	srvUnregisterProcess("hb:HB");
	srvExit();
	svcExitProcess();
	return 0;
}
