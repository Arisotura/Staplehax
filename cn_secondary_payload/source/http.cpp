#include <3ds/types.h>
#include "svc.h"


int _strlen(char* str);


Result _HTTPC_Initialize(Handle handle)
{
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0x10044; //request header code
	cmdbuf[1]=0x1000; //unk
	cmdbuf[2]=0x20; //unk
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result _HTTPC_CreateContext(Handle handle, char* url, Handle* contextHandle)
{
	u32* cmdbuf=getThreadCommandBuffer();
	u32 l=_strlen(url)+1;

	cmdbuf[0]=0x20082; //request header code
	cmdbuf[1]=l;
	cmdbuf[2]=0x01; //unk
	cmdbuf[3]=(l<<4)|0xA;
	cmdbuf[4]=(u32)url;
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;
	
	if(contextHandle)*contextHandle=cmdbuf[2];

	return cmdbuf[1];
}

Result _HTTPC_InitializeConnectionSession(Handle handle, Handle contextHandle)
{
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0x80042; //request header code
	cmdbuf[1]=contextHandle;
	cmdbuf[2]=0x20; //unk, fixed to that in code
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result _HTTPC_SetProxyDefault(Handle handle, Handle contextHandle)
{
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0xe0040; //request header code
	cmdbuf[1]=contextHandle;
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result _HTTPC_CloseContext(Handle handle, Handle contextHandle)
{
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0x30040; //request header code
	cmdbuf[1]=contextHandle;
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result _HTTPC_BeginRequest(Handle handle, Handle contextHandle)
{
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0x90040; //request header code
	cmdbuf[1]=contextHandle;
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result _HTTPC_ReceiveData(Handle handle, Handle contextHandle, u8* buffer, u32 size)
{
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0xB0082; //request header code
	cmdbuf[1]=contextHandle;
	cmdbuf[2]=size;
	cmdbuf[3]=(size<<4)|12;
	cmdbuf[4]=(u32)buffer;
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result _HTTPC_GetDownloadSizeState(Handle handle, Handle contextHandle, u32* totalSize)
{
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0x00060040; //request header code
	cmdbuf[1]=contextHandle;
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	if(totalSize)*totalSize=cmdbuf[3];

	return cmdbuf[1];
}
