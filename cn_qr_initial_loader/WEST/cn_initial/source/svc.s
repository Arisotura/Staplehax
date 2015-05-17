.arm
.align 4


.global svcSleepThread
.type svcSleepThread, %function
svcSleepThread:
	svc 0x0A
	bx  lr

.global svcReleaseSemaphore
.type svcReleaseSemaphore, %function
svcReleaseSemaphore:
	push {r0}
	svc  0x16
	pop  {r3}
	str  r1, [r3]
	bx   lr

.global svcSignalEvent
.type svcSignalEvent, %function
svcSignalEvent:
	svc 0x18
	bx  lr

.global svcArbitrateAddress
.type svcArbitrateAddress, %function
svcArbitrateAddress:
	push {r4, r5}
	add sp, #8
	ldr r5, [sp]
	ldr r4, [sp, #4]
	sub sp, #8
	svc 0x22
	pop {r4, r5}
	bx  lr

.global svcSendSyncRequest
.type svcSendSyncRequest, %function
svcSendSyncRequest:
	svc 0x32
	bx lr
