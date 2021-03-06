#include "stdafx.h"

#include "../../source/Applewin.h"
#include "../../source/CPU.h"

// From Applewin.cpp
eCPU		g_ActiveCPU = CPU_6502;
enum AppMode_e g_nAppMode = MODE_RUNNING;

// From Memory.cpp
LPBYTE         memwrite[0x100];		// TODO: Init
LPBYTE         mem          = NULL;	// TODO: Init
LPBYTE         memdirty     = NULL;	// TODO: Init
iofunction		IORead[256] = {0};	// TODO: Init
iofunction		IOWrite[256] = {0};	// TODO: Init

// From Debugger_Types.h
	enum AddressingMode_e // ADDRESSING_MODES_e
	{
		  AM_IMPLIED // Note: SetDebugBreakOnInvalid() assumes this order of first 4 entries
		, AM_1    //    Invalid 1 Byte
		, AM_2    //    Invalid 2 Bytes
		, AM_3    //    Invalid 3 Bytes
	};

// From CPU.cpp
#define	 AF_SIGN       0x80
#define	 AF_OVERFLOW   0x40
#define	 AF_RESERVED   0x20
#define	 AF_BREAK      0x10
#define	 AF_DECIMAL    0x08
#define	 AF_INTERRUPT  0x04
#define	 AF_ZERO       0x02
#define	 AF_CARRY      0x01

regsrec regs;

static const int IRQ_CHECK_TIMEOUT = 128;
static signed int g_nIrqCheckTimeout = IRQ_CHECK_TIMEOUT;

static __forceinline int Fetch(BYTE& iOpcode, ULONG uExecutedCycles)
{
	iOpcode = *(mem+regs.pc);
	regs.pc++;
	return 1;
}

#define INV IsDebugBreakOnInvalid(AM_1);
inline int IsDebugBreakOnInvalid( int iOpcodeType )
{
	return 0;
}

static __forceinline void DoIrqProfiling(DWORD uCycles)
{
}

static __forceinline void CheckInterruptSources(ULONG uExecutedCycles)
{
}

static __forceinline void NMI(ULONG& uExecutedCycles, UINT& uExtraCycles, BOOL& flagc, BOOL& flagn, BOOL& flagv, BOOL& flagz)
{
}

static __forceinline void IRQ(ULONG& uExecutedCycles, UINT& uExtraCycles, BOOL& flagc, BOOL& flagn, BOOL& flagv, BOOL& flagz)
{
}

void RequestDebugger()
{
}

// From Debug.h
inline int IsDebugBreakpointHit()
{
	return 0;
}

// From Debug.cpp
int g_bDebugBreakpointHit = 0;

// From z80.cpp
DWORD z80_mainloop(ULONG uTotalCycles, ULONG uExecutedCycles)
{
	return 0;
}

//-------------------------------------

#include "../../source/cpu/cpu_general.inl"
#include "../../source/cpu/cpu_instructions.inl"
#include "../../source/cpu/cpu6502.h"  // MOS 6502
#include "../../source/cpu/cpu65C02.h"  // WDC 65C02

void init(void)
{
	mem = (LPBYTE)VirtualAlloc(NULL,64*1024,MEM_COMMIT,PAGE_READWRITE);

	for (UINT i=0; i<256; i++)
		memwrite[i] = mem+i*256;

	memdirty = new BYTE[256];
}

void reset(void)
{
	regs.a  = 0;
	regs.x  = 0;
	regs.y  = 0;
	regs.pc = 0x300;
	regs.sp = 0x1FF;
	regs.ps = 0;
	regs.bJammed = 0;
}

//-------------------------------------

int GH264_test(void)
{
	// No page-cross
	reset();
	regs.pc = 0x300;
	WORD abs = regs.pc+3;
	WORD dst = abs+2;
	mem[regs.pc+0] = 0x6c;	// JMP (IND) 
	mem[regs.pc+1] = abs&0xff;
	mem[regs.pc+2] = abs>>8;
	mem[regs.pc+3] = dst&0xff;
	mem[regs.pc+4] = dst>>8;

	DWORD cycles = Cpu6502(0);
	if (cycles != 5) return 1;
	if (regs.pc != dst) return 1;

	reset();
	cycles = Cpu65C02(0);
	if (cycles != 6) return 1;
	if (regs.pc != dst) return 1;

	// Page-cross
	reset();
	regs.pc = 0x3fc;		// 3FC: JMP (abs)
	abs = regs.pc+3;		// 3FF: lo(dst), hi(dst)
	dst = abs+2;
	mem[regs.pc+0] = 0x6c;	// JMP (IND)
	mem[regs.pc+1] = abs&0xff;
	mem[regs.pc+2] = abs>>8;
	mem[regs.pc+3] = dst&0xff;
	mem[regs.pc+4] = mem[regs.pc & ~0xff] = dst>>8;	// Allow for bug in 6502

	cycles = Cpu6502(0);
	if (cycles != 5) return 1;
	if (regs.pc != dst) return 1;

	reset();
	regs.pc = 0x3fc;
	mem[regs.pc & ~0xff] = 0;	// Test that 65C02 fixes the bug in the 6502
	cycles = Cpu65C02(0);
	if (cycles != 7) return 1;	// todo: is this 6 or 7?
	if (regs.pc != dst) return 1;

	return 0;
}

//-------------------------------------

void ASL_ABSX(BYTE x, WORD base, BYTE d)
{
	WORD addr = base+x;
	mem[addr] = d;

	reset();
	regs.x = x;
	mem[regs.pc+0] = 0x1e;
	mem[regs.pc+1] = base&0xff;
	mem[regs.pc+2] = base>>8;
}

void DEC_ABSX(BYTE x, WORD base, BYTE d)
{
	WORD addr = base+x;
	mem[addr] = d;

	reset();
	regs.x = x;
	mem[regs.pc+0] = 0xde;
	mem[regs.pc+1] = base&0xff;
	mem[regs.pc+2] = base>>8;
}

void INC_ABSX(BYTE x, WORD base, BYTE d)
{
	WORD addr = base+x;
	mem[addr] = d;

	reset();
	regs.x = x;
	mem[regs.pc+0] = 0xfe;
	mem[regs.pc+1] = base&0xff;
	mem[regs.pc+2] = base>>8;
}

int GH271_test(void)
{
	// asl abs,x
	{
		const WORD base = 0x20ff;
		const BYTE d = 0x40;

		// no page-cross
		{
			const BYTE x = 0;

			ASL_ABSX(x, base, d);
			if (Cpu6502(0) != 7) return 1;
			if (mem[base+x] != ((d<<1)&0xff)) return 1;

			ASL_ABSX(x, base, d);
			if (Cpu65C02(0) != 6) return 1;	// Non-PX case is optimised on 65C02
			if (mem[base+x] != ((d<<1)&0xff)) return 1;
		}

		// page-cross
		{
			const BYTE x = 1;

			ASL_ABSX(x, base, d);
			if (Cpu6502(0) != 7) return 1;
			if (mem[base+x] != ((d<<1)&0xff)) return 1;

			ASL_ABSX(x, base, d);
			if (Cpu65C02(0) != 7) return 1;
			if (mem[base+x] != ((d<<1)&0xff)) return 1;
		}
	}

	// dec abs,x
	{
		const WORD base = 0x20ff;
		const BYTE d = 0x40;

		// no page-cross
		{
			const BYTE x = 0;

			DEC_ABSX(x, base, d);
			if (Cpu6502(0) != 7) return 1;
			if (mem[base+x] != ((d-1)&0xff)) return 1;

			DEC_ABSX(x, base, d);
			if (Cpu65C02(0) != 7) return 1;	// NB. Not optimised for 65C02
			if (mem[base+x] != ((d-1)&0xff)) return 1;
		}

		// page-cross
		{
			const BYTE x = 1;

			DEC_ABSX(x, base, d);
			if (Cpu6502(0) != 7) return 1;
			if (mem[base+x] != ((d-1)&0xff)) return 1;

			DEC_ABSX(x, base, d);
			if (Cpu65C02(0) != 7) return 1;
			if (mem[base+x] != ((d-1)&0xff)) return 1;
		}
	}

	// inc abs,x
	{
		const WORD base = 0x20ff;
		const BYTE d = 0x40;

		// no page-cross
		{
			const BYTE x = 0;

			INC_ABSX(x, base, d);
			if (Cpu6502(0) != 7) return 1;
			if (mem[base+x] != ((d+1)&0xff)) return 1;

			INC_ABSX(x, base, d);
			if (Cpu65C02(0) != 7) return 1;	// NB. Not optimised for 65C02
			if (mem[base+x] != ((d+1)&0xff)) return 1;
		}

		// page-cross
		{
			const BYTE x = 1;

			INC_ABSX(x, base, d);
			if (Cpu6502(0) != 7) return 1;
			if (mem[base+x] != ((d+1)&0xff)) return 1;

			INC_ABSX(x, base, d);
			if (Cpu65C02(0) != 7) return 1;
			if (mem[base+x] != ((d+1)&0xff)) return 1;
		}
	}

	return 0;
}

//-------------------------------------

enum {CYC_6502=0, CYC_6502_PX, CYC_65C02, CYC_65C02_PX};

const BYTE g_OpcodeTimings[256][4] =
{
// 6502 (no page-cross), 6502 (page-cross), 65C02 (no page-cross), 65C02 (page-cross)
	{7,7,7,7},	// 00
	{6,6,6,6},	// 01
	{2,2,2,2},	// 02
	{8,8,2,2},	// 03
	{3,3,5,5},	// 04
	{3,3,3,3},	// 05
	{5,5,5,5},	// 06
	{5,5,2,2},	// 07
	{3,3,3,3},	// 08
	{2,2,2,2},	// 09
	{2,2,2,2},	// 0A
	{2,2,2,2},	// 0B
	{4,5,6,6},	// 0C
	{4,4,4,4},	// 0D
	{6,6,6,6},	// 0E
	{6,6,2,2},	// 0F
	{3,3,3,3},	// 10
	{5,6,5,6},	// 11
	{2,2,5,5},	// 12
	{8,8,2,2},	// 13
	{4,4,5,5},	// 14
	{4,4,4,4},	// 15
	{6,6,6,6},	// 16
	{6,6,2,2},	// 17
	{2,2,2,2},	// 18
	{4,5,4,5},	// 19
	{2,2,2,2},	// 1A
	{7,7,2,2},	// 1B
	{4,5,6,6},	// 1C
	{4,5,4,5},	// 1D
	{7,7,6,7},	// 1E
	{7,7,2,2},	// 1F
	{6,6,6,6},	// 20
	{6,6,6,6},	// 21
	{2,2,2,2},	// 22
	{8,8,2,2},	// 23
	{3,3,3,3},	// 24
	{3,3,3,3},	// 25
	{5,5,5,5},	// 26
	{5,5,2,2},	// 27
	{4,4,4,4},	// 28
	{2,2,2,2},	// 29
	{2,2,2,2},	// 2A
	{2,2,2,2},	// 2B
	{4,4,4,4},	// 2C
	{2,2,2,2},	// 2D
	{6,6,6,6},	// 2E
	{6,6,2,2},	// 2F
	{2,2,2,2},	// 30
	{5,6,5,6},	// 31
	{2,2,5,5},	// 32
	{8,8,2,2},	// 33
	{4,4,4,4},	// 34
	{4,4,4,4},	// 35
	{6,6,6,6},	// 36
	{6,6,2,2},	// 37
	{2,2,2,2},	// 38
	{4,5,4,5},	// 39
	{2,2,2,2},	// 3A
	{7,7,2,2},	// 3B
	{4,5,4,5},	// 3C
	{4,5,4,5},	// 3D
	{6,6,6,7},	// 3E
	{7,7,2,2},	// 3F
	{6,6,6,6},	// 40
	{6,6,6,6},	// 41
	{2,2,2,2},	// 42
	{8,8,2,2},	// 43
	{3,3,3,3},	// 44
	{3,3,3,3},	// 45
	{5,5,5,5},	// 46
	{5,5,2,2},	// 47
	{3,3,3,3},	// 48
	{2,2,2,2},	// 49
	{2,2,2,2},	// 4A
	{2,2,2,2},	// 4B
	{3,3,3,3},	// 4C
	{4,4,4,4},	// 4D
	{6,6,6,6},	// 4E
	{6,6,2,2},	// 4F
	{3,3,3,3},	// 50
	{5,6,5,6},	// 51
	{2,2,5,5},	// 52
	{8,8,2,2},	// 53
	{4,4,4,4},	// 54
	{4,4,4,4},	// 55
	{6,6,6,6},	// 56
	{6,6,2,2},	// 57
	{2,2,2,2},	// 58
	{4,5,4,5},	// 59
	{2,2,3,3},	// 5A
	{7,7,2,2},	// 5B
	{4,5,8,9},	// 5C
	{4,5,4,5},	// 5D
	{6,6,6,7},	// 5E
	{7,7,2,2},	// 5F
	{6,6,6,6},	// 60
	{6,6,6,6},	// 61
	{2,2,2,2},	// 62
	{8,8,2,2},	// 63
	{3,3,3,3},	// 64
	{3,3,3,3},	// 65
	{5,5,5,5},	// 66
	{5,5,2,2},	// 67
	{4,4,4,4},	// 68
	{2,2,2,2},	// 69
	{2,2,2,2},	// 6A
	{2,2,2,2},	// 6B
	{5,5,7,7},	// 6C
	{4,4,4,4},	// 6D
	{6,6,6,6},	// 6E
	{6,6,2,2},	// 6F
	{2,2,2,2},	// 70
	{5,6,5,6},	// 71
	{2,2,5,5},	// 72
	{8,8,2,2},	// 73
	{4,4,4,4},	// 74
	{4,4,4,4},	// 75
	{6,6,6,6},	// 76
	{6,6,2,2},	// 77
	{2,2,2,2},	// 78
	{4,5,4,5},	// 79
	{2,2,4,4},	// 7A
	{7,7,2,2},	// 7B
	{4,5,6,6},	// 7C
	{4,5,4,5},	// 7D
	{6,6,6,7},	// 7E
	{7,7,2,2},	// 7F
	{2,2,3,3},	// 80
	{6,6,6,6},	// 81
	{2,2,2,2},	// 82
	{6,6,2,2},	// 83
	{3,3,3,3},	// 84
	{3,3,3,3},	// 85
	{3,3,3,3},	// 86
	{3,3,2,2},	// 87
	{2,2,2,2},	// 88
	{2,2,2,2},	// 89
	{2,2,2,2},	// 8A
	{2,2,2,2},	// 8B
	{4,4,4,4},	// 8C
	{4,4,4,4},	// 8D
	{4,4,4,4},	// 8E
	{4,4,2,2},	// 8F
	{3,3,3,3},	// 90
	{6,6,6,6},	// 91
	{2,2,5,5},	// 92
	{6,6,2,2},	// 93
	{4,4,4,4},	// 94
	{4,4,4,4},	// 95
	{4,4,4,4},	// 96
	{4,4,2,2},	// 97
	{2,2,2,2},	// 98
	{5,5,5,5},	// 99
	{2,2,2,2},	// 9A
	{5,5,2,2},	// 9B
	{5,5,4,4},	// 9C
	{5,5,5,5},	// 9D
	{5,5,5,5},	// 9E
	{5,5,2,2},	// 9F
	{2,2,2,2},	// A0
	{6,6,6,6},	// A1
	{2,2,2,2},	// A2
	{6,6,2,2},	// A3
	{3,3,3,3},	// A4
	{3,3,3,3},	// A5
	{3,3,3,3},	// A6
	{3,3,2,2},	// A7
	{2,2,2,2},	// A8
	{2,2,2,2},	// A9
	{2,2,2,2},	// AA
	{2,2,2,2},	// AB
	{4,4,4,4},	// AC
	{4,4,4,4},	// AD
	{4,4,4,4},	// AE
	{4,4,2,2},	// AF
	{2,2,2,2},	// B0
	{5,6,5,6},	// B1
	{2,2,5,5},	// B2
	{5,6,2,2},	// B3
	{4,4,4,4},	// B4
	{4,4,4,4},	// B5
	{4,4,4,4},	// B6
	{4,4,2,2},	// B7
	{2,2,2,2},	// B8
	{4,5,4,5},	// B9
	{2,2,2,2},	// BA
	{4,5,2,2},	// BB
	{4,5,4,5},	// BC
	{4,5,4,5},	// BD
	{4,5,4,5},	// BE
	{4,5,2,2},	// BF
	{2,2,2,2},	// C0
	{6,6,6,6},	// C1
	{2,2,2,2},	// C2
	{8,8,2,2},	// C3
	{3,3,3,3},	// C4
	{3,3,3,3},	// C5
	{5,5,5,5},	// C6
	{5,5,2,2},	// C7
	{2,2,2,2},	// C8
	{2,2,2,2},	// C9
	{2,2,2,2},	// CA
	{2,2,2,2},	// CB
	{4,4,4,4},	// CC
	{4,4,4,4},	// CD
	{6,6,6,6},	// CE
	{6,6,2,2},	// CF
	{3,3,3,3},	// D0
	{5,6,5,6},	// D1
	{2,2,5,5},	// D2
	{8,8,2,2},	// D3
	{4,4,4,4},	// D4
	{4,4,4,4},	// D5
	{6,6,6,6},	// D6
	{6,6,2,2},	// D7
	{2,2,2,2},	// D8
	{4,5,4,5},	// D9
	{2,2,3,3},	// DA
	{7,7,2,2},	// DB
	{4,5,4,5},	// DC
	{4,5,4,5},	// DD
	{7,7,7,7},	// DE
	{7,7,2,2},	// DF
	{2,2,2,2},	// E0
	{6,6,6,6},	// E1
	{2,2,2,2},	// E2
	{8,8,2,2},	// E3
	{3,3,3,3},	// E4
	{3,3,3,3},	// E5
	{5,5,5,5},	// E6
	{5,5,2,2},	// E7
	{2,2,2,2},	// E8
	{2,2,2,2},	// E9
	{2,2,2,2},	// EA
	{2,2,2,2},	// EB
	{4,4,4,4},	// EC
	{4,4,4,4},	// ED
	{6,6,6,6},	// EE
	{6,6,2,2},	// EF
	{2,2,2,2},	// F0
	{5,6,5,6},	// F1
	{2,2,5,5},	// F2
	{8,8,2,2},	// F3
	{4,4,4,4},	// F4
	{4,4,4,4},	// F5
	{6,6,6,6},	// F6
	{6,6,2,2},	// F7
	{2,2,2,2},	// F8
	{4,5,4,5},	// F9
	{2,2,4,4},	// FA
	{7,7,2,2},	// FB
	{4,5,4,5},	// FC
	{4,5,4,5},	// FD
	{7,7,7,7},	// FE
	{7,7,2,2},	// FF
};

int GH278_Bcc_Sub(BYTE op, BYTE ps_not_taken, BYTE ps_taken, WORD pc)
{
	mem[pc+0] = op;
	mem[pc+1] = 0x01;
	const WORD dst_not_taken = pc+2;
	const WORD dst_taken     = pc+2 + mem[pc+1];

	const int pagecross = (((pc+2) ^ dst_taken) >> 8) & 1;

	// 6502

	reset();
	regs.pc = pc;
	regs.ps = ps_not_taken;
	if (Cpu6502(0) != 2) return 1;
	if (regs.pc != dst_not_taken) return 1;

	reset();
	regs.pc = pc;
	regs.ps = ps_taken;
	if (Cpu6502(0) != 3+pagecross) return 1;
	if (regs.pc != dst_taken) return 1;

	// 65C02

	reset();
	regs.pc = pc;
	regs.ps = ps_not_taken;
	if (Cpu65C02(0) != 2) return 1;
	if (regs.pc != dst_not_taken) return 1;

	reset();
	regs.pc = pc;
	regs.ps = ps_taken;
	if (Cpu65C02(0) != 3+pagecross) return 1;
	if (regs.pc != dst_taken) return 1;

	return 0;
}

int GH278_Bcc(BYTE op, BYTE ps_not_taken, BYTE ps_taken)
{
	if (GH278_Bcc_Sub(op, ps_not_taken, ps_taken, 0x300)) return 1;	// no page cross
	if (GH278_Bcc_Sub(op, ps_not_taken, ps_taken, 0x3FD)) return 1;	// page cross

	return 0;
}

int GH278_BRA(void)
{
	// No page-cross
	{
		WORD pc = 0x300;
		mem[pc+0] = 0x80;	// BRA
		mem[pc+1] = 0x01;
		const WORD dst_taken = pc+2 + mem[pc+1];

		reset();
		regs.pc = pc;
		if (Cpu65C02(0) != 3) return 1;
		if (regs.pc != dst_taken) return 1;
	}

	// Page-cross
	{
		WORD pc = 0x3FD;
		mem[pc+0] = 0x80;	// BRA
		mem[pc+1] = 0x01;
		const WORD dst_taken = pc+2 + mem[pc+1];

		reset();
		regs.pc = pc;
		if (Cpu65C02(0) != 4) return 1;
		if (regs.pc != dst_taken) return 1;
	}

	return 0;
}

int GH278_JMP_INDX(void)
{
	// No page-cross
	reset();
	regs.pc = 0x300;
	WORD abs = regs.pc+3;
	WORD dst = abs+2;
	mem[regs.pc+0] = 0x7c;	// JMP (IND,X)
	mem[regs.pc+1] = abs&0xff;
	mem[regs.pc+2] = abs>>8;
	mem[regs.pc+3] = dst&0xff;
	mem[regs.pc+4] = dst>>8;

	DWORD cycles = Cpu65C02(0);
	if (cycles != 6) return 1;
	if (regs.pc != dst) return 1;

	// Page-cross (case 1)
	reset();
	regs.pc = 0x3fc;
	abs = regs.pc+3;
	dst = abs+2;
	mem[regs.pc+0] = 0x7c;	// JMP (IND,X)
	mem[regs.pc+1] = abs&0xff;
	mem[regs.pc+2] = abs>>8;
	mem[regs.pc+3] = dst&0xff;
	mem[regs.pc+4] = dst>>8;

	cycles = Cpu65C02(0);
	if (cycles != 6) return 1;	// todo: is this 6 or 7?
	if (regs.pc != dst) return 1;

	// Page-cross (case 2)
	reset();
	regs.x = 1;
	regs.pc = 0x3fa;
	abs = regs.pc+3;
	dst = abs+2 + regs.x;
	mem[regs.pc+0] = 0x7c;	// JMP (IND,X)
	mem[regs.pc+1] = abs&0xff;
	mem[regs.pc+2] = abs>>8;
	mem[regs.pc+3] = 0xcc;	// unused
	mem[regs.pc+4] = dst&0xff;
	mem[regs.pc+5] = dst>>8;

	cycles = Cpu65C02(0);
	if (cycles != 6) return 1;	// todo: is this 6 or 7?
	if (regs.pc != dst) return 1;

	return 0;
}

int GH278_ADC_SBC(UINT op)
{
	const WORD base = 0x20ff;
	reset();
	mem[regs.pc+0] = op;
	mem[regs.pc+1] = base&0xff;
	mem[regs.pc+2] = base>>8;
	mem[0xff] = 0xff; mem[0x00] = 0x00;	// For: OPCODE (zp),Y

	// No page-cross
	reset();
	regs.ps = AF_DECIMAL;
	DWORD cycles = Cpu6502(0);
	if (g_OpcodeTimings[op][CYC_6502] != cycles) return 1;

	reset();
	regs.ps = AF_DECIMAL;
	cycles = Cpu65C02(0);
	if (g_OpcodeTimings[op][CYC_65C02]+1 != cycles) return 1;	// CMOS is +1 cycles in decimal mode

	// Page-cross
	reset();
	regs.ps = AF_DECIMAL;
	regs.x = 1;
	regs.y = 1;
	cycles = Cpu6502(0);
	if (g_OpcodeTimings[op][CYC_6502_PX] != cycles) return 1;

	reset();
	regs.ps = AF_DECIMAL;
	regs.x = 1;
	regs.y = 1;
	cycles = Cpu65C02(0);
	if (g_OpcodeTimings[op][CYC_65C02_PX]+1 != cycles) return 1;	// CMOS is +1 cycles in decimal mode

	return 0;
}

int GH278_ADC(void)
{
	const BYTE adc[] = {0x61,0x65,0x69,0x6D,0x71,0x72,0x75,0x79,0x7D};

	for (UINT i = 0; i<sizeof(adc); i++)
	{
		if (GH278_ADC_SBC(adc[i])) return 1;
	}

	return 0;
}

int GH278_SBC(void)
{
	const BYTE sbc[] = {0xE1,0xE5,0xE9,0xED,0xF1,0xF2,0xF5,0xF9,0xFD};

	for (UINT i = 0; i<sizeof(sbc); i++)
	{
		if (GH278_ADC_SBC(sbc[i])) return 1;
	}

	return 0;
}

int GH278_test(void)
{
	int variant = 0;

	//
	// 6502
	//

	// No page-cross
	for (UINT op=0; op<256; op++)
	{
		reset();
		WORD base = 0x20ff;
		mem[regs.pc+0] = op;
		mem[regs.pc+1] = base&0xff;
		mem[regs.pc+2] = base>>8;
		DWORD cycles = Cpu6502(0);
		if (g_OpcodeTimings[op][variant] != cycles) return 1;
	}

	variant++;

	// Page-cross
	for (UINT op=0; op<256; op++)
	{
		reset();
		regs.x = 1;
		regs.y = 1;
		WORD base = 0x20ff;
		mem[regs.pc+0] = op;
		mem[regs.pc+1] = base&0xff;
		mem[regs.pc+2] = base>>8;
		mem[0xff] = 0xff; mem[0x00] = 0x00;	// For: OPCODE (zp),Y
		DWORD cycles = Cpu6502(0);
		if (g_OpcodeTimings[op][variant] != cycles) return 1;
	}

	variant++;

	//
	// 65C02
	//

	// No page-cross
	for (UINT op=0; op<256; op++)
	{
		reset();
		WORD base = 0x20ff;
		mem[regs.pc+0] = op;
		mem[regs.pc+1] = base&0xff;
		mem[regs.pc+2] = base>>8;
		DWORD cycles = Cpu65C02(0);
		if (g_OpcodeTimings[op][variant] != cycles) return 1;
	}

	variant++;

	// Page-cross
	for (UINT op=0; op<256; op++)
	{
		reset();
		regs.x = 1;
		regs.y = 1;
		WORD base = 0x20ff;
		mem[regs.pc+0] = op;
		mem[regs.pc+1] = base&0xff;
		mem[regs.pc+2] = base>>8;
		mem[0xff] = 0xff; mem[0x00] = 0x00;	// For: OPCODE (zp),Y
		DWORD cycles = Cpu65C02(0);
		if (g_OpcodeTimings[op][variant] != cycles) return 1;
	}

	//
	// Bcc
	//

	if (GH278_Bcc(0x10, AF_SIGN, 0)) return 1;		// BPL
	if (GH278_Bcc(0x30, 0, AF_SIGN)) return 1;		// BMI
	if (GH278_Bcc(0x50, AF_OVERFLOW, 0)) return 1;	// BVC
	if (GH278_Bcc(0x70, 0, AF_OVERFLOW)) return 1;	// BVS
	if (GH278_Bcc(0x90, AF_CARRY, 0)) return 1;		// BCC
	if (GH278_Bcc(0xB0, 0, AF_CARRY)) return 1;		// BCS
	if (GH278_Bcc(0xD0, AF_ZERO, 0)) return 1;		// BNE
	if (GH278_Bcc(0xF0, 0, AF_ZERO)) return 1;		// BEQ
	if (GH278_BRA()) return 1;						// BRA

	//
	// JMP (IND) and JMP (IND,X)
	// . NB. GH264_test() tests JMP (IND)
	//

	if (GH278_JMP_INDX()) return 1;

	//
	// ADC/SBC CMOS decimal mode is +1 cycles
	//

	if (GH278_ADC()) return 1;
	if (GH278_SBC()) return 1;

	return 0;
}

//-------------------------------------

DWORD AXA_ZPY(BYTE a, BYTE x, BYTE y, WORD base)
{
	reset();
	mem[0xfe] = base&0xff;
	mem[0xff] = base>>8;
	regs.a = a;
	regs.x = x;
	regs.y = y;
	mem[regs.pc+0] = 0x93;
	mem[regs.pc+1] = 0xfe;
	return Cpu6502(0);
}

DWORD AXA_ABSY(BYTE a, BYTE x, BYTE y, WORD base)
{
	reset();
	regs.a = a;
	regs.x = x;
	regs.y = y;
	mem[regs.pc+0] = 0x9f;
	mem[regs.pc+1] = base&0xff;
	mem[regs.pc+2] = base>>8;
	return Cpu6502(0);
}

DWORD SAY_ABSX(BYTE a, BYTE x, BYTE y, WORD base)
{
	reset();
	regs.a = a;
	regs.x = x;
	regs.y = y;
	mem[regs.pc+0] = 0x9c;
	mem[regs.pc+1] = base&0xff;
	mem[regs.pc+2] = base>>8;
	return Cpu6502(0);
}

DWORD TAS_ABSY(BYTE a, BYTE x, BYTE y, WORD base)
{
	reset();
	regs.a = a;
	regs.x = x;
	regs.y = y;
	mem[regs.pc+0] = 0x9b;
	mem[regs.pc+1] = base&0xff;
	mem[regs.pc+2] = base>>8;
	return Cpu6502(0);
}

DWORD XAS_ABSY(BYTE a, BYTE x, BYTE y, WORD base)
{
	reset();
	regs.a = a;
	regs.x = x;
	regs.y = y;
	mem[regs.pc+0] = 0x9e;
	mem[regs.pc+1] = base&0xff;
	mem[regs.pc+2] = base>>8;
	return Cpu6502(0);
}

int GH282_test(void)
{
	// axa (zp),y
	{
		WORD base = 0x20ff, addr = 0x20ff;
		mem[addr] = 0xcc;
		BYTE a = 0xea, x = 0xff, y = 0;
		DWORD cycles = AXA_ZPY(a, x, y, base);
		if (cycles != 6) return 1;
		if (mem[addr] != (a & x & ((base>>8)+1))) return 1;
	}

	// axa (zp),y (page-cross)
	{
		WORD base = 0x20ff, addr = 0x2000;
		mem[addr] = 0xcc;
		BYTE a = 0xea, x = 0xff, y = 1;
		DWORD cycles = AXA_ZPY(a, x, y, base);
		if (cycles != 6) return 1;
		if (mem[addr] != (a & x & ((base>>8)+1))) return 1;
	}

	//

	// axa abs,y
	{
		WORD base = 0x20ff, addr = 0x20ff;
		mem[addr] = 0xcc;
		BYTE a = 0xea, x = 0xff, y = 0;
		DWORD cycles = AXA_ABSY(a, x, y, base);
		if (cycles != 5) return 1;
		if (mem[addr] != (a & x & ((base>>8)+1))) return 1;
	}

	// axa abs,y (page-cross)
	{
		WORD base = 0x20ff, addr = 0x2000;
		mem[addr] = 0xcc;
		BYTE a = 0xea, x = 0xff, y = 1;
		DWORD cycles = AXA_ABSY(a, x, y, base);
		if (cycles != 5) return 1;
		if (mem[addr] != (a & x & ((base>>8)+1))) return 1;
	}

	//

	// say abs,x
	{
		WORD base = 0x20ff, addr = 0x20ff;
		mem[addr] = 0xcc;
		BYTE a = 0xea, x = 0, y=0x20;
		DWORD cycles = SAY_ABSX(a, x, y, base);
		if (cycles != 5) return 1;
		if (mem[addr] != (y & ((base>>8)+1))) return 1;
	}

	// say abs,x (page-cross)
	{
		WORD base = 0x20ff, addr = 0x2000;
		mem[addr] = 0xcc;
		BYTE a = 0xea, x = 1, y=0x20;
		DWORD cycles = SAY_ABSX(a, x, y, base);
		if (cycles != 5) return 1;
		if (mem[addr] != (y & ((base>>8)+1))) return 1;
	}

	//

	// tas abs,y
	{
		WORD base = 0x20ff, addr = 0x20ff;
		mem[addr] = 0xcc;
		BYTE a = 0xea, x = 0xff, y = 0;
		DWORD cycles = TAS_ABSY(a, x, y, base);
		if (cycles != 5) return 1;
		if (mem[addr] != (a & x & ((base>>8)+1))) return 1;
		if (regs.sp != (0x100 | (a & x))) return 1;
	}

	// tas abs,y (page-cross)
	{
		WORD base = 0x20ff, addr = 0x2000;
		mem[addr] = 0xcc;
		BYTE a = 0xea, x = 0xff, y = 1;
		DWORD cycles = TAS_ABSY(a, x, y, base);
		if (cycles != 5) return 1;
		if (mem[addr] != (a & x & ((base>>8)+1))) return 1;
		if (regs.sp != (0x100 | (a & x))) return 1;
	}

	//

	// xas abs,y
	{
		WORD base = 0x20ff, addr = 0x20ff;
		mem[addr] = 0xcc;
		BYTE a = 0xea, x = 0x20, y = 0;
		DWORD cycles = XAS_ABSY(a, x, y, base);
		if (cycles != 5) return 1;
		if (mem[addr] != (x & ((base>>8)+1))) return 1;
	}

	// xas abs,y (page-cross)
	{
		WORD base = 0x20ff, addr = 0x2000;
		mem[addr] = 0xcc;
		BYTE a = 0xea, x = 0x20, y = 1;
		DWORD cycles = XAS_ABSY(a, x, y, base);
		if (cycles != 5) return 1;
		if (mem[addr] != (x & ((base>>8)+1))) return 1;
	}

	return 0;
}

//-------------------------------------

int _tmain(int argc, _TCHAR* argv[])
{
	int res = 1;
	init();
	reset();

	res = GH264_test();
	if (res) return res;

	res = GH271_test();
	if (res) return res;

	res = GH278_test();
	if (res) return res;

	res = GH282_test();
	if (res) return res;

	return 0;
}
