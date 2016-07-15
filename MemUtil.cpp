#include "MemUtil.h"

#include "skse/SafeWrite.h"

void __stdcall MemUtil::WriteRedirectionHook(IntPtr targetOfHook, IntPtr sourceBegin, IntPtr sourceEnd, UInt32 asmSegSize) {
	WriteRelJump(targetOfHook, sourceBegin);

	for (int i = 5; i < asmSegSize; ++i) {
		SafeWrite8(targetOfHook + i, 0x90); // nop
	}

	WriteRelJump(sourceEnd, targetOfHook + asmSegSize);
}
MemUtil::IntPtr __stdcall MemUtil::WriteVTableHook(IntPtr vtable, UInt32 fn, IntPtr fnPtr) {
	IntPtr originalFunction = vtable[fn];// GetVTableFunction(vtable, fn);

	SafeWrite32(vtable + fn * sizeof(IntPtr), fnPtr);

	//_MESSAGE("before\tvtable[fn]: %0x.8x", originalFunction);
	//_MESSAGE("after\t[fn]: %0x.8x", vtable[fn]);

	return originalFunction;
}

void __stdcall MemUtil::WriteOpCode8(IntPtr target, OpCode8 op) {
	SafeWrite8(target, op);
}
void __stdcall MemUtil::WriteOpCode16(IntPtr target, OpCode16 op) {
	SafeWrite16(target, op);
}
void __stdcall MemUtil::WriteOpCode24(IntPtr target, OpCode24 op) {
	SafeWriteBuf(target, reinterpret_cast<void*>(&op), 3);
}
void __stdcall MemUtil::WriteOpCode8WithImmediate(IntPtr target, OpCode8 op, IntPtr var) {
	SafeWrite8(target, op);
	SafeWrite32(target + 0x1, var);
}
void __stdcall MemUtil::WriteOpCode16WithImmediate(IntPtr target, OpCode16 op, IntPtr var) {
	SafeWrite16(target, op);
	SafeWrite32(target + 0x2, var);
}
void __stdcall MemUtil::WriteOpCode24WithImmediate(IntPtr target, OpCode24 op, IntPtr var) {
	SafeWriteBuf(target, reinterpret_cast<void*>(&op), 3);
	SafeWrite32(target + 0x3, var);
}
