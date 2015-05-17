#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <3ds/types.h>
#include <3ds/services/fs.h>
#include <3ds/services/apt.h>
#include "svc.h"
#include "shit.h"
#include "text.h"
#include "cn_save_initial_loader_bin.h"
#include "khax.h"
#include "3dsx.h"
#include "constants.h"
#include "kobjects.h"
#include "http.h"

#include "../../build/constants.h"

#define TOPFBADR1 ((u8*)CN_TOPFBADR1)
#define TOPFBADR2 ((u8*)CN_TOPFBADR2)


int _strlen(char* str)
{
	int l=0;
	while(*(str++))l++;
	return l;
}

void _strcpy(char* dst, char* src)
{
	while(*src)*(dst++)=*(src++);
	*dst=0x00;
}

void _strappend(char* str1, char* str2)
{
	_strcpy(&str1[_strlen(str1)], str2);
}

Result _srv_RegisterClient(Handle* handleptr)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x10002; //request header code
	cmdbuf[1]=0x20;

	Result ret=0;
	if((ret=svcSendSyncRequest(*handleptr)))return ret;

	return cmdbuf[1];
}

Result _initSrv(Handle* srvHandle)
{
	Result ret=0;
	if(svcConnectToPort(srvHandle, "srv:"))return ret;
	return _srv_RegisterClient(srvHandle);
}

Result _srv_getServiceHandle(Handle* handleptr, Handle* out, char* server)
{
	u8 l=_strlen(server);
	if(!out || !server || l>8)return -1;

	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0x50100; //request header code
	_strcpy((char*)&cmdbuf[1], server);
	cmdbuf[3]=l;
	cmdbuf[4]=0x0;

	Result ret=0;
	if((ret=svcSendSyncRequest(*handleptr)))return ret;

	*out=cmdbuf[3];

	return cmdbuf[1];
}

const u8 hexTable[]=
{
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

void hex2str(char* out, u32 val)
{
	int i;
	for(i=0;i<8;i++){out[7-i]=hexTable[val&0xf];val>>=4;}
	out[8]=0x00;
}

void renderString(char* str, int x, int y)
{
	Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;
	Result (*_GSPGPU_FlushDataCache)(Handle* handle, Handle kprocess, u8* addr, u32 size)=(Result(*)(Handle*,Handle,u8*,u32))(void*)CN_GSPGPU_FlushDataCache_ADR;
	drawString(TOPFBADR1,str,x,y);
	drawString(TOPFBADR2,str,x,y);
	_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, TOPFBADR1, 240*400*3);
	_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, TOPFBADR2, 240*400*3);
}

void centerString(char* str, int y)
{
	Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;
	Result (*_GSPGPU_FlushDataCache)(Handle* handle, Handle kprocess, u8* addr, u32 size)=(Result(*)(Handle*,Handle,u8*,u32))(void*)CN_GSPGPU_FlushDataCache_ADR;
	int x=200-(_strlen(str)*4);
	drawString(TOPFBADR1,str,x,y);
	drawString(TOPFBADR2,str,x,y);
	_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, TOPFBADR1, 240*400*3);
	_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, TOPFBADR2, 240*400*3);
}

void drawHex(u32 val, int x, int y)
{
	char str[9];

	hex2str(str,val);
	renderString(str,x,y);
}

Result _APT_PrepareToStartSystemApplet(Handle handle, NS_APPID appId)
{
	u32* cmdbuf=getThreadCommandBuffer();
	
	cmdbuf[0]=0x00190040; //request header code
	cmdbuf[1]=appId;
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;
	
	return cmdbuf[1];
}

Result _APT_StartSystemApplet(Handle handle, NS_APPID appId, u32* buffer, u32 bufferSize, Handle arg)
{
	u32* cmdbuf=getThreadCommandBuffer();
	
	cmdbuf[0]=0x001F0084; //request header code
	cmdbuf[1]=appId;
	cmdbuf[2]=bufferSize;
	cmdbuf[3]=0x0;
	cmdbuf[4]=arg;
	cmdbuf[5]=(bufferSize<<14)|0x2;
	cmdbuf[6]=(u32)buffer;
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;
	
	return cmdbuf[1];
}

Result _GSPGPU_AcquireRight(Handle handle, u8 flags)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x160042; //request header code
	cmdbuf[1]=flags;
	cmdbuf[2]=0x0;
	cmdbuf[3]=0xffff8001;

	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result _GSPGPU_ReleaseRight(Handle handle)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x170000; //request header code

	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

#define _aptOpenSession() \
	svcWaitSynchronization(aptLockHandle, U64_MAX);\
	_srv_getServiceHandle(srvHandle, &aptuHandle, "APT:U");\

#define _aptCloseSession()\
	svcCloseHandle(aptuHandle);\
	svcReleaseMutex(aptLockHandle);\

void doGspwn(u32* src, u32* dst, u32 size)
{
	Result (*nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue)(u32** sharedGspCmdBuf, u32* cmdAdr)=(Result (*)(u32**,u32*))(void*)CN_nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue;
	u32 gxCommand[]=
	{
		0x00000004, //command header (SetTextureCopy)
		(u32)src, //source address
		(u32)dst, //destination address
		size, //size
		0xFFFFFFFF, // dim in
		0xFFFFFFFF, // dim out
		0x00000008, // flags
		0x00000000, //unused
	};

	u32** sharedGspCmdBuf=(u32**)(CN_GSPSHAREDBUF_ADR);
	nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue(sharedGspCmdBuf, gxCommand);
}

Result _APT_ReceiveParameter(Handle handle, NS_APPID appID, u32 bufferSize, u32* buffer, u32* actualSize, u8* signalType, Handle* outHandle)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0xD0080; //request header code
	cmdbuf[1]=appID;
	cmdbuf[2]=bufferSize;
	
	cmdbuf[0+0x100/4]=(bufferSize<<14)|2;
	cmdbuf[1+0x100/4]=(u32)buffer;
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	if(!cmdbuf[1])
	{
		if(signalType)*signalType=cmdbuf[3];
		if(actualSize)*actualSize=cmdbuf[4];
		if(outHandle)*outHandle=cmdbuf[6];
	}

	return cmdbuf[1];
}

Result _APT_CancelParameter(Handle handle, NS_APPID appID)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0xF0100; //request header code
	cmdbuf[1]=0x0;
	cmdbuf[2]=0x0;
	cmdbuf[3]=0x0;
	cmdbuf[4]=appID;
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result __APT_MapProgramId(u32 appid, u64 titleid)
{
	Result ret=0;

	Handle srvHandle = *(Handle*)CN_SRVHANDLE_ADR;
	Handle lock = *(Handle*)CN_APTLOCKHANDLE_ADR;
	
	svcWaitSynchronization(lock, U64_MAX);
	Handle apthandle;
	ret = _srv_getServiceHandle(&srvHandle, &apthandle, "APT:U");
	if (ret) return ret;

	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0] = 0x110C00;
	cmdbuf[1] = appid;
	*(u16*)&cmdbuf[2] = titleid;
	
	if((ret=svcSendSyncRequest(apthandle)))return ret;
	
	svcCloseHandle(apthandle);
	svcReleaseMutex(lock);

	ret = (Result)cmdbuf[1];
	return ret;
}

Result __APT_Finalize(u32 appid)
{
	Result ret=0;

	Handle srvHandle = *(Handle*)CN_SRVHANDLE_ADR;
	Handle lock = *(Handle*)CN_APTLOCKHANDLE_ADR;
	
	svcWaitSynchronization(lock, U64_MAX);
	Handle apthandle;
	ret = _srv_getServiceHandle(&srvHandle, &apthandle, "APT:U");
	if (ret) return ret;

	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0] = 0x40040;
	cmdbuf[1] = appid;
	
	if((ret=svcSendSyncRequest(apthandle)))return ret;
	
	svcCloseHandle(apthandle);
	svcReleaseMutex(lock);

	ret = (Result)cmdbuf[1];
	return ret;
}

Result __APT_PrepareToDoApplicationJump(u32 flags, u64 titleid, u32 mediatype)
{
	Result ret=0;

	Handle srvHandle = *(Handle*)CN_SRVHANDLE_ADR;
	Handle lock = *(Handle*)CN_APTLOCKHANDLE_ADR;
	
	svcWaitSynchronization(lock, U64_MAX);
	Handle apthandle;
	ret = _srv_getServiceHandle(&srvHandle, &apthandle, "APT:U");
	if (ret) return ret;

	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0] = 0x310100;
	cmdbuf[1] = flags;
	*(u16*)&cmdbuf[2] = titleid;
	cmdbuf[4] = mediatype;
	
	if((ret=svcSendSyncRequest(apthandle)))return ret;
	
	svcCloseHandle(apthandle);
	svcReleaseMutex(lock);

	ret = (Result)cmdbuf[1];
	return ret;
}

Result __APT_DoApplicationJump()
{
	Result ret=0;
	u8 buf_big[0x300] = {0};
	u8 buf_small[0x20] = {0};

	Handle srvHandle = *(Handle*)CN_SRVHANDLE_ADR;
	Handle lock = *(Handle*)CN_APTLOCKHANDLE_ADR;
	
	svcWaitSynchronization(lock, U64_MAX);
	Handle apthandle;
	ret = _srv_getServiceHandle(&srvHandle, &apthandle, "APT:U");
	if (ret) return ret;

	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0] = 0x320084;
	cmdbuf[1] = 0x300;
	cmdbuf[2] = 0x20;
	cmdbuf[3] = (0x300<<14) | 2;
	cmdbuf[4] = (u32)&buf_big[0];
	cmdbuf[5] = (0x20<<14) | 0x802;
	cmdbuf[6] = (u32)&buf_small[0];
	
	if((ret=svcSendSyncRequest(apthandle)))return ret;
	
	svcCloseHandle(apthandle);
	svcReleaseMutex(lock);

	ret = (Result)cmdbuf[1];
	return ret;
}

Result _HB_FlushInvalidateCache(Handle handle)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x00010042; //request header code
	cmdbuf[1]=0x00100000;
	cmdbuf[2]=0x00000000;
	cmdbuf[3]=0xFFFF8001;
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result _HB_SetupBootloader(Handle handle, u32 addr)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x00020042; //request header code
	cmdbuf[1]=addr;
	cmdbuf[2]=0x00000000;
	cmdbuf[3]=0xFFFF8001;
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result _HB_GetHandle(Handle handle, u32 index, Handle* out)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x00040040; //request header code
	cmdbuf[1]=index;
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	if(out)*out=cmdbuf[5];

	return cmdbuf[1];
}

void bruteforceCloseHandle(u16 index, u32 maxCnt)
{
	int i;
	for(i=0;i<maxCnt;i++)if(!svcCloseHandle((index)|(i<<15)))return;
}

//no idea what this does; apparently used to switch up save partitions
Result FSUSER_ControlArchive(Handle handle, FS_archive archive)
{
	u32* cmdbuf=getThreadCommandBuffer();

	u32 b1, b2;
	((u8*)&b1)[0]=0x4e;
	((u8*)&b2)[0]=0xe4;

	cmdbuf[0]=0x080d0144;
	cmdbuf[1]=archive.handleLow;
	cmdbuf[2]=archive.handleHigh;
	cmdbuf[3]=0x0;
	cmdbuf[4]=0x1; //buffer1 size
	cmdbuf[5]=0x1; //buffer1 size
	cmdbuf[6]=0x1a;
	cmdbuf[7]=(u32)&b1;
	cmdbuf[8]=0x1c;
	cmdbuf[9]=(u32)&b2;
 
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;
 
	return cmdbuf[1];
}

void clearScreen(u8 shade)
{
	Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;
	Result (*_GSPGPU_FlushDataCache)(Handle* handle, Handle kprocess, u8* addr, u32 size)=(Result(*)(Handle*,Handle,u8*,u32))(void*)CN_GSPGPU_FlushDataCache_ADR;
	memset(TOPFBADR1, shade, 240*400*3);
	memset(TOPFBADR2, shade, 240*400*3);
	_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, TOPFBADR1, 240*400*3);
	_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, TOPFBADR2, 240*400*3);
}

void errorScreen(char* str, u32* dv, u8 n)
{
	clearScreen(0x00);
	renderString("FATAL ERROR",0,0);
	renderString(str,0,10);
	if(dv && n)
	{
		int i;
		for(i=0;i<n;i++)drawHex(dv[i], 8, 50+i*10);
	}
	while(1);
}

void drawTitleScreen(char* str)
{
	clearScreen(0x00);
	centerString("Legohax or someshit",0);
	centerString(BUILDTIME,10);
	centerString("http://somesite.net/somehax/",20);
	renderString(str, 0, 40);
}

u32 shitload = 0;

s32 kerneltest()
{
	__asm__ volatile("cpsid aif");
	*(volatile u32*)0xFFFD6204 = shitload;
	return 0;
}

int httpdownload(char* url, u8* dst)
{
	Result ret;

	Handle* srvHandle=(Handle*)CN_SRVHANDLE_ADR;
	Handle httpcHandle;
	ret=_srv_getServiceHandle(srvHandle, &httpcHandle, "http:C");

	Handle httpContextHandle=0x00;
	u32 sizeread = 0;

	ret=_HTTPC_Initialize(httpcHandle);
	ret=_HTTPC_CreateContext(httpcHandle, url, &httpContextHandle);

	Handle httpcHandle2;
	ret=_srv_getServiceHandle(srvHandle, &httpcHandle2, "http:C");
	if(ret) return -1;

	ret=_HTTPC_InitializeConnectionSession(httpcHandle2, httpContextHandle);
	if(ret) return -2;
	ret=_HTTPC_SetProxyDefault(httpcHandle2, httpContextHandle);
	if(ret) return -3;

	ret=_HTTPC_BeginRequest(httpcHandle2, httpContextHandle);
	if(ret) return -4;

	ret=_HTTPC_ReceiveData(httpcHandle2, httpContextHandle, dst, 0x100000); // FIXME!!
	if(ret) return -5;

	ret=_HTTPC_GetDownloadSizeState(httpcHandle2, httpContextHandle, &sizeread);
	if(ret) return -6;

	_HTTPC_CloseContext(httpcHandle2, httpContextHandle);
	if(ret) return -7;

	return sizeread;
}

/*void installerScreen(u32 size)
{
	char str[512] =
		"install the exploit to your savegame ?\n"
		"this will overwrite your custom levels\n"
		"and make some of the game inoperable.\n"
		"the exploit can later be uninstalled.\n"
		"    A : Yes \n"
		"    B : No  ";
	drawTitleScreen(str);

	while(1)
	{
		u32 PAD=((u32*)0x10000000)[7];
		drawHex(PAD,100,100);
		if(PAD&KEY_A)
		{
			//install
			int state=0;
			Result ret;
			Handle fsuHandle=*(Handle*)CN_FSHANDLE_ADR;
			FS_archive saveArchive=(FS_archive){0x00000004, (FS_path){PATH_EMPTY, 1, (u8*)""}};
			u32 totalWritten;
			Handle fileHandle;

			_strappend(str, "\n\n   installing..."); drawTitleScreen(str);

			ret=FSUSER_OpenArchive(fsuHandle, &saveArchive);
			state++; if(ret)goto installEnd;

			//write exploit map file
			ret=FSUSER_OpenFile(fsuHandle, &fileHandle, saveArchive, FS_makePath(PATH_CHAR, "/edit/mslot0.map"), FS_OPEN_WRITE|FS_OPEN_CREATE, FS_ATTRIBUTE_NONE);
			state++; if(ret)goto installEnd;
			ret=FSFILE_Write(fileHandle, &totalWritten, 0x0, (u32*)cn_save_initial_loader_bin, cn_save_initial_loader_bin_size, 0x10001);
			state++; if(ret)goto installEnd;
			ret=FSFILE_Close(fileHandle);
			state++; if(ret)goto installEnd;

			//write secondary payload file
			ret=FSUSER_OpenFile(fsuHandle, &fileHandle, saveArchive, FS_makePath(PATH_CHAR, "/edit/payload.bin"), FS_OPEN_WRITE|FS_OPEN_CREATE, FS_ATTRIBUTE_NONE);
			state++; if(ret)goto installEnd;
			ret=FSFILE_Write(fileHandle, &totalWritten, 0x0, (u32*)0x14300000, size, 0x10001);
			state++; if(ret)goto installEnd;
			ret=FSFILE_Close(fileHandle);
			state++; if(ret)goto installEnd;

			ret=FSUSER_ControlArchive(fsuHandle, saveArchive);
			state++; if(ret)goto installEnd;

			ret=FSUSER_CloseArchive(fsuHandle, &saveArchive);
			state++; if(ret)goto installEnd;

			installEnd:
			if(ret)
			{
				u32 v[2]={ret, state};
				errorScreen("   installation process failed.\n   please report the below information by\n   email to sme@lum.sexy", v, 2);
			}

			_strappend(str, " done.\n\n   press A to run the exploit."); drawTitleScreen(str);
			u32 oldPAD=((u32*)0x10000000)[7];
			while(1)
			{
				u32 PAD=((u32*)0x10000000)[7];
				if(((PAD^oldPAD)&KEY_A) && (PAD&KEY_A))break;
				drawHex(PAD,200,200);
				oldPAD=PAD;
			}

			break;
		}else if(PAD&KEY_B)break;
	}
}*/



Result dbgcolor(u32 color)
{
	Handle* handle=(Handle*)CN_GSPHANDLE_ADR;
	//color |= 0x01000000;

	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x00010082; //request header code
	cmdbuf[1]=0x202A04;
	cmdbuf[2]=4;
	cmdbuf[3]=(4<<14)|2;
	cmdbuf[4]=(u32)&color;

	Result ret=0;
	if((ret=svcSendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result GSPGPU_FlushDataCache(Handle* handle, u32* adr, u32 size)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x00080082; //request header code
	cmdbuf[1]=(u32)adr;
	cmdbuf[2]=size;
	cmdbuf[3]=0x0;
	cmdbuf[4]=0xffff8001;

	Result ret=0;
	if((ret=svcSendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result GSPGPU_InvalidateDataCache(Handle* handle, u32* adr, u32 size)
{
	Result ret=0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x00090082;
	cmdbuf[1] = (u32)adr;
	cmdbuf[2] = size;
	cmdbuf[3] = 0;
	cmdbuf[4] = 0xFFFF8001;

	if((ret=svcSendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

u32 computeCodeAddress(u32 offset)
{
	return CN_GSPHEAP+CN_TEXTPA_OFFSET_FROMEND+FIRM_APPMEMALLOC+offset;
}

/*void crappyshit()
{
	u32 buf;

	_GSPGPU_ReleaseRight(*gspHandle); //disable GSP module access

	_aptOpenSession();
		ret=_APT_PrepareToStartSystemApplet(aptuHandle, APPID_WEB);
	_aptCloseSession();
	//drawTitleScreen("running exploit... 000%");

	_aptOpenSession();
		ret=_APT_StartSystemApplet(aptuHandle, APPID_WEB, &buf, 0, 0);
	_aptCloseSession();
	//drawTitleScreen("running exploit... 020%");

	svcSleepThread(100000000); //sleep just long enough for menu to grab rights

	_GSPGPU_AcquireRight(*gspHandle, 0x0); //get in line for gsp rights
}*/

u32 derpo[4];

s32 PatchNS()
{
	__asm__ volatile("cpsid aif");
	
	KCodeSet* ns_codeset = FindTitleCodeSet("ns", 2);
	//u32* ptr = (u32*)FindCodeOffsetKAddr(ns_codeset, 0x836C);
	//*ptr = 0xE1A00000;
	/*u32* ptr = (u32*)FindCodeOffsetKAddr(ns_codeset, 0xE6C4);//0x26EC);
	*ptr = 0xE1A00000;//0xE12FFF1E;*/
	
	// 119000
	// 20C960
	
	u32* ptr = (u32*)FindDataOffsetKAddr(ns_codeset, /*0x20C960*/0x11AE60-0x119000);
	derpo[0] = *ptr++;
	derpo[1] = *ptr++;
	derpo[2] = *ptr++;
	derpo[3] = *ptr++;
	
	KernelFlushCaches();
	return 0;
}

#define ARM_BRANCH(src, dst) (0xEA000000 | (((((u32)(dst)) - ((u32)(src)) - 8) >> 2) & 0x00FFFFFF))

s32 PatchFS()
{
	__asm__ volatile("cpsid aif");
	
	KCodeSet* codeset = FindTitleCodeSet("fs", 2);
	//u32* ptr = (u32*)FindCodeOffsetKAddr(ns_codeset, 0x836C);
	//*ptr = 0xE1A00000;
	/*u32* ptr = (u32*)FindCodeOffsetKAddr(ns_codeset, 0xE6C4);//0x26EC);
	*ptr = 0xE1A00000;//0xE12FFF1E;*/
	
	// 119000
	// 20C960
	
	u32 patchaddr = 0x12FA78; // TODO!! compute dynamically
	u32 branchdst = 0x12FD94;
	
	u32* ptr = (u32*)FindCodeOffsetKAddr(codeset, patchaddr-0x100000);
	u32* patch = FSPatchStart;
	while (patch < FSPatchEnd)
	{
		u32 val = *patch++;
		if (val == 0x77770001)
			val = ARM_BRANCH(patchaddr, branchdst);
			
		*ptr++ = val;
		patchaddr += 4;
	}
	
	KernelFlushCaches();
	return 0;
}

u32 getsp();
void clearregs();
void dumpregs(u32* space);



void niceend()
{
	Result res;
	
	//unmap GSP and HID shared mem
	/*res = svcUnmapMemoryBlock(*((Handle*)CN_HIDMEMHANDLE_ADR), 0x10000000);
	if (res) errorScreen("failed to free HID sharedmem", (u32*)&res, 1);
	
	res = svcUnmapMemoryBlock(*((Handle*)CN_GSPMEMHANDLE_ADR), 0x10002000);
	if (res) errorScreen("failed to free GSP sharedmem", (u32*)&res, 1);*/
	
	Handle _srvHandle=*(Handle*)CN_SRVHANDLE_ADR;
	Handle _gspHandle=*(Handle*)CN_GSPHANDLE_ADR;

	//close all handles in data and .bss sections
	int i;
	for(i=0;i<(CN_DATABSS_SIZE)/4;i++)
	{
		Handle val=((Handle*)(CN_DATABSS_START))[i];
		if(val && (val&0x7FFF)<0x30 && val!=_srvHandle && val!=_gspHandle)svcCloseHandle(val);
	}

	//bruteforce the cnt part of the remaining 3 handles
	bruteforceCloseHandle(0x4, 0x1FFFF);
	bruteforceCloseHandle(0xA, 0x1FFFF);
	bruteforceCloseHandle(0x1E, 0x1FFFF);

	//free GSP heap and regular heap
	//u32 out;
	//svcControlMemory(&out, 0x08000000, 0x00000000, CN_HEAPSIZE, MEMOP_FREE, (MemPerm)0x0);
	//svcControlMemory(&out, 0x14000000, 0x00000000, 0x02000000, MEMOP_FREE, (MemPerm)0x0);
	
	_GSPGPU_ReleaseRight(_gspHandle); //disable GSP module access
	svcCloseHandle(_gspHandle);
	
	svcCloseHandle(_srvHandle);
	svcExitProcess();
}



int main(u32 size, char** argv)
{
	int line=10;
	drawTitleScreen("");
	
	// TODO: do install before khaxing!
	// khaxing will destroy shit at 0x14300000
	
	// free linear memory first
	u32 crapo;
	Result hax;
	//Handle test;
	
	//MemInfo meminfo; PageInfo pageinfo;
	
	/*u32 regs[4];
	Handle darp = *(Handle*)CN_GSPHANDLE_ADR;
	
	clearregs();
	//svcGetThreadId(&crapo, 0xFFFF8000); // R2=2 R3=0FFFFFE4 R12=0
	//svcSleepThread(1); // R1=FFF76D8C R2=FFFE30C8 R3=FFF8EDEC R12=FFFF7F00
	//svcGetProcessId(&crapo, 0xFFFF8001); // R2=F R3=0FFFFFE4 R12=0
	//svcWaitSynchronization(test, 1); // R1=FFF0174C R2=1
	//svcCreateEvent(&test, 0); // R2=0FFFFFE4
	//svcSetThreadPriority(0xFFFF8000, 0x28); // R1=FFF12DB4 R2=2
	//svcControlMemory(&crapo, (u32*)CN_GSPHEAP+0x7000, 0, 0x00007000, MEMOP_FREE, (MemPerm)0); // R1=1401C000 R2=0FFFFFE4
	// 14007000 -> 1401C000
	// 14010000 -> 14040000
	//svcQueryMemory(&meminfo, &pageinfo, 0x14000000); // R1=14000000 R2=02000000 R3=00000003 R12=FF533EC4
	
	// 0xEC4
	dumpregs(regs);
	errorScreen("regdump", regs, 4);*/
	
	/*u32* nopsled = (u32*)CN_GSPHEAP;
	u32 p = 0, i;
	for (i = 0; i < 0x1000>>2; i++)
		nopsled[i] = 0xE1A00000;
		
	Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;
	GSPGPU_FlushDataCache(gspHandle, (u32*)nopsled, size);
	GSPGPU_InvalidateDataCache(gspHandle, (u32*)0x00100000, 0x00008000);
		
	for (p = 0; p < 0x7000; p += 0x1000)
	{
		doGspwn(nopsled, (u32*)computeCodeAddress(p), 0x00001000);
		svcSleepThread(100000000);
	}
	
	nopsled[0x3FF] = 0xE12FFF1E;
	GSPGPU_FlushDataCache(gspHandle, (u32*)nopsled, size);
	
	doGspwn(nopsled, (u32*)computeCodeAddress(0x7000), 0x00001000);
	svcSleepThread(100000000);
	
	flushcaches();*/
	
	// TODO pointer for other CN regions!
	//hax = svcControlMemory(&crapo, *(u32*)0x334F38, 0, 0x02000000, MEMOP_FREE, (MemPerm)0);
	/*hax = svcControlMemory(&crapo, (u32*)CN_GSPHEAP, 0, 0x00007000, MEMOP_FREE, (MemPerm)0);
	if (hax)
	{
		errorScreen("linearfree failed", (u32*)&hax, 1);
		for(;;);
	}*/
	
	//u32 sp = GetMagicalPointer();
	//errorScreen("magical", &sp, 1);
	
	//u32 sp1 = getsp();
	//dbgcolor(0xFF0000);
	hax = khaxInit();
	
	//svcSleepThread(1);
	//*(u32*)0 = 0x12345678;
	
	//u32 sp2 = getsp();
	
	//if (sp1 != sp2)
	/*{
		u32 dorp[2] = {sp1, sp2};
		errorScreen("stack fucked!", dorp, 2);
	}*/
	//dbgcolor(0xFF);
	
	if (!hax)
		drawTitleScreen("khaxed!");
	else
		errorScreen("khax failed", (u32*)&hax, 1);
		
	dbgcolor(0);
		
		
	u32 addr = 0x14000000;
	u32 out;
	while (addr < 0x16000000)
	{
		svcControlMemory(&out, addr, 0, 0x00001000, MEMOP_FREE, (MemPerm)0);
		addr += 0x1000;
	}
	
	hax = svcControlMemory(&out, 0, 0, 0x01000000, MEMOP_ALLOC_LINEAR, (MemPerm)3);
	if (hax)
	{
		dbgcolor(0x010000FF);
		errorScreen("linear realloc failed", (u32*)&hax, 1);
	}
	hax = svcControlMemory(&out, 0x08000000, 0x00000000, CN_HEAPSIZE, MEMOP_FREE, (MemPerm)0x0);
	if (hax)
	{
		dbgcolor(0x010000FF);
		errorScreen("heap free failed", (u32*)&hax, 1);
	}
	/*hax = svcControlMemory(&out, 0x08000000, 0x00000000, 0x02000000, MEMOP_ALLOC, (MemPerm)0x3);
	if (hax)
	{
		dbgcolor(0x010000FF);
		errorScreen("heap realloc failed", (u32*)&hax, 1);
	}*/
		
	/*Handle rsrclimit;
	svcGetResourceLimit(&rsrclimit, 0xFFFF8001);
	// 18
	// 04000000
	// 20
	// 20
	// 20
	// 08
	// 08
	// 10
	// 02
	// 0
	
	s64 values[16] = {0};
	u32 names[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
	hax = svcGetResourceLimitLimitValues(values, rsrclimit, names, 16);
	errorScreen("res limit", (u32*)values, 32);
		*/

	SaveVersionConstants();
	svcBackdoor(PatchFS);
	
	/*errorScreen("derp!", derpo, 4);*/
	
	
	int s;


	/*s = httpdownload("http://192.168.1.240/testprocess.3dsx", (u8*)0x14000000);
	if (s < 0)
		errorScreen("temp. download failed", (u32*)&s, 1);
		
	s = Load3DSX((u8*)0x14000000);
	if (s != 0)
	{
		//dbgcolor(0);
		//svcControlMemory(&crapo, 0, 0, 0x01000000, MEMOP_ALLOC_LINEAR, (MemPerm)3);
		errorScreen("3DSX loading failed", (u32*)&s, 1);
	}
	
	svcSleepThread(1000000000);*/
	
	
	
	/*u32 darp[10] = {0};
	svcQueryMemory((MemInfo*)&darp[0], (PageInfo*)&darp[4], 0x10003000);
	svcQueryMemory((MemInfo*)&darp[5], (PageInfo*)&darp[9], 0x10004000);
	errorScreen("sharedmem", darp, 10);
	*/
	
	
	//__APT_MapProgramId(0x300, 0x0004000012345678);
	
	__APT_Finalize(0x300);
	
	
	s = httpdownload("http://192.168.1.240/hello-world.3dsx", (u8*)0x14000000);
	//s = httpdownload("http://192.168.1.240/blargSNES.3dsx", (u8*)0x14000000);
	if (s < 0)
		errorScreen("temp. download 2 failed", (u32*)&s, 1);
		
		
	hax = svcUnmapMemoryBlock(*((Handle*)CN_HIDMEMHANDLE_ADR), 0x10000000);
	if (hax) errorScreen("failed to free HID sharedmem", (u32*)&hax, 1);
	
	hax = svcUnmapMemoryBlock(*((Handle*)CN_GSPMEMHANDLE_ADR), 0x10002000);
	if (hax) errorScreen("failed to free GSP sharedmem", (u32*)&hax, 1);
	
	//dbgcolor(0x010000FF);
	s = Load3DSX_2((u8*)0x14000000);
	if (s != 0)
	{
		//dbgcolor(0);
		svcControlMemory(&crapo, 0, 0, 0x01000000, MEMOP_ALLOC_LINEAR, (MemPerm)3);
		errorScreen("3DSX loading 2 failed", (u32*)&s, 1);
	}
	//svcSleepThread(1000000000);
	
	//for (;;) svcSleepThread(100000000);
	niceend();
	
	
	//hax = __APT_MapProgramId(0x300, 0x000401300BADC0DE);
	/*hax = __APT_MapProgramId(0x300, 0x000400000007AF00);
	if (hax)
	{
		dbgcolor(0x01FF0000);
		svcControlMemory(&crapo, 0, 0, 0x01000000, MEMOP_ALLOC_LINEAR, (MemPerm)3);
		errorScreen("apt shit #1 failed", (u32*)&hax, 1);
	}
	
	hax = __APT_PrepareToDoApplicationJump(2, 0, 0);
	//hax = __APT_PrepareToDoApplicationJump(0, 0x000400000007AF00, 1);
	if (hax)
	{
		dbgcolor(0x0100FF00);
		svcControlMemory(&crapo, 0, 0, 0x01000000, MEMOP_ALLOC_LINEAR, (MemPerm)3);
		errorScreen("apt shit #2 failed", (u32*)&hax, 1);
	}
	
	hax = __APT_DoApplicationJump();
	if (hax)
	{
		dbgcolor(0x010000FF);
		svcControlMemory(&crapo, 0, 0, 0x01000000, MEMOP_ALLOC_LINEAR, (MemPerm)3);
		errorScreen("apt shit #3 failed", (u32*)&hax, 1);
	}*/
	
	
	
	/*u32 colors[6] = {0x010000FF, 0x0100FFFF, 0x0100FF00, 0x01FFFF00, 0x01FF0000, 0x01FF00FF};
	u32 ncolor=0;
	
	for (;;)
	{
		dbgcolor(colors[ncolor]);
		ncolor++;
		if (ncolor >= 6) ncolor=0;
		svcSleepThread(100000000);
	}*/
	
	
	
	/*Handle* srvHandle=(Handle*)CN_SRVHANDLE_ADR;
	Handle hb;
	hax=_srv_getServiceHandle(srvHandle, &hb, "hb:HB");
	if (hax)
	{
		svcControlMemory(&crapo, 0, 0, 0x01000000, MEMOP_ALLOC_LINEAR, (MemPerm)3);
		errorScreen("failed to get hb:HB", (u32*)&hax, 1);
	}
	
	
	u32 colors[6] = {0x010000FF, 0x0100FFFF, 0x0100FF00, 0x01FFFF00, 0x01FF0000, 0x01FF00FF};
	u32 ncolor=0;
	
	for (;;)
	{
		u32* cmdbuf = getThreadCommandBuffer();
		cmdbuf[0] = 0x10040;
		cmdbuf[1] = colors[ncolor];
		hax = svcSendSyncRequest(hb);
		if (hax || cmdbuf[1])
		{
			svcControlMemory(&crapo, 0, 0, 0x01000000, MEMOP_ALLOC_LINEAR, (MemPerm)3);
			errorScreen("failed to send command", (u32*)&hax, 1);
		}
		
		ncolor++;
		if (ncolor >= 6) ncolor=0;
		svcSleepThread(100000000);
	}
	svcCloseHandle(hb);*/
	
	
	
	
	//niceend();
	for (;;) svcSleepThread(1000000000);
	
	
	
	//svcExitProcess();

/*	Handle* srvHandle=(Handle*)CN_SRVHANDLE_ADR;
	Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;

	Handle aptLockHandle=*((Handle*)CN_APTLOCKHANDLE_ADR);
	Handle aptuHandle=0x00;
	Result ret;

	u8 recvbuf[0x1000];

	Result (*_GSPGPU_FlushDataCache)(Handle* handle, Handle kprocess, u32* addr, u32 size)=(Result(*)(Handle*,Handle,u8*,u32))(void*)CN_GSPGPU_FlushDataCache_ADR;

	if(size)installerScreen(size);

	{
		u32 buf;

		_GSPGPU_ReleaseRight(*gspHandle); //disable GSP module access

		_aptOpenSession();
			ret=_APT_PrepareToStartSystemApplet(aptuHandle, APPID_WEB);
		_aptCloseSession();
		drawTitleScreen("running exploit... 000%");

		_aptOpenSession();
			ret=_APT_StartSystemApplet(aptuHandle, APPID_WEB, &buf, 0, 0);
		_aptCloseSession();
		drawTitleScreen("running exploit... 020%");

		svcSleepThread(100000000); //sleep just long enough for menu to grab rights

		_GSPGPU_AcquireRight(*gspHandle, 0x0); //get in line for gsp rights

		//need to sleep longer on 4.x ?
		svcSleepThread(1000000000); //sleep long enough for spider to startup

		//read spider memory
		{
			_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, (u32*)0x14100000, 0x00000300);
			
			doGspwn((u32*)(SPIDER_HOOKROP_PADR+FIRM_LINEAROFFSET), (u32*)0x14100000, 0x00000300);
		}

		svcSleepThread(1000000); //sleep long enough for memory to be read

		//patch memdump and write it
		{
			((u8*)0x14100000)[SPIDER_HOOKROP_KILLOFFSET]=0xFF;
			memcpy(((u8*)(0x14100000+SPIDER_HOOKROP_OFFSET)), spider_hook_rop_bin, 0xC);
			_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, (u32*)0x14100000, 0x00000300);

			doGspwn((u32*)0x14100000, (u32*)(SPIDER_HOOKROP_PADR+FIRM_LINEAROFFSET), 0x00000300);
		}

		svcSleepThread(100000000);

		{
			memset((u8*)0x14100000, 0x00, 0x2000);
			memcpy((u8*)0x14100000, spider_initial_rop_bin, spider_initial_rop_bin_size);
			_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, (u32*)0x14100000, 0x1000);

			doGspwn((u32*)0x14100000, (u32*)(SPIDER_INITIALROP_PADR+FIRM_LINEAROFFSET), 0x1000);
		}

		svcSleepThread(100000000);

		{
			memset((u8*)0x14100000, 0x00, 0x2000);
			memcpy((u8*)0x14100000, spider_thread0_rop_bin, spider_thread0_rop_bin_size);
			_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, (u32*)0x14100000, 0x2000);

			doGspwn((u32*)0x14100000, (u32*)(SPIDER_THREAD0ROP_PADR+FIRM_LINEAROFFSET), 0x2000);
		}

		svcSleepThread(100000000);//sleep long enough for memory to be written

		_aptOpenSession();
			_APT_CancelParameter(aptuHandle, APPID_WEB);
		_aptCloseSession();

		//cleanup
		{
			//unmap GSP and HID shared mem
			svcUnmapMemoryBlock(*((Handle*)CN_HIDMEMHANDLE_ADR), 0x10000000);
			svcUnmapMemoryBlock(*((Handle*)CN_GSPMEMHANDLE_ADR), 0x10002000);

			Handle _srvHandle=*srvHandle;
			Handle _gspHandle=*gspHandle;

			//close all handles in data and .bss sections
			int i;
			for(i=0;i<(CN_DATABSS_SIZE)/4;i++)
			{
				Handle val=((Handle*)(CN_DATABSS_START))[i];
				if(val && (val&0x7FFF)<0x30 && val!=_srvHandle && val!=_gspHandle)svcCloseHandle(val);
			}

			//bruteforce the cnt part of the remaining 3 handles
			bruteforceCloseHandle(0x4, 0x1FFFF);
			bruteforceCloseHandle(0xA, 0x1FFFF);
			bruteforceCloseHandle(0x1E, 0x1FFFF);

			//free GSP heap and regular heap
			u32 out;
			svcControlMemory(&out, 0x08000000, 0x00000000, CN_HEAPSIZE, MEMOP_FREE, 0x0);
		}

		drawTitleScreen("running exploit... 040%");

		_GSPGPU_ReleaseRight(*gspHandle); //disable GSP module access
	}

	svcSleepThread(100000000); //sleep just long enough for spider to grab rights

	_GSPGPU_AcquireRight(*gspHandle, 0x0); //get in line for gsp rights

	drawTitleScreen("running exploit... 070%");

	u32* debug=(u32*)CN_DATABSS_START;
	debug[6]=0xDEADBABE;
	debug[7]=0xDEADBABE;
	debug[8]=0xDEADBABE;
	debug[9]=0xDEADBABE;

	ret=0xC880CFFA;
	while(ret==0xC880CFFA || ret==0xC8A0CFEF)
	{
		_aptOpenSession();
			ret=_APT_ReceiveParameter(aptuHandle, APPID_APPLICATION, 0x1000, (u32*)recvbuf, &debug[6], (u8*)&debug[7], (Handle*)&debug[8]);
			debug[0]=0xDEADCAFE;
			debug[1]=0xDEADBABE;
			debug[2]=debug[1]+1;
			debug[3]=debug[2]+1;
			debug[4]=debug[3]+1;
			debug[5]=ret;
		_aptCloseSession();
	}

	Handle hbHandle=debug[8];
	debug[8]=0x0;
	_HB_FlushInvalidateCache(hbHandle);
	Handle fsHandle;
	debug[8]=_HB_GetHandle(hbHandle, 0x0, &fsHandle);

	// //allocate some memory for the bootloader code (will be remapped)
	// u32 out; ret=svcControlMemory(&out, 0x13FF0000, 0x00000000, 0x00008000, MEMOP_COMMIT, 0x3);
	//allocate some memory for homebrew .text/rodata/data/bss... (will be remapped)
	u32 out; ret=svcControlMemory(&out, CN_ALLOCPAGES_ADR, 0x00000000, CN_ADDPAGES*0x1000, MEMOP_COMMIT, 0x3);

	drawTitleScreen("running exploit... 080%");

	if(_HB_SetupBootloader(hbHandle, 0x13FF0000))*((u32*)NULL)=0xBABE0061;

	drawTitleScreen("running exploit... 090%");
	
	memcpy((u8*)0x00100000, cn_bootloader_bin, cn_bootloader_bin_size);
	
	drawTitleScreen("running exploit... 095%");

	_HB_FlushInvalidateCache(hbHandle);

	drawTitleScreen("running exploit... 100%");

	//open sdmc 3dsx file
	Handle fileHandle;
	FS_archive sdmcArchive=(FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath=(FS_path){PATH_CHAR, 11, (u8*)"/boot.3dsx"};
	if((ret=FSUSER_OpenFileDirectly(fsHandle, &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE))!=0)
	{
		errorScreen("   failed to open sd:/boot.3dsx.\n    does it exist ?", (u32*)&ret, 1);
	}
	
	svcControlMemory(&out, 0x14000000, 0x00000000, 0x02000000, MEMOP_FREE, 0x0);

	void (*callBootloader)(Handle hb, Handle file)=(void*)CN_BOOTLOADER_LOC;
	void (*setArgs)(u32* src, u32 length)=(void*)CN_ARGSETTER_LOC;
	_GSPGPU_ReleaseRight(*gspHandle); //disable GSP module access
	svcCloseHandle(*gspHandle);

	setArgs(NULL, 0);
	callBootloader(hbHandle, fileHandle);*/

	while(1);
	return 0;
}
