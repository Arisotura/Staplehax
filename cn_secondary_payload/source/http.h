#pragma once


Result _HTTPC_Initialize(Handle handle);
Result _HTTPC_CreateContext(Handle handle, char* url, Handle* contextHandle);
Result _HTTPC_InitializeConnectionSession(Handle handle, Handle contextHandle);
Result _HTTPC_SetProxyDefault(Handle handle, Handle contextHandle);
Result _HTTPC_CloseContext(Handle handle, Handle contextHandle);
Result _HTTPC_BeginRequest(Handle handle, Handle contextHandle);
Result _HTTPC_ReceiveData(Handle handle, Handle contextHandle, u8* buffer, u32 size);
Result _HTTPC_GetDownloadSizeState(Handle handle, Handle contextHandle, u32* totalSize);
