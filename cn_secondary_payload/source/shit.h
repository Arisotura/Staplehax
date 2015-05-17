#pragma once

#ifdef __cplusplus
extern "C" {
#endif

u32 GetMagicalPointer();
Result svcCreateCodeSet(Handle* codeset, void* codesetinfo, void* code, void* rodata, void* data);
Result svcCreateResourceLimit(Handle* rsrclimit);
Result svcSetResourceLimitValues(Handle rsrclimit, u32* typelist, s64* valuelist, s32 num);
Result svcCreateProcess(Handle* process, Handle codeset, void* arm11caps, u32 arm11caps_num);
Result svcSetProcessResourceLimits(Handle process, Handle rsrclimit);
Result svcSetProcessAffinityMask(Handle process, u8* affinitymask, s32 processorcount);
Result svcSetProcessIdealProcessor(Handle process, s32 idealprocessor);
Result svcRun(Handle process, void* startupinfo);
Result svcGetResourceLimit(Handle* rsrclimit, Handle process);
Result svcGetResourceLimitLimitValues(s64* values, Handle rsrclimit, u32* names, s32 num);
Result svcKernelSetState(u32 type, u32 v0, u32 v1, u32 v2);

s32 KernelFlushCaches();

extern u32 FSPatchStart[], FSPatchEnd[];

#ifdef __cplusplus
}
#endif
