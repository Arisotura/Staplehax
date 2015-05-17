.arm
	
.global GetMagicalPointer
GetMagicalPointer:
	stmdb sp!, {r4-r5}
	mov r0, #0x14000000
	svc 0x2
	bic r0, r12, #0xFF0
	bic r0, r0, #0xF
	ldmia sp!, {r4-r5}
	bx lr
	
.global svcCreateCodeSet
svcCreateCodeSet:
	str r0, [sp, #-4]!
	ldr r0, [sp, #4]
	svc 0x73
	ldr r2, [sp], #4
	str r1, [r2]
	bx  lr
	
.global svcCreateResourceLimit
svcCreateResourceLimit:
	str r0, [sp, #-4]!
	svc 0x78
	ldr r2, [sp], #4
	str r1, [r2]
	bx lr
	
.global svcSetResourceLimitValues
svcSetResourceLimitValues:
	svc 0x79
	bx lr
	
.global svcCreateProcess
svcCreateProcess:
	str r0, [sp, #-4]!
	svc 0x75
	ldr r2, [sp], #4
	str r1, [r2]
	bx lr
	
.global svcSetProcessResourceLimits
svcSetProcessResourceLimits:
	svc 0x77
	bx lr
	
.global svcSetProcessAffinityMask
svcSetProcessAffinityMask:
	svc 0x5
	bx lr
	
.global svcSetProcessIdealProcessor
svcSetProcessIdealProcessor:
	svc 0x7
	bx lr
	
.global svcRun
svcRun:
	stmdb sp!, {r4,r5}
	ldr r5, [r1, #0x10]
	ldr r4, [r1, #0xC]
	ldr r3, [r1, #0x8]
	ldr r2, [r1, #0x4]
	ldr r1, [r1]
	svc 0x12
	ldmia sp!, {r4, r5}
	bx lr
	
.global svcGetResourceLimit
svcGetResourceLimit:
	str r0, [sp, #-4]!
	svc 0x38
	ldr r2, [sp], #4
	str r1, [r2]
	bx lr
	
.global svcGetResourceLimitLimitValues
svcGetResourceLimitLimitValues:
	@ *values, rsrclimit, *names, namecount
	svc 0x39
	bx lr
	
.global svcKernelSetState
svcKernelSetState:
	svc 0x7C
	bx lr
	
.global KernelFlushCaches
KernelFlushCaches:
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0
	mov r0, #0
	mcr p15, 0, r0, c7, c10, 0
	bx lr
	
	
	
.global FSPatchStart
.global FSPatchEnd

@ R4 = cmdbuf
FSPatchStart:
	add sp, sp, #0x40
	ldmia sp!, {r12, r5-r10}
	stmdb sp!, {r12, r5-r11}
	sub sp, sp, #0x3C
	.word 0x77770001
FSPatchEnd:
