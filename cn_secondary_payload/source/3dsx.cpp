#include <3ds/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "3dsx.h"
#include "svc.h"
#include "shit.h"

#include "../../build/constants.h"

#define KHAX_lengthof(...) (sizeof(__VA_ARGS__) / sizeof((__VA_ARGS__)[0]))


#define ARM_BRANCH(src, dst) (0xEA000000 | (((((u32)(dst)) - ((u32)(src)) - 8) >> 2) & 0x00FFFFFF))




// http://192.168.1.240/testprocess.3dsx


Result dbgcolor(u32 color);

Result _srv_getServiceHandle(Handle* handleptr, Handle* out, char* server);
Result _GSPGPU_ReleaseRight(Handle handle);

void errorScreen(char* str, u32* dv, u8 n);

Result _srvpm_RegisterProcess(Handle* handleptr, u32 pid, u8* serviceaccess, u32 sasize)
{
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0] = 0x04030082; // TODO new command ID for newer firmwares!
	cmdbuf[1] = pid;
	cmdbuf[2] = sasize;
	cmdbuf[3] = (sasize << 16) | 2;
	cmdbuf[4] = (u32)serviceaccess;

	Result ret=0;
	if((ret=svcSendSyncRequest(*handleptr)))return ret;
	
	return cmdbuf[1];
}

Result FSUser_GhettoRegister(u32 pid, u64 titleid)
{
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0] = 0x086403C0;
	cmdbuf[1] = pid;
	*(u64*)&cmdbuf[2] = titleid;
	*(u16*)&cmdbuf[4] = titleid;
	cmdbuf[6] = 1; // mediatype (NAND/SD/gamecard)
	cmdbuf[7] = 0;
	cmdbuf[8] = 0; // extdata ID
	cmdbuf[9] = 0;
	cmdbuf[10] = 0; // system save data IDs
	cmdbuf[11] = 0;
	cmdbuf[12] = 0; // storage accessable unique IDs
	cmdbuf[13] = 0; 
	cmdbuf[14] = 0x00000080; // FS access info
	cmdbuf[15] = 0x01000000; // and misc. shit (not use RomFS)

	Result ret=0;
	if((ret=svcSendSyncRequest(*(Handle*)CN_FSHANDLE_ADR)))return ret;
	
	return cmdbuf[1];
}

u32 derpkerneltest()
{
	*(volatile u32*)0xFFFD6204 = 0x01FF0000;
	return 0;
}

u32 fileOffset;

int _fread(void* dst, int size, u8* file)
{
	/*u32 bytesRead;
	Result ret;
	if((ret=FSFILE_Read(file, &bytesRead, fileOffset, (u32*)dst, size))!=0)return ret;
	fileOffset+=bytesRead;
	return (bytesRead==size)?0:-1;*/
	switch (size)
	{
		case 1: *(u8*)dst = *(file+fileOffset); break;
		case 2: *(u16*)dst = *(u16*)(file+fileOffset); break;
		case 4: *(u32*)dst = *(u32*)(file+fileOffset); break;
		
		default: memcpy(dst, file+fileOffset, size); break;
	}
	
	fileOffset += size;
	
	return 0;
}

int _fseek(u8* file, u32 offset, int origin)
{
	switch(origin)
	{
		case SEEK_SET:
			fileOffset=offset;
			break;
		case SEEK_CUR:
			fileOffset+=offset;
			break;
	}
	return 0;
}

typedef struct
{
	void* segPtrs[3]; // code, rodata & data
	u32 segSizes[3];
} _3DSX_LoadInfo;

static inline void* TranslateAddr(u32 addr, _3DSX_LoadInfo* d, u32* offsets)
{
	/*if (addr < offsets[0])
		return (char*)d->segPtrs[0] + addr;
	if (addr < offsets[1])
		return (char*)d->segPtrs[1] + addr - offsets[0];
	return (char*)d->segPtrs[2] + addr - offsets[1];*/
	return (char*)0x00100000 + addr;
}

#define __CN_NEWTOTALPAGES 0x262
#define RELOCBUFSIZE 512
#define SEC_ASSERT(x) {}

int Load3DSX(u8* file)
{
	// Extra heap must be deallocated before loading a new 3DSX.
	//if(hasExtraHeap)
	//	return -5;
		
	//printf("starting to load 3DSX\n");

	u32 i, j, k, m;
	u32 endAddr = 0x00100000+__CN_NEWTOTALPAGES*0x1000;

	//SEC_ASSERT(baseAddr >= (void*)0x00100000);
	//SEC_ASSERT((((u32) baseAddr) & 0xFFF) == 0); // page alignment

	_fseek(file, 0x0, SEEK_SET);

	_3DSX_Header hdr;
	if (_fread(&hdr, sizeof(hdr), file) != 0)
		return -1;

	if (hdr.magic != _3DSX_MAGIC)
		return -2;
		
	//printf("basic checks passed\n");

	_3DSX_LoadInfo d;
	d.segSizes[0] = (hdr.codeSegSize+0xFFF) &~ 0xFFF;
	SEC_ASSERT(d.segSizes[0] >= hdr.codeSegSize); // int overflow
	d.segSizes[1] = (hdr.rodataSegSize+0xFFF) &~ 0xFFF;
	SEC_ASSERT(d.segSizes[1] >= hdr.rodataSegSize); // int overflow
	d.segSizes[2] = (hdr.dataSegSize+0xFFF) &~ 0xFFF;
	SEC_ASSERT(d.segSizes[2] >= hdr.dataSegSize); // int overflow

	// Map extra heap.
	u32 pagesRequired = d.segSizes[0]/0x1000 + d.segSizes[1]/0x1000 + d.segSizes[2]/0x1000; // XXX: int overflow
	u32 extendedPagesSize = 0;

	/*if(pagesRequired > CN_TOTAL3DSXPAGES)
	{
		if(svc_unmapProcessMemory(process, 0x00100000, 0x02000000))return -12;
		u32 extendedPages = pagesRequired - CN_TOTAL3DSXPAGES + 1;

		u32 i;
		for(i=0; i<extendedPages; i++)
		{
			if(svc_controlProcessMemory(process, endAddr+i*0x1000, heapAddr+i*0x1000, 0x1000, MEMOP_MAP, 0x7))
				return -4;
		}

		if(svc_controlProcessMemory(process, heapAddr, 0, extendedPages*0x1000, MEMOP_PROTECT, 0x1))
			return -5;

		processHandle = process;
		hasExtraHeap = 1;
		extraHeapAddr = heapAddr;
		extraHeapPages = extendedPages;

		extendedPagesSize = extraHeapPages*0x1000;
		endAddr += extendedPagesSize;
		if(svc_mapProcessMemory(process, 0x00100000, 0x02000000))return -13;
	}*/

	u32 offsets[2] = { d.segSizes[0], d.segSizes[0] + d.segSizes[1] };
	/*d.segPtrs[0] = baseAddr;
	d.segPtrs[1] = (char*)d.segPtrs[0] + d.segSizes[0];
	SEC_ASSERT((u32)d.segPtrs[1] >= d.segSizes[0]); // int overflow
	d.segPtrs[2] = (char*)d.segPtrs[1] + d.segSizes[1];
	SEC_ASSERT((u32)d.segPtrs[2] >= d.segSizes[1]); // int overflow
	SEC_ASSERT((u32)d.segPtrs[2] < endAddr);*/ // within user memory
	u8* buf = (u8*)0x15000000;//linearAlloc(pagesRequired<<12);
	d.segPtrs[0] = &buf[0];
	d.segPtrs[1] = &buf[offsets[0]];
	d.segPtrs[2] = &buf[offsets[1]];

	// Skip header for future compatibility.
	_fseek(file, hdr.headerSize, SEEK_SET);
	
	// Read the relocation headers
	SEC_ASSERT(hdr.dataSegSize >= hdr.bssSize); // int underflow
	u32* relocs = (u32*)((char*)d.segPtrs[2] + hdr.dataSegSize - hdr.bssSize);
	SEC_ASSERT((u32)relocs >= (u32)d.segPtrs[2]); // int overflow
	SEC_ASSERT((u32)relocs < endAddr); // within user memory
	u32 nRelocTables = hdr.relocHdrSize/4;
 
	u32 relocsEnd = (u32)(relocs + 3*nRelocTables);
	SEC_ASSERT((u32)relocsEnd >= (u32)relocs); // int overflow
	SEC_ASSERT((u32)relocsEnd < endAddr); // within user memory

	// XXX: Ensure enough RW pages exist at baseAddr to hold a memory block of length "totalSize".
	//    This also checks whether the memory region overflows into IPC data or loader data.
 
	for (i = 0; i < 3; i ++)
		if (_fread(&relocs[i*nRelocTables], nRelocTables*4, file) != 0)
			return -3;
 
	// Read the segments
	if (_fread(d.segPtrs[0], hdr.codeSegSize, file) != 0) return -4;
	if (_fread(d.segPtrs[1], hdr.rodataSegSize, file) != 0) return -5;
	if (_fread(d.segPtrs[2], hdr.dataSegSize - hdr.bssSize, file) != 0) return -6;
	
	//printf("shit read\n");

	// Relocate the segments
	for (i = 0; i < 3; i ++)
	{
		for (j = 0; j < nRelocTables; j ++)
		{
			u32 nRelocs = relocs[i*nRelocTables+j];
			if (j >= 2)
			{
				// We are not using this table - ignore it
				_fseek(file, nRelocs*sizeof(_3DSX_Reloc), SEEK_CUR);
				continue;
			}
 
			static _3DSX_Reloc relocTbl[RELOCBUFSIZE];
 
			u32* pos = (u32*)d.segPtrs[i];
			u32* endPos = pos + (d.segSizes[i]/4);
			SEC_ASSERT(((u32) endPos) < endAddr); // within user memory

			while (nRelocs)
			{
				u32 toDo = nRelocs > RELOCBUFSIZE ? RELOCBUFSIZE : nRelocs;
				nRelocs -= toDo;
 
				if (_fread(relocTbl, toDo*sizeof(_3DSX_Reloc), file) != 0)
					return -7;
 
				for (k = 0; k < toDo && pos < endPos; k ++)
				{
					pos += relocTbl[k].skip;
					u32 num_patches = relocTbl[k].patch;
					for (m = 0; m < num_patches && pos < endPos; m ++)
					{
						void* addr = TranslateAddr(*pos, &d, offsets);
						SEC_ASSERT(((u32) pos) < endAddr); // within user memory
						switch (j)
						{
							case 0: *pos = (u32)addr; break;
							case 1: *pos = (int)addr - (int)pos; break;
						}
						pos++;
					}
				}
			}
		}
	}
	
	//printf("shit relocated\n");

	// Detect and fill _prm structure
#if 0
	u32* prmStruct = (u32*)buf + 1;
	if(prmStruct[0]==0x6D72705F)
	{
		// Write service handle table pointer
		// the actual structure has to be filled out by cn_bootloader
		//prmStruct[1] = (u32)__service_ptr;

		// XXX: other fields that need filling:
		// prmStruct[2] <-- __apt_appid (default: 0x300)
		// prmStruct[3] <-- __heap_size (default: 24*1024*1024)
		// prmStruct[4] <-- __gsp_heap_size (default: 32*1024*1024)
		// prmStruct[5] <-- __system_arglist (default: NULL)

		//prmStruct[2] = 0x300;
		prmStruct[3] = 1*1024*1024;
		prmStruct[4] = 1*1024*1024;
		//prmStruct[5] = CN_ARGCV_LOC;
		//prmStruct[6] = RUNFLAG_APTWORKAROUND; //__system_runflags

		// XXX: Notes on __system_arglist:
		//     Contains a pointer to a u32 specifying the number of arguments immediately followed
		//     by the NULL-terminated argument strings themselves (no pointers). The first argument
		//     should always be the path to the file we are booting. Example:
		//     \x02\x00\x00\x00sd:/dir/file.3dsx\x00Argument1\x00
		//     Above corresponds to { "sd:/dir/file.3dsx", "Argument1" }.
	}
#endif
	
	u32 derp;
	Result res;
	
	
	/*res = svcControlMemory(&derp, 0x093FF000, 0, 0x1000, 0x303, 3);
	if (res)
	{
		dbgcolor(0x10000FF);
		return res;
	}*/
	
	
	//dbgcolor(0x100FF00);
	Handle codeset;
	u8 codesetinfo[64];
	
	strncpy((char*)&codesetinfo[0x00], "blarg", 6);
	*(u32*)&codesetinfo[0x08] = 0;
	*(u32*)&codesetinfo[0x0C] = 0;
	*(u32*)&codesetinfo[0x10] = 0x00100000; // .text addr
	*(u32*)&codesetinfo[0x14] = d.segSizes[0]>>12;
	*(u32*)&codesetinfo[0x18] = 0x00100000+d.segSizes[0];
	*(u32*)&codesetinfo[0x1C] = d.segSizes[1]>>12;
	*(u32*)&codesetinfo[0x20] = 0x00100000+d.segSizes[0]+d.segSizes[1];
	*(u32*)&codesetinfo[0x24] = d.segSizes[2]>>12;
	*(u32*)&codesetinfo[0x28] = d.segSizes[0]>>12;
	*(u32*)&codesetinfo[0x2C] = d.segSizes[1]>>12;
	*(u32*)&codesetinfo[0x30] = d.segSizes[2]>>12;
	*(u32*)&codesetinfo[0x34] = 0;
	*(u32*)&codesetinfo[0x38] = 0x0BADC0DE;
	*(u32*)&codesetinfo[0x3C] = 0x00040130;
	//*(u32*)&codesetinfo[0x38] = 0;
	//*(u32*)&codesetinfo[0x3C] = 0;
	
	
	res = svcCreateCodeSet(&codeset, codesetinfo, d.segPtrs[0], d.segPtrs[1], d.segPtrs[2]);
	if (res) return res;
	//dbgcolor(0x100FFFF);
	
	Handle process;
	u32 arm11caps[] = 
	{
		// grant all SVCs
		0xF0FFFFFE,
		0xF1FFFFFF,
		0xF2FFFFFF,
		0xF3FFFFFF,
		0xF4FFFFFF,
		0xF500003F,
		// stolen from CN -- dunno
		0xFF81FF50,
		0xFF81FF58,
		0xFF81FF70,
		0xFF81FF78,
		0xFF91F000,
		0xFF91F600,
		//0xFF000101, // other capabilities (bit 8-11: mem type)
		//0xFF000280,
		0xFF000380, // other capabilities (bit 8-11: mem type)
		0xFE000200, // handle table size
	};
	
	res = svcCreateProcess(&process, codeset, arm11caps, 14);
	if (res) return res;
	//dbgcolor(0x1FF0000);
	
	svcCloseHandle(codeset);
	
	
	Handle rsrclimit;
	u32 rsrctypes[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	s64 rsrcvalues[] = 
	{
		0x18, // max thread prio
		0x04000000,//0x04000000, // max allocatable memory
		0x20, // max threads
		0x20, // max events
		0x20, // max mutexes
		0x8, // max semaphores
		0x8, // max timers
		0x10, // max shared memory (pages??)
		0x2, // max address arbiters
		1000 // max syscore time
	};
	
	res = svcCreateResourceLimit(&rsrclimit);
	if (res) return res;
	//dbgcolor(0x10000FF);
	
	res = svcSetResourceLimitValues(rsrclimit, rsrctypes, rsrcvalues, 10);
	if (res) return res;
	//dbgcolor(0x1FF00FF);
	
	res = svcSetProcessResourceLimits(process, rsrclimit);
	if (res) return res;
	//dbgcolor(0x1000080);
	
	
	u8 affmask = 0x3;
	//u8 affmask = 0x1;
	res = svcSetProcessAffinityMask(process, &affmask, 2);
	if (res) return res;
	//dbgcolor(0x1008080);
	
	res = svcSetProcessIdealProcessor(process, 1);
	//res = svcSetProcessIdealProcessor(process, 0);
	if (res) return res;
	//dbgcolor(0x1800080);

	
	
	// SRV reg
	u32 pid;
	svcGetProcessId(&pid, process);
	
	Handle* srv = (Handle*)CN_SRVHANDLE_ADR;
	Handle srvpm;
	//res = _srv_getServiceHandle(srv, &srvpm, "srv:pm");
	res = svcConnectToPort(&srvpm, "srv:pm");
	if (res) return res;
	//dbgcolor(0x1808000);
	
	u8 serviceaccess[0x100] = {0};
	u32 sasize = 0;
	
#define ADDSERVICE(n) strncpy((char*)&serviceaccess[sasize], n, 8); sasize += 8;
	ADDSERVICE("fs:USER");
	
	res = _srvpm_RegisterProcess(&srvpm, pid, serviceaccess, sasize);
	if (res) return res;
	//dbgcolor(0x10080FF);
	
	
	/*Handle fsreg;
	res = _srv_getServiceHandle(srv, &fsreg, "fs:REG");
	dbgcolor(0x1FF00FF);
	if (res) return res;
	dbgcolor(0x180FF80);
	
	res = _FSReg_Register(&fsreg, pid, 0x000400000BADC0DE);
	if (res) return res;
	dbgcolor(0x1FF80FF);*/
	
	
	//Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;
	//_GSPGPU_ReleaseRight(*gspHandle);

	
	/*for (i = 0; i < 0x02000000; i += 0x1000)
	{
		res = svcControlMemory(&derp, 0x14000000+i, 0, 0x1000, MEMOP_FREE, (MemPerm)0);
		//if (res) return res;
	}*/
	
	/*u32 dorp[5*3] = {0}; u32 crapo;
	svcQueryProcessMemory(&dorp[0], &dorp[4], process, 0x00100000);
	svcQueryProcessMemory(&dorp[5], &dorp[9], process, 0x00100000+d.segSizes[0]);
	svcQueryProcessMemory(&dorp[10], &dorp[14], process, 0x00100000+d.segSizes[0]+d.segSizes[1]);
	svcControlMemory(&crapo, 0, 0, 0x01000000, MEMOP_ALLOC_LINEAR, (MemPerm)3);
	errorScreen("meminfo", dorp, 15);*/
	
	
	/*res = svcControlProcessMemory(process, *(u32*)&codesetinfo[0x10], d.segPtrs[0], (*(u32*)&codesetinfo[0x14])<<12, MEMOP_MAP, MEMPERM_READ|MEMPERM_EXECUTE);
	if (res) return res;
	res = svcControlProcessMemory(process, *(u32*)&codesetinfo[0x18], d.segPtrs[1], (*(u32*)&codesetinfo[0x1C])<<12, MEMOP_MAP, MEMPERM_READ);
	if (res) return res;
	res = svcControlProcessMemory(process, *(u32*)&codesetinfo[0x20], d.segPtrs[2], (*(u32*)&codesetinfo[0x24])<<12, MEMOP_MAP, MEMPERM_READ|MEMPERM_WRITE);
	if (res) return res;*/
	
	/*res = svcKernelSetState(6, 3, 0, 0);
	if (res)
	{
		dbgcolor(0x10000FF);
		return res;
	}*/
	
	//dbgcolor(0x1FFFF00);
	
	u32 startupinfo[5] = 
	{
		//0x30, 		// prio
		0x34,		// prio
		0x4000, 	// stack size
		0,			// argc
		NULL,		// argv
		NULL		// envp
	};
	
	//svcExitProcess();
	res = svcRun(process, startupinfo);
	if (res)
	{
		dbgcolor(0x1404040);
		//*(u32*)0 = 12345678;
		return res;
	}
 dbgcolor(0x1400040);
 
	svcCloseHandle(process);
	svcCloseHandle(rsrclimit);
 
 
	
	
	/*Handle dorp;
	res = svcOpenProcess(&dorp, pid);
	if (res)
	{
		dbgcolor(0x10000FF);
		return res;
	}
	
	Handle dbg;
	res = svcDebugActiveProcess(&dbg, pid);
	if (res)
	{
		dbgcolor(0x100FFFF);
		return res;
	}
	
	u32 buffer[0x100];
	res = svcReadProcessMemory(buffer, dbg, 0x00100000, 0x100<<2);
	if (res)
	{
		dbgcolor(0x100FF00);
		return res;
	}
	
	
	u32 crapo;
	svcControlMemory(&crapo, 0, 0, 0x01000000, MEMOP_ALLOC_LINEAR, (MemPerm)3);
	errorScreen("first words", buffer, 0x20);*/
 
 
        //svc_closeHandle(process); TODO
        //svc_closeHandle(file);
	return 0; // Success.
}




// KILL ME KILL ME

int Load3DSX_2(u8* file)
{
	// Extra heap must be deallocated before loading a new 3DSX.
	//if(hasExtraHeap)
	//	return -5;
		
	//printf("starting to load 3DSX\n");

	u32 i, j, k, m;
	u32 derp;
	u32 endAddr = 0x00100000+__CN_NEWTOTALPAGES*0x1000;

	//SEC_ASSERT(baseAddr >= (void*)0x00100000);
	//SEC_ASSERT((((u32) baseAddr) & 0xFFF) == 0); // page alignment
 dbgcolor(0x01FFFF00);
	_fseek(file, 0x0, SEEK_SET);
 dbgcolor(0x01FF00FF);
	_3DSX_Header hdr;
	if (_fread(&hdr, sizeof(hdr), file) != 0)
		return -1;

	if (hdr.magic != _3DSX_MAGIC)
		return -2;
		
	//printf("basic checks passed\n");

	_3DSX_LoadInfo d;
	d.segSizes[0] = (hdr.codeSegSize+0xFFF) &~ 0xFFF;
	SEC_ASSERT(d.segSizes[0] >= hdr.codeSegSize); // int overflow
	d.segSizes[1] = (hdr.rodataSegSize+0xFFF) &~ 0xFFF;
	SEC_ASSERT(d.segSizes[1] >= hdr.rodataSegSize); // int overflow
	d.segSizes[2] = (hdr.dataSegSize+0xFFF) &~ 0xFFF;
	SEC_ASSERT(d.segSizes[2] >= hdr.dataSegSize); // int overflow

	// Map extra heap.
	u32 pagesRequired = d.segSizes[0]/0x1000 + d.segSizes[1]/0x1000 + d.segSizes[2]/0x1000; // XXX: int overflow
	u32 extendedPagesSize = 0;

	/*if(pagesRequired > CN_TOTAL3DSXPAGES)
	{
		if(svc_unmapProcessMemory(process, 0x00100000, 0x02000000))return -12;
		u32 extendedPages = pagesRequired - CN_TOTAL3DSXPAGES + 1;

		u32 i;
		for(i=0; i<extendedPages; i++)
		{
			if(svc_controlProcessMemory(process, endAddr+i*0x1000, heapAddr+i*0x1000, 0x1000, MEMOP_MAP, 0x7))
				return -4;
		}

		if(svc_controlProcessMemory(process, heapAddr, 0, extendedPages*0x1000, MEMOP_PROTECT, 0x1))
			return -5;

		processHandle = process;
		hasExtraHeap = 1;
		extraHeapAddr = heapAddr;
		extraHeapPages = extendedPages;

		extendedPagesSize = extraHeapPages*0x1000;
		endAddr += extendedPagesSize;
		if(svc_mapProcessMemory(process, 0x00100000, 0x02000000))return -13;
	}*/
	
	// allocate memory for the process's pages
	// Better only allocate the needed amount. Allocating too much results in a suboptimal memory layout
	// which may cause further heap allocs to fail
	svcControlMemory(&derp, 0x08000000, 0, pagesRequired<<12, MEMOP_ALLOC, (MemPerm)0x3);

	u32 offsets[2] = { d.segSizes[0], d.segSizes[0] + d.segSizes[1] };
	/*d.segPtrs[0] = baseAddr;
	d.segPtrs[1] = (char*)d.segPtrs[0] + d.segSizes[0];
	SEC_ASSERT((u32)d.segPtrs[1] >= d.segSizes[0]); // int overflow
	d.segPtrs[2] = (char*)d.segPtrs[1] + d.segSizes[1];
	SEC_ASSERT((u32)d.segPtrs[2] >= d.segSizes[1]); // int overflow
	SEC_ASSERT((u32)d.segPtrs[2] < endAddr);*/ // within user memory
	u8* buf = (u8*)0x08000000;//linearAlloc(pagesRequired<<12);
	d.segPtrs[0] = &buf[0];
	d.segPtrs[1] = &buf[offsets[0]];
	d.segPtrs[2] = &buf[offsets[1]];
 dbgcolor(0x01FFC0FF);
	// Skip header for future compatibility.
	_fseek(file, hdr.headerSize, SEEK_SET);
	
	// Read the relocation headers
	SEC_ASSERT(hdr.dataSegSize >= hdr.bssSize); // int underflow
	u32* relocs = (u32*)((char*)d.segPtrs[2] + hdr.dataSegSize - hdr.bssSize);
	SEC_ASSERT((u32)relocs >= (u32)d.segPtrs[2]); // int overflow
	SEC_ASSERT((u32)relocs < endAddr); // within user memory
	u32 nRelocTables = hdr.relocHdrSize/4;
 
	u32 relocsEnd = (u32)(relocs + 3*nRelocTables);
	SEC_ASSERT((u32)relocsEnd >= (u32)relocs); // int overflow
	SEC_ASSERT((u32)relocsEnd < endAddr); // within user memory

	// XXX: Ensure enough RW pages exist at baseAddr to hold a memory block of length "totalSize".
	//    This also checks whether the memory region overflows into IPC data or loader data.
 dbgcolor(0x010000FF);
	for (i = 0; i < 3; i ++)
		if (_fread(&relocs[i*nRelocTables], nRelocTables*4, file) != 0)
			return -3;
 dbgcolor(0x0100FFFF);
	// Read the segments
	if (_fread(d.segPtrs[0], hdr.codeSegSize, file) != 0) return -4;
	 dbgcolor(0x01FF0000);
	if (_fread(d.segPtrs[1], hdr.rodataSegSize, file) != 0) return -5;
	dbgcolor(0x0100FF00);
	if (_fread(d.segPtrs[2], hdr.dataSegSize - hdr.bssSize, file) != 0) return -6;
	
	//printf("shit read\n");
dbgcolor(0x01FFFF00);
	// Relocate the segments
	for (i = 0; i < 3; i ++)
	{
		for (j = 0; j < nRelocTables; j ++)
		{
			u32 nRelocs = relocs[i*nRelocTables+j];
			if (j >= 2)
			{
				// We are not using this table - ignore it
				_fseek(file, nRelocs*sizeof(_3DSX_Reloc), SEEK_CUR);
				continue;
			}
 
			static _3DSX_Reloc relocTbl[RELOCBUFSIZE];
 
			u32* pos = (u32*)d.segPtrs[i];
			u32* endPos = pos + (d.segSizes[i]/4);
			SEC_ASSERT(((u32) endPos) < endAddr); // within user memory

			while (nRelocs)
			{
				u32 toDo = nRelocs > RELOCBUFSIZE ? RELOCBUFSIZE : nRelocs;
				nRelocs -= toDo;
 
				if (_fread(relocTbl, toDo*sizeof(_3DSX_Reloc), file) != 0)
					return -7;
 
				for (k = 0; k < toDo && pos < endPos; k ++)
				{
					pos += relocTbl[k].skip;
					u32 num_patches = relocTbl[k].patch;
					for (m = 0; m < num_patches && pos < endPos; m ++)
					{
						void* addr = TranslateAddr(*pos, &d, offsets);
						SEC_ASSERT(((u32) pos) < endAddr); // within user memory
						switch (j)
						{
							case 0: *pos = (u32)addr; break;
							case 1: *pos = (int)addr - (int)pos; break;
						}
						pos++;
					}
				}
			}
		}
	}
	
	//printf("shit relocated\n");
dbgcolor(0x01000040);
	// Detect and fill _prm structure
	u32* prmStruct = (u32*)buf + 1;
	if(prmStruct[0]==0x6D72705F)
	{
		// Write service handle table pointer
		// the actual structure has to be filled out by cn_bootloader
		//prmStruct[1] = (u32)__service_ptr;

		// XXX: other fields that need filling:
		// prmStruct[2] <-- __apt_appid (default: 0x300)
		// prmStruct[3] <-- __heap_size (default: 24*1024*1024)
		// prmStruct[4] <-- __gsp_heap_size (default: 32*1024*1024)
		// prmStruct[5] <-- __system_arglist (default: NULL)

		prmStruct[2] = 0x300;
		prmStruct[3] = 24*1024*1024;
		prmStruct[4] = 32*1024*1024;
		//prmStruct[5] = CN_ARGCV_LOC;
		prmStruct[6] = 2;//RUNFLAG_APTWORKAROUND; //__system_runflags

		// XXX: Notes on __system_arglist:
		//     Contains a pointer to a u32 specifying the number of arguments immediately followed
		//     by the NULL-terminated argument strings themselves (no pointers). The first argument
		//     should always be the path to the file we are booting. Example:
		//     \x02\x00\x00\x00sd:/dir/file.3dsx\x00Argument1\x00
		//     Above corresponds to { "sd:/dir/file.3dsx", "Argument1" }.
	}
	
	
	Result res;
	
	dbgcolor(0x1004000);
	Handle codeset;
	u8 codesetinfo[64];
	
	memcpy(&codesetinfo[0x00], "Wart\0\0\0\0", 8);
	*(u32*)&codesetinfo[0x08] = 0;
	*(u32*)&codesetinfo[0x0C] = 0;
	*(u32*)&codesetinfo[0x10] = 0x00100000; // .text addr
	*(u32*)&codesetinfo[0x14] = d.segSizes[0]>>12;
	*(u32*)&codesetinfo[0x18] = 0x00100000+d.segSizes[0];
	*(u32*)&codesetinfo[0x1C] = d.segSizes[1]>>12;
	*(u32*)&codesetinfo[0x20] = 0x00100000+d.segSizes[0]+d.segSizes[1];
	*(u32*)&codesetinfo[0x24] = d.segSizes[2]>>12;
	*(u32*)&codesetinfo[0x28] = d.segSizes[0]>>12;
	*(u32*)&codesetinfo[0x2C] = d.segSizes[1]>>12;
	*(u32*)&codesetinfo[0x30] = d.segSizes[2]>>12;
	*(u32*)&codesetinfo[0x34] = 0;
	*(u32*)&codesetinfo[0x38] = 0x12345678; // 0004B300
	*(u32*)&codesetinfo[0x3C] = 0x00040000;
	
	res = svcCreateCodeSet(&codeset, codesetinfo, d.segPtrs[0], d.segPtrs[1], d.segPtrs[2]);
	if (res) return res;
	dbgcolor(0x100FFFF);
	
	Handle process;
	u32 arm11caps[] = 
	{
		// grant all SVCs
		0xF0FFFFFE,
		0xF1FFFFFF,
		0xF2FFFFFF,
		0xF3FFFFFF,
		0xF4FFFFFF,
		0xF500003F,
		// extra shit
		0xFF81FF50,
		0xFF81FF58,
		0xFF81FF70,
		0xFF81FF78,
		0xFF91F000,
		0xFF91F600,
		0xFF000101, // other capabilities (bit 8-11: mem type)
		0xFE000200, // handle table size
	};
	
	res = svcCreateProcess(&process, codeset, arm11caps, 14);
	if (res) return res;
	dbgcolor(0x1400000);
	
	svcCloseHandle(codeset);
	
	
	// deallocate memory
	/*for (i = 0; i < 0x02000000; i += 0x1000)
	{
		res = svcControlMemory(&derp, 0x14000000+i, 0, 0x1000, MEMOP_FREE, (MemPerm)0);
		//if (res) return res;
	}*/
	
	svcControlMemory(&derp, 0x14000000, 0, 0x01000000, MEMOP_FREE, (MemPerm)0);
	//svcControlMemory(&derp, 0x08000000, 0, 0x02000000, MEMOP_FREE, (MemPerm)0);
	svcControlMemory(&derp, 0x08000000, 0, pagesRequired<<12, MEMOP_FREE, (MemPerm)0);
	
	
	/*u32 darp[10] = {0};
	svcQueryMemory((MemInfo*)&darp[0], (PageInfo*)&darp[4], 0x14000000);
	svcQueryMemory((MemInfo*)&darp[5], (PageInfo*)&darp[9], 0x08000000);
	u32 crapo;svcControlMemory(&crapo, 0, 0, 0x01000000, MEMOP_ALLOC_LINEAR, (MemPerm)3);
	errorScreen("heap", darp, 10);*/
	
	
	Handle rsrclimit;
	u32 rsrctypes[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	s64 rsrcvalues[] = 
	{
		0x18, // max thread prio
		0x04000000, // max allocatable memory
		0x20, // max threads
		0x20, // max events
		0x20, // max mutexes
		0x8, // max semaphores
		0x8, // max timers
		0x10, // max shared memory (pages??)
		0x2, // max address arbiters
		0 // max syscore time
	};
	
	res = svcCreateResourceLimit(&rsrclimit);
	if (res) return res;
	dbgcolor(0x10000FF);
	
	res = svcSetResourceLimitValues(rsrclimit, rsrctypes, rsrcvalues, 10);
	if (res) return res;
	//dbgcolor(0x1FF00FF);
	
	res = svcSetProcessResourceLimits(process, rsrclimit);
	if (res) return res;
	dbgcolor(0x1000080);
	
	
	//u8 affmask = 0x3;
	u8 affmask = 0x1;
	res = svcSetProcessAffinityMask(process, &affmask, 2);
	if (res) return res;
	dbgcolor(0x1008080);
	
	//res = svcSetProcessIdealProcessor(process, 1);
	res = svcSetProcessIdealProcessor(process, 0);
	if (res) return res;
	dbgcolor(0x1800080);

	
	
	// SRV reg
	u32 pid;
	svcGetProcessId(&pid, process);
	
	Handle* srv = (Handle*)CN_SRVHANDLE_ADR;
	Handle srvpm;
	//res = _srv_getServiceHandle(srv, &srvpm, "srv:pm");
	res = svcConnectToPort(&srvpm, "srv:pm");
	if (res) return res;
	dbgcolor(0x1808000);
	
	u8 serviceaccess[0x100] = {0};
	u32 sasize = 0;
	
#define ADDSERVICE(n) strncpy((char*)&serviceaccess[sasize], n, 8); sasize += 8;
	ADDSERVICE("APT:U");
	ADDSERVICE("fs:USER");
	ADDSERVICE("gsp::Gpu");
	ADDSERVICE("hid:USER");
	ADDSERVICE("csnd:SND");
	ADDSERVICE("dsp::DSP");
	ADDSERVICE("http:C");
	ADDSERVICE("mic:u");
	
	res = _srvpm_RegisterProcess(&srvpm, pid, serviceaccess, sasize);
	if (res) return res;
	dbgcolor(0x10080FF);
	
	
	// FS reg
	res = FSUser_GhettoRegister(pid, 0x0004000012345678);
	if (res) return res;
	dbgcolor(0x1FF80FF);
	
	
	//Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;
	//_GSPGPU_ReleaseRight(*gspHandle);
	
	dbgcolor(0x1FFFF00);
	
	u32 startupinfo[5] = 
	{
		0x30,		// prio
		0x4000, 	// stack size
		0,			// argc
		NULL,		// argv
		NULL		// envp
	};
	
	//svcExitProcess();
	res = svcRun(process, startupinfo);
	if (res)
	{
		dbgcolor(0x1404040);
		//*(u32*)0 = 12345678;
		return res;
	}
 dbgcolor(0x1FFFFFF);
 
	svcCloseHandle(process);
	svcCloseHandle(rsrclimit);
 
	return 0; // Success.
}


