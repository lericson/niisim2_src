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

#include "sim.h"
#include "CCpu.h"

/*
 *	CCpu::CCpu()
 *
 *  Constructor for the CCpu class.
 *  Initializes all private variables to 0.
 */
CCpu::CCpu()
{
	for(int i=0; i<32; i++)
	{
		reg[i] = 0;
		ctrl_reg[i] = 0;
	}

	reset_addr = exception_addr = pc = 0;
	name[0] = '\0';
	freq = 0;
}

/*
 *	CCpu::~CCpu()
 *
 *  Destructor for the CCpu class.
 */
CCpu::~CCpu()
{
}

/*
 *	CCpu::Reset()
 *
 *  Performs a reset operation for the CCpu class by resetting all regsiters to 0 and
 *  setting the PC to reset_addr
 */
void CCpu::Reset()
{	
	// Loop through all registers and set them to 0
	for(int i=0; i<32; i++)
	{
		reg[i] = 0;
		ctrl_reg[i] = 0;
	}

	// Reset the PC
	pc = reset_addr;
}

/*
 *	CCpu::OnClock()
 *
 *  Executes one instruction and checks for hardware interrupts
 */
void CCpu::OnClock()
{
	UINT instr;
	Instruction s_instr;

	// Check for hardware interrupts	
	// Check if the PIE bit is 1
	if(ctrl_reg[0] & 0x1)
	{
		// Check ipending to see if there are any hardware interrupts pending
		if(ctrl_reg[4])
		{
			// Issue an exception. The instruction at PC got interrupted so we pass
			// PC + 4, which is the next instruction, as parameter
			IssueException(pc + 4);
		}
	}

	try
	{
		// Check if pc is valid
		if(!main_system.IsAddressValid(pc))
		{
			// If it isn't, display an error message and stop the simulation
			ShowInvalidMemAddressError(pc, 4, 0, true, false);
			//SendMessage(hWnd, WM_COMMAND, ID_CPU_STOP, NULL);
			return;
		}
	}
	catch(const StopError& e)
	{
		ReportError(e.msg);
		goto do_break;
	}
		
	// Handle breakpoints for debugging
	if(main_debug.AddressIsBreakpoint(pc))
	{
		// Ugly label, I know...
		do_break:
		main_system.PauseSimulationThread();
		main_debug.BreakFromThread(pc);
		
		while(main_system.IsSimulationRunning() && main_system.IsSimulationPaused())
		{
			CThread::Sleep(100); // 100 ms
		}
		
		if(!main_system.IsSimulationRunning())
			return;
		
		try
		{
			// Check if pc is valid, the user might have changed it
			if(!main_system.IsAddressValid(pc))
			{
				// If it isn't, display an error message and stop the simulation
				ShowInvalidMemAddressError(pc, 4, 0, true, false);
				//SendMessage(hWnd, WM_COMMAND, ID_CPU_STOP, NULL);
				return;
			}
		}
		catch(const StopError& e)
		{
			ReportError(e.msg);
			goto do_break;
		}
	}
	
	try
	{
		// Fetch the instruction from memory
		instr = main_system.Read(pc, 32, false, true);
		
		// Update PC to point to the next instruction
		UpdatePC(pc+4);

		// Decode the instruction

		s_instr.OP = instr & 0x3F;
		s_instr.OPX_1 = (instr >> 11) & 0x3F;
		s_instr.OPX_2 = (instr >> 6) & 0x1F;
		s_instr.IMM26 = (instr >> 6) & 0x3FFFFFF;
		s_instr.IMM16 = (instr >> 6) & 0xFFFF;
		s_instr.rA = (instr >> 27) & 0x1F;
		s_instr.rB = (instr >> 22) & 0x1F;
		s_instr.rC = (instr >> 17) & 0x1F;

		// Execute the instruction
		if(s_instr.OP == INSTR_R_TYPE)
		{
			switch(s_instr.OPX_1)
			{
			case INSTR_R_AND:
				ExecAnd(&s_instr);
				break;

			case INSTR_R_OR:
				ExecOr(&s_instr);
				break;

			case INSTR_R_XOR:
				ExecXor(&s_instr);
				break;

			case INSTR_R_NOR:
				ExecNor(&s_instr);
				break;

			case INSTR_R_ADD:
				ExecAdd(&s_instr);
				break;

			case INSTR_R_SUB:
				ExecSub(&s_instr);
				break;

			case INSTR_R_MUL:
			case INSTR_R_MULXUU:
			case INSTR_R_MULXSU:
			case INSTR_R_MULXSS:
				ExecMul(&s_instr);
				break;

			case INSTR_R_DIV:
			case INSTR_R_DIVU:
				ExecDiv(&s_instr);
				break;

			case INSTR_R_CMPGE:
			case INSTR_R_CMPLT:
			case INSTR_R_CMPNE:
			case INSTR_R_CMPEQ:
			case INSTR_R_CMPGEU:
			case INSTR_R_CMPLTU:
				ExecCmp(&s_instr);
				break;

			case INSTR_R_ROLI:
			case INSTR_R_ROL:
			case INSTR_R_ROR:
				ExecRotate(&s_instr);
				break;

			case INSTR_R_SLLI:
			case INSTR_R_SLL:
			case INSTR_R_SRLI:
			case INSTR_R_SRL:
			case INSTR_R_SRAI:
			case INSTR_R_SRA:
				ExecShift(&s_instr);
				break;

			case INSTR_R_CALLR:
				ExecCall(&s_instr);
				break;

			case INSTR_R_ERET:
			case INSTR_R_RET:
			case INSTR_R_BRET:
				ExecRet(&s_instr);
				break;

			case INSTR_R_JMP:
				ExecJmp(&s_instr);
				break;

			case INSTR_R_TRAP:
				ExecTrap(&s_instr);
				break;

			case INSTR_R_BREAK:
				ExecBreak(&s_instr);
				break;

			case INSTR_R_RDCTL:
				ExecReadControl(&s_instr);
				break;

			case INSTR_R_WRCTL:
				ExecWriteControl(&s_instr);
				break;

			case INSTR_R_FLUSHI:
			case INSTR_R_INITI:
				ExecCache(&s_instr);
				break;

			case INSTR_R_FLUSHP:
				ExecFlushPipeline(&s_instr);
				break;

			case INSTR_R_SYNC:
				ExecSync(&s_instr);
				break;

			case INSTR_R_NEXTPC:
				ExecNextPC(&s_instr);
				break;

			case INSTR_R_WRPRS:
				ExecWrprs(&s_instr);
				break;

			// Unimplemented instruction
			default:
				IssueException(pc);
				break;
			}
		}
		else
		{
			switch(s_instr.OP)
			{
			case INSTR_LDBU:
			case INSTR_LDB:
			case INSTR_LDHU:
			case INSTR_LDH:
			case INSTR_LDW:
				ExecLoad(&s_instr, false);
				break;

			case INSTR_LDBUIO:
			case INSTR_LDBIO:
			case INSTR_LDHUIO:
			case INSTR_LDHIO:
			case INSTR_LDWIO:
				ExecLoad(&s_instr, true);
				break;

			case INSTR_STB:
			case INSTR_STH:
			case INSTR_STW:
				ExecStore(&s_instr, false);
				break;

			case INSTR_STBIO:
			case INSTR_STHIO:
			case INSTR_STWIO:
				ExecStore(&s_instr, true);
				break;

			case INSTR_ANDI:
			case INSTR_ANDHI:
				ExecAnd(&s_instr);
				break;

			case INSTR_ORI:
			case INSTR_ORHI:
				ExecOr(&s_instr);
				break;

			case INSTR_XORI:
			case INSTR_XORHI:
				ExecXor(&s_instr);
				break;

			case INSTR_ADDI:
				ExecAdd(&s_instr);
				break;

			case INSTR_MULI:
				ExecMul(&s_instr);
				break;

			case INSTR_CMPGEI:
			case INSTR_CMPLTI:
			case INSTR_CMPNEI:
			case INSTR_CMPEQI:
			case INSTR_CMPGEUI:
			case INSTR_CMPLTUI:
				ExecCmp(&s_instr);
				break;

			case INSTR_CALL:
				ExecCall(&s_instr);
				break;

			case INSTR_JMPI:
				ExecJmp(&s_instr);
				break;

			case INSTR_BR:
			case INSTR_BGE:
			case INSTR_BLT:
			case INSTR_BNE:
			case INSTR_BEQ:
			case INSTR_BGEU:
			case INSTR_BLTU:
				ExecBr(&s_instr);
				break;

			case INSTR_INITDA:
			case INSTR_INITD:
			case INSTR_FLUSHDA:
			case INSTR_FLUSHD:
				ExecCache(&s_instr);
				break;

			case INSTR_CUSTOM:
				ExecCustom(&s_instr);
				break;

			case INSTR_RDPRS:
				ExecRdprs(&s_instr);
				break;

			// Unimplemented instruction
			default:
				IssueException(pc);
				break;
			}
		}
	}
	catch(const StopError& e)
	{
		ReportError(e.msg);
		goto do_break;
	}
}

/*
 *	CCpu::ExecLoad()
 *
 *  Executes loads instructions.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 *				io - A boolean that determines if this instruction should load data 
 *					 from the cache. False means data from cache, true means data from
 *					 the device.
 */
void CCpu::ExecLoad(Instruction *instr, bool io)
{
	UINT addr, data, size;

	// Compute the address
	addr = reg[instr->rA] + SignExtend(instr->IMM16, 16);

	// Check the size
	if(instr->OP == INSTR_LDBU || instr->OP == INSTR_LDB || 
	   instr->OP == INSTR_LDBUIO || instr->OP == INSTR_LDBIO)
		size = 8;
	else if(instr->OP == INSTR_LDHU || instr->OP == INSTR_LDH || 
			instr->OP == INSTR_LDHUIO || instr->OP == INSTR_LDHIO)
		size = 16;
	else
		size = 32;

	// Check if address is aligned correctly
	if((((addr & 0x3) != 0) && (size == 32)) ||
	   (((addr & 0x1) != 0) && (size == 16)))
	{
		// If it isn't, display an error message and stop the simulation
		ShowMisalignedMemError(addr, size, 0, true);
		//SendMessage(hWnd, WM_COMMAND, ID_CPU_STOP, NULL);
		return;
	}

	// Check if address is valid
	if(!main_system.IsAddressValid(addr))
	{
		// If it isn't, display an error message and stop the simulation
		ShowInvalidMemAddressError(addr, size, 0, true);
		//SendMessage(hWnd, WM_COMMAND, ID_CPU_STOP, NULL);
		return;
	}

	// Load the data from memory
	data = MemLoad(addr, size, io, false);

	// Sign extend if needed
	if(instr->OP == INSTR_LDB || instr->OP == INSTR_LDH || 
	   instr->OP == INSTR_LDBIO || instr->OP == INSTR_LDHIO)
		data = SignExtend(data, size);

	// Write back to register
	SetReg(instr->rB, data);
}

/*
 *	CCpu::ExecStore()
 *
 *  Executes store instructions.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 *				io - A boolean that determines if this instruction should store data 
 *					 to the cache. False means data to cache, true means data to
 *					 the device.
 */
void CCpu::ExecStore(Instruction *instr, bool io)
{
	UINT addr, size, data;

	// Compute the address
	addr = reg[instr->rA] + SignExtend(instr->IMM16, 16);

	// Retrieve the data
	data = reg[instr->rB];

	// Check the size
	if(instr->OP == INSTR_STB || instr->OP == INSTR_STBIO)
	{
		size = 8;
		data = data & 0xFF;
	}
	else if(instr->OP == INSTR_STH || instr->OP == INSTR_STHIO)
	{
		size = 16;
		data = data & 0xFFFF;
	}
	else
	{
		size = 32;
	}

	// Check if address is aligned correctly
	if((((addr & 0x3) != 0) && (size == 32)) ||
	   (((addr & 0x1) != 0) && (size == 16)))
	{
		// If it isn't, display an error message and stop the simulation
		ShowMisalignedMemError(addr, size, data, false);
		//SendMessage(hWnd, WM_COMMAND, ID_CPU_STOP, NULL);
		return;
	}

	// Check if address is valid
	if(!main_system.IsAddressValid(addr))
	{
		// If it isn't, display an error message and stop the simulation
		ShowInvalidMemAddressError(addr, size, data, false);
		//SendMessage(hWnd, WM_COMMAND, ID_CPU_STOP, NULL);
		return;
	}

	// Write the data to memory
	MemStore(addr, size, data, io);
}

/*
 *	CCpu::ExecAnd()
 *
 *  Executes AND instructions.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecAnd(Instruction *instr)
{
	UINT data, r;

	// Perform the specific AND operation
	if(instr->OP == INSTR_R_TYPE && instr->OPX_1 == INSTR_R_AND)
	{
		data = reg[instr->rA] & reg[instr->rB];
		r = instr->rC;
	}
	else if(instr->OP == INSTR_ANDHI)
	{
		data = reg[instr->rA] & ((instr->IMM16 << 16) & 0xFFFF0000);
		r = instr->rB;
	}
	else
	{
		data = reg[instr->rA] & (instr->IMM16 & 0xFFFF);
		r = instr->rB;
	}

	// Write back to register
	SetReg(r, data);
}

/*
 *	CCpu::ExecOr()
 *
 *  Executes OR instructions.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecOr(Instruction *instr)
{
	UINT data, r;

	// Perform the specific OR operation
	if(instr->OP == INSTR_R_TYPE && instr->OPX_1 == INSTR_R_OR)
	{
		data = reg[instr->rA] | reg[instr->rB];
		r = instr->rC;
	}
	else if(instr->OP == INSTR_ORHI)
	{
		data = reg[instr->rA] | ((instr->IMM16 << 16) & 0xFFFF0000);
		r = instr->rB;
	}
	else
	{
		data = reg[instr->rA] | (instr->IMM16 & 0xFFFF);
		r = instr->rB;
	}

	// Write back to register
	SetReg(r, data);
}

/*
 *	CCpu::ExecXor()
 *
 *  Executes XOR instructions.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecXor(Instruction *instr)
{
	UINT data, r;

	// Perform the specific XOR operation
	if(instr->OP == INSTR_R_TYPE && instr->OPX_1 == INSTR_R_XOR)
	{
		data = reg[instr->rA] ^ reg[instr->rB];
		r = instr->rC;
	}
	else if(instr->OP == INSTR_XORHI)
	{
		data = reg[instr->rA] ^ ((instr->IMM16 << 16) & 0xFFFF0000);
		r = instr->rB;
	}
	else
	{
		data = reg[instr->rA] ^ (instr->IMM16 & 0xFFFF);
		r = instr->rB;
	}

	// Write back to register
	SetReg(r, data);
}

/*
 *	CCpu::ExecNor()
 *
 *  Executes a NOR instruction.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecNor(Instruction *instr)
{
	UINT data;

	// Perform the NOR operation
	data = ~(reg[instr->rA] | reg[instr->rB]);

	// Write back to register
	SetReg(instr->rC, data);
}

/*
 *	CCpu::ExecAdd()
 *
 *  Executes ADD instructions.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecAdd(Instruction *instr)
{
	UINT data, r;

	// Perform the specific ADD operation
	if(instr->OP == INSTR_R_TYPE && instr->OPX_1 == INSTR_R_ADD)
	{
		data = reg[instr->rA] + reg[instr->rB];
		r = instr->rC;
	}
	else if(instr->OP == INSTR_ADDI)
	{
		data = reg[instr->rA] + SignExtend(instr->IMM16, 16);
		r = instr->rB;
	}

	// Write back to register
	SetReg(r, data);
}

/*
 *	CCpu::ExecSub()
 *
 *  Executes a SUB instruction.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecSub(Instruction *instr)
{
	UINT data;

	// Perform the SUB operation
	//data = (INT)reg[instr->rA] - (INT)reg[instr->rB];
	data = reg[instr->rA] - reg[instr->rB];

	// Write back to register
	SetReg(instr->rC, data);
}

/*
 *	CCpu::ExecMul()
 *
 *  Executes MUL instructions.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecMul(Instruction *instr)
{
	UINT data, r;
	__int64 data_64;

	// Perform the specific MUL operation
	
	if(instr->OP == INSTR_R_TYPE)
	{
		if(instr->OPX_1 == INSTR_R_MUL)
		{
			data = (INT)reg[instr->rA] * (INT)reg[instr->rB];
		}
		else if(instr->OPX_1 == INSTR_R_MULXSS)
		{
			data_64 = (__int64)(INT)reg[instr->rA] * (__int64)(INT)reg[instr->rB];
			data = (data_64 >> 32) & 0xFFFFFFFF;
		}
		else if(instr->OPX_1 == INSTR_R_MULXSU)
		{
			data_64 = (__int64)(INT)reg[instr->rA] * (__int64)reg[instr->rB];
			data = (data_64 >> 32) & 0xFFFFFFFF;
		}
		else if(instr->OPX_1 == INSTR_R_MULXUU)
		{
			data_64 = (__int64)reg[instr->rA] * (__int64)reg[instr->rB];
			data = (data_64 >> 32) & 0xFFFFFFFF;
		}
		r = instr->rC;
	}
	else if(instr->OP == INSTR_MULI)
	{
		data = (INT)reg[instr->rA] * (INT)SignExtend(instr->IMM16, 16);
		r = instr->rB;
	}

	// Write back to register
	SetReg(r, data);
}

/*
 *	CCpu::ExecDiv()
 *
 *  Executes DIV instructions.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecDiv(Instruction *instr)
{
	UINT data;

	// Perform the specific DIV operation	
	if(instr->OPX_1 == INSTR_R_DIV)
	{
		if(reg[instr->rB] != 0)
			data = (INT)reg[instr->rA] / (INT)reg[instr->rB];
	}
	else
	{
		if(reg[instr->rB] != 0)
			data = reg[instr->rA] * reg[instr->rB];
	}

	// Write back to register
	SetReg(instr->rC, data);
}

/*
 *	CCpu::ExecCmp()
 *
 *  Executes compare instructions.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecCmp(Instruction *instr)
{
	UINT data, r;

	data = 0;
	// Perform the specific cmp operation
	if(instr->OP == INSTR_R_TYPE)
	{
		r = instr->rC;
		if(instr->OPX_1 == INSTR_R_CMPGE)
		{
			if((INT)reg[instr->rA] >= (INT)reg[instr->rB])
				data = 1;
		}
		else if(instr->OPX_1 == INSTR_R_CMPGEU)
		{
			if(reg[instr->rA] >= reg[instr->rB])
				data = 1;
		}
		else if(instr->OPX_1 == INSTR_R_CMPLT)
		{
			if((INT)reg[instr->rA] < (INT)reg[instr->rB])
				data = 1;
		}
		else if(instr->OPX_1 == INSTR_R_CMPLTU)
		{
			if(reg[instr->rA] < reg[instr->rB])
				data = 1;
		}
		else if(instr->OPX_1 == INSTR_R_CMPNE)
		{
			if(reg[instr->rA] != reg[instr->rB])
				data = 1;
		}
		else if(instr->OPX_1 == INSTR_R_CMPEQ)
		{
			if(reg[instr->rA] == reg[instr->rB])
				data = 1;
		}
	}
	else
	{
		r = instr->rB;
		if(instr->OP == INSTR_CMPGEI)
		{
			if((INT)reg[instr->rA] >= (INT)SignExtend(instr->IMM16, 16))
				data = 1;
		}
		else if(instr->OP == INSTR_CMPGEUI)
		{
			if(reg[instr->rA] >= instr->IMM16)
				data = 1;
		}
		else if(instr->OP == INSTR_CMPLTI)
		{
			if((INT)reg[instr->rA] < (INT)SignExtend(instr->IMM16, 16))
				data = 1;
		}
		else if(instr->OP == INSTR_CMPLTUI)
		{
			if(reg[instr->rA] < SignExtend(instr->IMM16, 16))
				data = 1;
		}
		else if(instr->OP == INSTR_CMPNEI)
		{
			if(reg[instr->rA] != SignExtend(instr->IMM16, 16))
				data = 1;
		}
		else if(instr->OP == INSTR_CMPEQI)
		{
			if(reg[instr->rA] == SignExtend(instr->IMM16, 16))
				data = 1;
		}
	}

	// Write back to register
	SetReg(r, data);
}

/*
 *	CCpu::ExecRotate()
 *
 *  Executes rotate instructions.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecRotate(Instruction *instr)
{
	UINT data, rots;

	data = 0;
	// Determine the number of rotations
	if(instr->OPX_1 == INSTR_R_ROL || instr->OPX_1 == INSTR_R_ROR)
		rots = reg[instr->rB] & 0x1F;
	else
		rots = instr->OPX_2;

	// Perform the rotation
	if(instr->OPX_1 == INSTR_R_ROLI || instr->OPX_1 == INSTR_R_ROL)
		data = (reg[instr->rA] << rots) | (reg[instr->rA] >> (32-rots));
	else
		data = (reg[instr->rA] >> rots) | (reg[instr->rA] << (32-rots));

	// Write back to register
	SetReg(instr->rC, data);
}

/*
 *	CCpu::ExecShift()
 *
 *  Executes shift instructions.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecShift(Instruction *instr)
{
	UINT data, shifts;

	data = 0;
	// Determine the number of shifts
	if(instr->OPX_1 == INSTR_R_SLL || instr->OPX_1 == INSTR_R_SRL || instr->OPX_1 == INSTR_R_SRA)
		shifts = reg[instr->rB] & 0x1F;
	else
		shifts = instr->OPX_2;

	// Perform the shift
	if(instr->OPX_1 == INSTR_R_SLLI || instr->OPX_1 == INSTR_R_SLL)
		data = reg[instr->rA] << shifts;
	else if(instr->OPX_1 == INSTR_R_SRLI || instr->OPX_1 == INSTR_R_SRL)
		data = reg[instr->rA] >> shifts;
	else
		data = (INT)reg[instr->rA] >> shifts;

	// Write back to register
	SetReg(instr->rC, data);
}

/*
 *	CCpu::ExecCall()
 *
 *  Executes a call instruction.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecCall(Instruction *instr)
{
	UINT addr;

	// Calculate the new pc
	if(instr->OP == INSTR_R_TYPE && instr->OPX_1 == INSTR_R_CALLR)
		addr = reg[instr->rA];
	else
		addr = (pc & 0xF0000000) | (instr->IMM26 << 2);

	// Check if address is word aligned
	if((addr & 0x3) != 0)
	{
		// If it isn't, issue an exception
		IssueException(pc);
		return;
	}

	// Save the address of the next instruction in r31 (ra)
	reg[31] = pc;
	
	// Tell the debugger we enter a new function with the return address and current sp
	main_debug.EnterFunctionFromThread(pc, reg[27]);

	// Update the PC
	UpdatePC(addr);
}

/*
 *	CCpu::ExecRet()
 *
 *  Executes return instructions.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecRet(Instruction *instr)
{
	UINT addr;

	// Return from an exception
	if(instr->OPX_1 == INSTR_R_ERET)
	{
		// Copy estatus to status
		//ctrl_reg[0] = ctrl_reg[1];
		SetCtrlReg(0, ctrl_reg[1]);

		// Retrieve the return address
		addr = reg[29];

		// Check if address is word aligned
		if((addr & 0x3) != 0)
		{
			// If it isn't, issue an exception
			IssueException(pc);
			return;
		}

		//jtag_console.AddText("Leaving exception handler\n", false);

		// Update the PC
		UpdatePC(addr);
	}
	// Return from a call
	else if(instr->OPX_1 == INSTR_R_RET)
	{
		// Retrieve the return address
		addr = reg[31];

		// Check if address is word aligned
		if((addr & 0x3) != 0)
		{
			// If it isn't, issue an exception
			IssueException(pc);
			return;
		}
		
		// Tell the debugger we are returning from a function to addr with current sp
		main_debug.RetFromThread(addr, reg[27]);

		// Update the PC
		UpdatePC(addr);
	}
	// Return from a break
	else if(instr->OPX_1 == INSTR_R_BRET)
	{
		// Not implemented
	}
}

/*
 *	CCpu::ExecJmp()
 *
 *  Executes a jump instruction.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecJmp(Instruction *instr)
{
	UINT addr;

	// Calculate the new pc
	if(instr->OP == INSTR_R_TYPE && instr->OPX_1 == INSTR_R_JMP)
		addr = reg[instr->rA];
	else
		addr = (pc & 0xF0000000) | (instr->IMM26 << 2);

	// Check if address is word aligned
	if((addr & 0x3) != 0)
	{
		// If it isn't, issue an exception
		IssueException(pc);
		return;
	}

	// Update the PC
	UpdatePC(addr);
}

/*
 *	CCpu::ExecBr()
 *
 *  Executes branch instructions.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecBr(Instruction *instr)
{
	UINT addr;
	bool jump = false;

	// Determine what kind of branch instruction we have 
	// and check if we are going to do a jump
	if(instr->OP == INSTR_BEQ)
	{
		if(reg[instr->rA] == reg[instr->rB])
			jump = true;
	}
	else if(instr->OP == INSTR_BGE)
	{
		if((INT)reg[instr->rA] >= (INT)reg[instr->rB])
			jump = true;
	}
	else if(instr->OP == INSTR_BGEU)
	{
		if(reg[instr->rA] >= reg[instr->rB])
			jump = true;
	}
	else if(instr->OP == INSTR_BLT)
	{
		if((INT)reg[instr->rA] < (INT)reg[instr->rB])
			jump = true;
	}
	else if(instr->OP == INSTR_BLTU)
	{
		if(reg[instr->rA] < reg[instr->rB])
			jump = true;
	}
	else if(instr->OP == INSTR_BNE)
	{
		if(reg[instr->rA] != reg[instr->rB])
			jump = true;
	}
	else
	{
		jump = true;
	}

	// Calculate the address
	addr = pc + SignExtend(instr->IMM16, 16);

	// Check if address is word aligned
	if((addr & 0x3) != 0)
	{
		// If it isn't, issue an exception
		IssueException(pc);
		return;
	}

	// Update the PC if we are going to jump
	if(jump)
		UpdatePC(addr);
}

/*
 *	CCpu::ExecTrap()
 *
 *  Executes a trap instruction.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecTrap(Instruction *instr)
{
	// Issue an exception
	IssueException(pc);
}

/*
 *	CCpu::ExecBreak()
 *
 *  Executes a break instruction.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecBreak(Instruction *instr)
{
	// Not implemented
	IssueException(pc);
}

/*
 *	CCpu::ExecReadControl()
 *
 *  Executes a read control register instruction.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecReadControl(Instruction *instr)
{
	// Write the data from a control register to a general purpose register
	SetReg(instr->rC, ctrl_reg[instr->OPX_2]);
}

/*
 *	CCpu::ExecWriteControl()
 *
 *  Executes a write control register instruction.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecWriteControl(Instruction *instr)
{
	// Write the data from a general purpose register to a control register
	//ctrl_reg[instr->OPX_2] = reg[instr->rA];
	SetCtrlReg(instr->OPX_2, reg[instr->rA]);
}

/*
 *	CCpu::ExecCache()
 *
 *  Executes cache instructions.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecCache(Instruction *instr)
{
	// To be implemented
}

/*
 *	CCpu::ExecFlushPipeline()
 *
 *  Executes a flush pipeline instruction.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecFlushPipeline(Instruction *instr)
{
	// Not implemented. We have no pipelined ISS
}

/*
 *	CCpu::ExecSync()
 *
 *  Executes a sync instruction.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecSync(Instruction *instr)
{
	// Not implemented since memory accesses take 0 cycles
	// Simply do nothing
}

/*
 *	CCpu::ExecNextPC()
 *
 *  Executes a next pc instruction.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecNextPC(Instruction *instr)
{
	// Write PC + 4 to a register
	SetReg(instr->rC, pc);
}

/*
 *	CCpu::ExecCustom()
 *
 *  Executes a cusom instruction.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecCustom(Instruction *instr)
{
	// Not implemented
	IssueException(pc);
}

/*
 *	CCpu::ExecWrprs()
 *
 *  Executes a wrprs instruction.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecWrprs(Instruction *instr)
{
	// Not implemented
	IssueException(pc);
}

/*
 *	CCpu::ExecRdprs()
 *
 *  Executes a rdprs instruction.
 *
 *  Paramters:	instr - A pointer to a structure that describes the instruction
 */
void CCpu::ExecRdprs(Instruction *instr)
{
	// Not implemented
	IssueException(pc);
}

/*
 *	CCpu::GetReg()
 *
 *  Gets the value from a register
 *
 *  Paramters:	index - The register index, 0-31
 *
 *	Returns:	The register value
 */
UINT CCpu::GetReg(int index)
{
	if(index >= 0 && index <= 31)
		return reg[index];

	return 0;
}

/*
 *	CCpu::GetCtrlReg()
 *
 *  Gets the value from a control register
 *
 *  Paramters:	index - The register index, 0-31
 *
 *	Returns:	The register value
 */
UINT CCpu::GetCtrlReg(int index)
{
	if(index >= 0 && index <= 31)
		return ctrl_reg[index];

	return 0;
}

/*
 *	CCpu::SetReg()
 *
 *  Writes data to a register
 *
 *  Paramters:	index - The register index, 0-31
 *  Paramters:	data - The data to write
 */
void CCpu::SetReg(UINT r, UINT data)
{
	if(r != 0)
		reg[r] = data;
}

/*
 *	CCpu::SetReg()
 *
 *  Writes data to a control register
 *
 *  Paramters:	index - The register index, 0-31
 *  Paramters:	data - The data to write
 */
void CCpu::SetCtrlReg(UINT r, UINT data)
{
	// Check if we are writing to ienable
	if(r == 3)
	{
		// We are writing to ienable so we must also update ipending
		ctrl_reg[3] = data;
		ctrl_reg[4] = pending_irq & ctrl_reg[3];
		
		/*// incase some irqs have been disabled
		// We update ipending by ANDing it with ienable
		ctrl_reg[4] &= ctrl_reg[r];*/
	}
	// Check if we are writing to ipending
	else if(r == 4)
	{
		// Writing to ipending is a nop
		
		/*// We are writing to ipending. We must make sure we don't enable some bits in 
		// ipending that are 0 in ienable.
		// This is done by ANDing the data with ienable
		ctrl_reg[r] = data & ctrl_reg[3];*/
	}
	else
	{
		ctrl_reg[r] = data;
	}
}

/*
 *	CCpu::SignExtend()
 *
 *  Sign extends a number
 *
 *  Paramters:	num - The number to sign extend
 *  Paramters:	bits - The number of bits that make up the number num
 *
 *	Returns:	The sign extended number
 */
UINT CCpu::SignExtend(UINT num, UINT bits)
{	
	/*if(((num >> (bits-1)) & 0x1) == 0)
		return num;

	return (0xFFFFFFFF << bits) | num;*/
	return ((int)(num << (32-bits))) >> (32-bits);
}

/*
 *	CCpu::IssueException()
 *
 *  Issues an exception
 *
 *  Paramters:	old_pc - The value of the pc that are to be written to r29 (ea)
 */
void CCpu::IssueException(UINT old_pc)
{
	// Copy status to estatus
	SetCtrlReg(1, ctrl_reg[0]);
	// Clear the PIE bit
	SetCtrlReg(0, ctrl_reg[0] &= 0xFFFFFFFE);
	// Write the old PC to reg29 (ea)
	reg[29] = old_pc;
	// Write the exception address to pc
	UpdatePC(exception_addr);
}

/*
 *	CCpu::UpdatePC()
 *
 *  Updates the PC
 *
 *  Paramters:	new_pc - The new value of the PC
 */

void CCpu::UpdatePC(UINT new_pc)
{
	pc = new_pc;
}


/*
 *	CCpu::MemLoad()
 *
 *  Loads data from memory
 *
 *  Paramters:	addr - The address of the data
 *				size - The size of the data in bits
 *				io - A boolean. True means bypass any caches. False means look in caches.
 *				fetch - A boolean. True means this is an instruction fetch load. False means a normal load.
 *
 *	Returns:	The data retrieved at address addr
 */
UINT CCpu::MemLoad(UINT addr, UINT size, bool io, bool fetch)
{
	// Call the Read function of main_system
	return main_system.Read(addr, size, io, fetch);
}

/*
 *	CCpu::MemLoad()
 *
 *  Stores data from memory
 *
 *  Paramters:	addr - The address of the data
 *				size - The size of the data in bits
 *				data - The data to write
 *				io - A boolean. True means bypass any caches. False means look in caches.
 */
void CCpu::MemStore(UINT addr, UINT size, UINT data, bool io)
{
	// Call the Write function of main_system
	main_system.Write(addr, size, data, io);
}

/*
 *	CCpu::AssertIRQ()
 *
 *  Signals that a hardware interrupt has been generated.
 *
 *  Paramters:	irq - The IRQ number of the device
 */
void CCpu::AssertIRQ(UINT irq)
{
	pending_irq |= 1 << irq;
	ctrl_reg[4] = pending_irq & ctrl_reg[3];
	
	/*UINT tmp = 1;

	// Set the corresponding bit in the ipending control register to 1
	for(UINT i=0; i<irq; i++)
		tmp <<= 1;

	// Check if interrupts from this irq are enabled
	if(ctrl_reg[3] & tmp)
		// Update ipending
		SetCtrlReg(4, ctrl_reg[4] |= tmp);*/
}

/*
 *	CCpu::AssertIRQ()
 *
 *  Stops signaling that a hardware interrupt has been generated.
 *
 *  Paramters:	irq - The IRQ number of the device
 */
void CCpu::DeassertIRQ(UINT irq)
{
	pending_irq &= ~(1 << irq);
	ctrl_reg[4] = pending_irq & ctrl_reg[3];
	/*UINT tmp = 1;

	// Set the corresponding bit in the ipending control register to 0
	for(UINT i=0; i<irq; i++)
		tmp <<= 1;

	// Check if interrupts from this irq are enabled
	if(ctrl_reg[3] & tmp)
		// Update ipending
		SetCtrlReg(4, ctrl_reg[4] &= ~tmp);*/
}

/*
 *	CCpu::ShowMisalignedMemError()
 *
 *  Displays an error message for misaligned addresses
 *
 *  Paramters:	addr - The address of the data
 *				size - The size of the data in bits
 *				data - The data to write
 *				read - True if it was a read transfer and false if it was a write transfer
 *				update_pc - True if the PC should be subtracted by 4
 */
void CCpu::ShowMisalignedMemError(UINT addr, UINT size, UINT data, bool read, bool update_pc)
{
	StopError err;
	
	// Reset the pc to the current instruction
	if(update_pc)
		UpdatePC(pc-4);

	// Set up the correct error message
	if(read)
		sprintf(err.msg, "Error!\nMisaligned memory address 0x%.8X. Unable to read %d bit data.\nPC: 0x%.8X\n\nPausing simulation", addr, size, pc);
	else
		sprintf(err.msg, "Error!\nMisaligned memory address 0x%.8X. Unable to write %d bit data 0x%X.\nPC: 0x%.8X\n\nPausing simulation", addr, size, data, pc);
	
	// Display the error
	//MessageBox(hWnd, err, "Error", MB_ICONERROR);
	throw err;
}

/*
 *	CCpu::ShowInvalidMemAddressError()
 *
 *  Displays an error message for invalid memory addresses
 *
 *  Paramters:	addr - The address of the data
 *				size - The size of the data in bits
 *				data - The data to write
 *				read - True if it was a read transfer and false if it was a write transfer
 *				update_pc - True if the PC should be subtracted by 4
 */
void CCpu::ShowInvalidMemAddressError(UINT addr, UINT size, UINT data, bool read, bool update_pc)
{
	StopError err;
	
	// Reset the pc to the current instruction
	if(update_pc)
		UpdatePC(pc-4);

	// Set up the correct error message
	if(read)
		sprintf(err.msg, "Error!\nInvalid memory address 0x%.8X. Unable to read %d bit data.\nPC: 0x%.8X\n\nPausing simulation", addr, size, pc);
	else
		sprintf(err.msg, "Error!\nInvalid memory address 0x%.8X. Unable to write %d bit data 0x%X.\nPC: 0x%.8X\n\nPausing simulation", addr, size, data, pc);
	
	// Display the error
	//MessageBox(hWnd, err, "Error", MB_ICONERROR);
	throw err;
}
