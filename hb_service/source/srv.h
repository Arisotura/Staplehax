#pragma once

Result srvInit();
Result srvExit();
Result srvRegisterClient();
Result srvGetServiceHandle(Handle* out, const char* name);

Result srvRegisterService(Handle* out, const char* name, u32 maxsessions);
Result srvUnegisterService(const char* name);
Result srvGetProcSemaphore(Handle* out);
Result srvSubscribe(u32 notif);
Result srvUnsubscribe(u32 notif);
Result srvReceiveNotification(u32* out);

Result srvPmInit();
Result srvRegisterProcess(u32 procid, u32 count, void *serviceaccesscontrol);
Result srvUnregisterProcess(u32 procid);
