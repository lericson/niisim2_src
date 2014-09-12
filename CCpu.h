/*
NIISim - Nios II Simulator, A simulator that is capable of simulating various systems containing Nios II cpus.
Copyright (C) 2010,2011 Emil Bäckström
Copyright (C) 2012 Emil Lenngren

This file is part of NIISim.

NIISim is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

NIISim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with NIISim.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _CCPU_H_
#define _CCPU_H_

//#include <windows.h>
#include <cstdio>
#include <cstring>

#include "types.h"

// OP Encodings (45)
#define INSTR_CALL		0x00
#define INSTR_JMPI		0x01

#define INSTR_LDBU		0x03
#define INSTR_ADDI		0x04
#define INSTR_STB		0x05
#define INSTR_BR		0x06
#define INSTR_LDB		0x07
#define INSTR_CMPGEI	0x08


#define INSTR_LDHU		0x0B
#define INSTR_ANDI		0x0C
#define INSTR_STH		0x0D
#define INSTR_BGE		0x0E
#define INSTR_LDH		0x0F

#define INSTR_CMPLTI	0x10

#define INSTR_INITDA	0x13
#define INSTR_ORI		0x14
#define INSTR_STW		0x15
#define INSTR_BLT		0x16
#define INSTR_LDW		0x17
#define INSTR_CMPNEI	0x18


#define INSTR_FLUSHDA	0x1B
#define INSTR_XORI		0x1C

#define INSTR_BNE		0x1E

#define INSTR_CMPEQI	0x20

#define INSTR_LDBUIO	0x23
#define INSTR_MULI		0x24
#define INSTR_STBIO		0x25
#define INSTR_BEQ		0x26
#define INSTR_LDBIO		0x27
#define INSTR_CMPGEUI	0x28


#define INSTR_LDHUIO	0x2B
#define INSTR_ANDHI		0x2C
#define INSTR_STHIO		0x2D
#define INSTR_BGEU		0x2E
#define INSTR_LDHIO		0x2F

#define INSTR_CMPLTUI	0x30

#define INSTR_CUSTOM	0x32
#define INSTR_INITD		0x33
#define INSTR_ORHI		0x34
#define INSTR_STWIO		0x35
#define INSTR_BLTU		0x36
#define INSTR_LDWIO		0x37
#define INSTR_RDPRS		0x38

#define INSTR_R_TYPE	0x3A	// Not an instruction
#define INSTR_FLUSHD	0x3B
#define INSTR_XORHI		0x3C

// OPX Encodings for R-Type instructions (42)

#define INSTR_R_ERET	0x01
#define INSTR_R_ROLI	0x02
#define INSTR_R_ROL		0x03
#define INSTR_R_FLUSHP	0x04
#define INSTR_R_RET		0x05
#define INSTR_R_NOR		0x06
#define INSTR_R_MULXUU	0x07
#define INSTR_R_CMPGE	0x08
#define INSTR_R_BRET	0x09

#define INSTR_R_ROR		0x0B
#define INSTR_R_FLUSHI	0x0C
#define INSTR_R_JMP		0x0D
#define INSTR_R_AND		0x0E

#define INSTR_R_CMPLT	0x10

#define INSTR_R_SLLI	0x12
#define INSTR_R_SLL		0x13
#define INSTR_R_WRPRS	0x14

#define INSTR_R_OR		0x16
#define INSTR_R_MULXSU	0x17
#define INSTR_R_CMPNE	0x18

#define INSTR_R_SRLI	0x1A
#define INSTR_R_SRL		0x1B
#define INSTR_R_NEXTPC	0x1C
#define INSTR_R_CALLR	0x1D
#define INSTR_R_XOR		0x1E
#define INSTR_R_MULXSS	0x1F

#define INSTR_R_CMPEQ	0x20



#define INSTR_R_DIVU	0x24
#define INSTR_R_DIV		0x25
#define INSTR_R_RDCTL	0x26
#define INSTR_R_MUL		0x27
#define INSTR_R_CMPGEU	0x28
#define INSTR_R_INITI	0x29



#define INSTR_R_TRAP	0x2D
#define INSTR_R_WRCTL	0x2E

#define INSTR_R_CMPLTU	0x30
#define INSTR_R_ADD		0x31


#define INSTR_R_BREAK	0x34

#define INSTR_R_SYNC	0x36


#define INSTR_R_SUB		0x39
#define INSTR_R_SRAI	0x3A
#define INSTR_R_SRA		0x3B

class CCpu
{
private:
	// Structure describing an instruction
	struct Instruction
	{
		UINT rA;
		UINT rB;
		UINT rC;
		UINT IMM16;
		UINT IMM26;
		UINT OPX_1, OPX_2; 
		UINT OP; 
	};
	UINT reg[32];			// The 32 registers in the cpu
	UINT ctrl_reg[32];		// The 32 control register in the cpu (not all are used!)
	UINT pending_irq;		// 32 pending irqs. This AND ienable == ipending

	UINT pc;				// The program counter

	UINT reset_addr, exception_addr;	// The reset and exception addresses

	char name[256];			// The name of the cpu

	UINT freq;				// The frequency of the cpu

	// Data transfer instructions
	void ExecLoad(Instruction *instr, bool io);
	void ExecStore(Instruction *instr, bool io);

	// Arithmetic and logical instructions
	void ExecAnd(Instruction *instr);
	void ExecOr(Instruction *instr);
	void ExecXor(Instruction *instr);
	void ExecNor(Instruction *instr);

	void ExecAdd(Instruction *instr);
	void ExecSub(Instruction *instr);
	void ExecMul(Instruction *instr);
	void ExecDiv(Instruction *instr);

	// Comparison instructions
	void ExecCmp(Instruction *instr);

	// Shift and rotate instructions
	void ExecRotate(Instruction *instr);
	void ExecShift(Instruction *instr);

	// Program control instructions
	void ExecCall(Instruction *instr);
	void ExecRet(Instruction *instr);
	void ExecJmp(Instruction *instr);
	void ExecBr(Instruction *instr);

	// Other control instructions
	void ExecTrap(Instruction *instr);
	void ExecBreak(Instruction *instr);
	void ExecReadControl(Instruction *instr);
	void ExecWriteControl(Instruction *instr);
	void ExecCache(Instruction *instr);
	void ExecFlushPipeline(Instruction *instr);
	void ExecSync(Instruction *instr);
	void ExecNextPC(Instruction *instr);
	void ExecCustom(Instruction *instr);
	void ExecWrprs(Instruction *instr);
	void ExecRdprs(Instruction *instr);

	UINT SignExtend(UINT num, UINT bits);
	void IssueException(UINT old_pc);
	void UpdatePC(UINT new_pc);
	UINT MemLoad(UINT addr, UINT size, bool io, bool fetch);
	void MemStore(UINT addr, UINT size, UINT data, bool io);
	
	void ShowMisalignedMemError(UINT addr, UINT size, UINT data, bool read, bool update_pc = true);
	void ShowInvalidMemAddressError(UINT addr, UINT size, UINT data, bool read, bool update_pc = true);
public:
	CCpu();
	~CCpu();

	void OnClock();
	void Reset();

	UINT GetReg(int index);
	void SetReg(UINT r, UINT data);

	UINT GetCtrlReg(int index);
	void SetCtrlReg(UINT r, UINT data);

	UINT GetPC() { return pc; };
	void SetPC(UINT new_pc) { pc = new_pc; };

	void SetName(const char *n) { strcpy(name,  n); };
	const char *GetName() { return name; };

	void SetFrequency(UINT f) { freq = f; };
	UINT GetFrequency() { return freq; };

	void SetResetAddress(UINT addr) { reset_addr = addr; };
	UINT GetResetAddress() { return reset_addr; };

	void SetExceptionAddress(UINT addr) { exception_addr = addr; };
	UINT GetExceptionAddress() { return exception_addr; };

	void EnableIRQ(UINT irq);
	void DisableIRQ(UINT irq);
	void AssertIRQ(UINT irq);
	void DeassertIRQ(UINT irq);
	
	struct StopError 
	{
		char msg[256];
	};
};

#endif
