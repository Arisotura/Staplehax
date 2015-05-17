/*
  srv.c _ Service manager.
*/

#include <string.h>
#include <3ds/types.h>
#include "srv.h"
#include "svc.h"


static Handle g_srv_handle = 0;


Result srvInit()
{
	Result rc = 0;

	if(g_srv_handle != 0) return rc;

	if((rc = svcConnectToPort(&g_srv_handle, "srv:")))return rc;

	if((rc = srvRegisterClient())) {
		svcCloseHandle(g_srv_handle);
		g_srv_handle = 0;
	}

	return rc;
}

Result srvExit()
{
	if(g_srv_handle != 0)svcCloseHandle(g_srv_handle);

	g_srv_handle = 0;
	return 0;
}

Result srvRegisterClient()
{
	Result rc = 0;
	
	u32* cmdbuf = getThreadCommandBuffer();
	
	cmdbuf[0] = 0x10002;
	cmdbuf[1] = 0x20;

	if((rc = svcSendSyncRequest(g_srv_handle)))return rc;

	return cmdbuf[1];
}

Result srvGetServiceHandle(Handle* out, const char* name)
{
	Result rc = 0;

	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = 0x50100;
	strcpy((char*) &cmdbuf[1], name);
	cmdbuf[3] = strlen(name);
	cmdbuf[4] = 0x0;
	
	if((rc = svcSendSyncRequest(g_srv_handle)))return rc;

	*out = cmdbuf[3];
	return cmdbuf[1];
}

Result srvRegisterService(Handle* out, const char* name, u32 maxsessions)
{
	Result rc = 0;

	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = 0x30100;
	strcpy((char*) &cmdbuf[1], name);
	cmdbuf[3] = strlen(name);
	cmdbuf[4] = maxsessions;
	
	if((rc = svcSendSyncRequest(g_srv_handle)))return rc;

	*out = cmdbuf[3];
	return cmdbuf[1];
}

Result srvUnegisterService(const char* name)
{
	Result rc = 0;

	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = 0x400C0;
	strcpy((char*) &cmdbuf[1], name);
	cmdbuf[3] = strlen(name);
	
	if((rc = svcSendSyncRequest(g_srv_handle)))return rc;

	return cmdbuf[1];
}

Result srvGetProcSemaphore(Handle* out)
{
	Result rc = 0;

	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = 0x20000;
	
	if((rc = svcSendSyncRequest(g_srv_handle)))return rc;

	*out = cmdbuf[3];
	return cmdbuf[1];
}

Result srvSubscribe(u32 notif)
{
	Result rc = 0;

	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = 0x90040;
	cmdbuf[1] = notif;
	
	if((rc = svcSendSyncRequest(g_srv_handle)))return rc;

	return cmdbuf[1];
}

Result srvUnsubscribe(u32 notif)
{
	Result rc = 0;

	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = 0xA0040;
	cmdbuf[1] = notif;
	
	if((rc = svcSendSyncRequest(g_srv_handle)))return rc;

	return cmdbuf[1];
}

Result srvReceiveNotification(u32* out)
{
	Result rc = 0;

	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = 0xB0000;
	
	if((rc = svcSendSyncRequest(g_srv_handle)))return rc;

	*out = cmdbuf[2];
	return cmdbuf[1];
}

// Old srv:pm interface, will only work on systems where srv:pm was a port (<7.X)
Result srvPmInit()
{	
	Result rc = 0;
	
	if((rc = svcConnectToPort(&g_srv_handle, "srv:pm")))return rc;
	
	if((rc = srvRegisterClient())) {
		svcCloseHandle(g_srv_handle);
		g_srv_handle = 0;
	}

	return rc;
}

Result srvRegisterProcess(u32 procid, u32 count, void *serviceaccesscontrol)
{
	Result rc = 0;
	
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x04030082; // <7.x
	cmdbuf[1] = procid;
	cmdbuf[2] = count;
	cmdbuf[3] = (count << 16) | 2;
	cmdbuf[4] = (u32)serviceaccesscontrol;
	
	if((rc = svcSendSyncRequest(g_srv_handle))) return rc;
		
	return cmdbuf[1];
}

Result srvUnregisterProcess(u32 procid)
{
	Result rc = 0;
	
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x04040040; // <7.x
	cmdbuf[1] = procid;
	
	if((rc = svcSendSyncRequest(g_srv_handle))) return rc;
		
	return cmdbuf[1];
}
