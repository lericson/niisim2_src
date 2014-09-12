/*
NIISim - Nios II Simulator, A simulator that is capable of simulating various systems containing Nios II cpus.
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

/*

This file implements a Nios2 disassembler.

It takes a 32-bit instruction at an addres and transforms it into the Disasm
data structure, which can be written to a string with the DumpDisasm function.

*/

#include "CCpu.h"

typedef unsigned int uint;
typedef unsigned long long int uint64;


enum
{						// Examples
	OT_null,
	OT_reg,				// r1
	OT_custom_reg,		// c2
	OT_ctl_reg,			// ctl3
	OT_byte_offset,		// -4(r5)	value2=-4, value=5
	OT_int,				// 6
	OT_address			// 0x800004
};

struct Operand
{
	short type;
	short value2;
	int value;
};

struct Disasm
{
	const char *instruction;
	Operand ops[4];
};

static const char* i_instructions[] = {
	"call", "jmpi", NULL, "ldbu", "addi", "stb", "br", "ldb", "cmpgei", NULL, NULL, "ldhu", "andi", "sth", "bge", "ldh",
	"cmplti", NULL, NULL, "initda", "ori", "stw", "blt", "ldw", "cmpnei", NULL, NULL, "flushda", "xori", NULL, "bne", NULL,
	"cmpeqi", NULL, NULL, "ldbuio", "muli", "stbio", "beq", "ldbio", "cmpgeui", NULL, NULL, "ldhuio", "andhi", "sthio", "bgeu", "ldhio",
	"cmpltui", NULL, "custom", "initd", "orhi", "stwio", "bltu", "ldwio", "rdprs", NULL, "R", "flushd", "xorhi", NULL, NULL, NULL
};

static const char* r_instructions[] = {
	NULL, "eret", "roli", "rol", "flushp", "ret", "nor", "mulxuu", "cmpge", "bret", NULL, "ror", "flushi", "jmp", "and", NULL,
	"cmplt", NULL, "slli", "sll", "wrprs", NULL, "or", "mulxsu", "cmpne", NULL, "srli", "srl", "nextpc", "callr", "xor", "mulxss",
	"cmpeq", NULL, NULL, NULL, "divu", "div", "rdctl", "mul", "cmpgeu", "initi", NULL, NULL, NULL, "trap", "wrctl", NULL,
	"cmpltu", "add", NULL, NULL, "break", NULL, "sync", NULL, NULL, "sub", "srai", "sra", NULL, NULL, NULL, NULL
};

static const char* reg_names[32] = {
	"r0", "at", "r2",  "r3",
	"r4", "r5", "r6", "r7",
	"r8", "r9", "r10", "r11",
	"r12", "r13", "r14", "r15",
	"r16", "r17", "r18", "r19",
	"r20", "r21", "r22", "r23",
	"et", "bt", "gp", "sp",
	"fp", "ea", "ba", "ra"
};

static const char* ctrl_reg_names[32] = {
	"status", "estatus", "bstatus", "ienable",
	"ipending", "cpuid", "ctl6", "exception",
	"pteaddr", "tlbacc", "tlbnisc", "ctl11",
	"badaddr", "config", "mpubase", "mpuacc",
	"ctl16", "ctl17", "ctl18", "ctl19",
	"ctl20", "ctl21", "ctl22", "ctl23",
	"ctl24", "ctl25", "ctl26", "ctl27",
	"ctl28", "ctl29", "ctl30", "ctl31"
};

static Operand make_op(short type){
	Operand op;
	op.type = type;
	return op;
}
static Operand make_op(short type, int value)
{
	Operand op;
	op.type = type, op.value = value;
	return op;
}
static Operand make_op(short type, int value, short value2)
{
	Operand op = {type, value2, value};
	return op;
}

static Disasm make_disasm(const char *instruction)
{
	Disasm disasm;
	disasm.instruction = instruction;
	disasm.ops[0] = disasm.ops[1] = disasm.ops[2] = disasm.ops[3] = make_op(OT_null);
	return disasm;
}
static Disasm make_disasm(const char *instruction, Operand op1)
{
	Disasm disasm;
	disasm.instruction = instruction;
	disasm.ops[0] = op1;
	disasm.ops[1] = disasm.ops[2] = disasm.ops[3] = make_op(OT_null);
	return disasm;
}
static Disasm make_disasm(const char *instruction, Operand op1, Operand op2)
{
	Disasm disasm;
	disasm.instruction = instruction;
	disasm.ops[0] = op1;
	disasm.ops[1] = op2;
	disasm.ops[2] = disasm.ops[3] = make_op(OT_null);
	return disasm;
}
static Disasm make_disasm(const char *instruction, Operand op1, Operand op2, Operand op3)
{
	Disasm disasm;
	disasm.instruction = instruction;
	disasm.ops[0] = op1;
	disasm.ops[1] = op2;
	disasm.ops[2] = op3;
	disasm.ops[3] = make_op(OT_null);
	return disasm;
}
static Disasm make_disasm(const char *instruction, Operand op1, Operand op2, Operand op3, Operand op4)
{
	Disasm disasm;
	disasm.instruction = instruction;
	disasm.ops[0] = op1;
	disasm.ops[1] = op2;
	disasm.ops[2] = op3;
	disasm.ops[3] = op4;
	return disasm;
}

#define FOUND_ILLEGAL return make_disasm("")

/*
 *  DecompileInstruction()
 *
 *  Transform a 32-bit instruction, at a certain address, into a Disasm data structure.
 */
Disasm DecompileInstruction(uint pc, uint instr)
{
	uint OP = instr & 0x3F;
	uint OPX_1 = (instr >> 11) & 0x3F;
	uint OPX_2 = (instr >> 6) & 0x1F;
	uint IMM26 = (instr >> 6) & 0x3FFFFFF;
	uint IMM16 = (instr >> 6) & 0xFFFF;
	uint rA = (instr >> 27) & 0x1F;
	uint rB = (instr >> 22) & 0x1F;
	uint rC = (instr >> 17) & 0x1F;
	
	if(i_instructions[OP] == NULL)
		FOUND_ILLEGAL;
	
	
	if(OP == INSTR_R_TYPE)
	{
		const char *inst = r_instructions[OPX_1];
		
		const uint64 ABC0 = 0b0000101000000011000000011011000111001001110010010100100111001000ULL;
		const uint64 A0CIMM5 = (1ULL<<INSTR_R_ROLI) | (1ULL<<INSTR_R_SLLI) | (1ULL<<INSTR_R_SRLI) | (1ULL<<INSTR_R_SRAI);
		if((1ULL<<OPX_1) & ABC0)
		{
			if(OPX_2 != 0)
				FOUND_ILLEGAL;
			// Instruction is r_instructions[OPX_1] rC, rA, rB
			return make_disasm(inst, make_op(OT_reg, rC), make_op(OT_reg, rA), make_op(OT_reg, rB));
		}
		else if((1ULL<<OPX_1) & A0CIMM5)
		{
			if(rB != 0)
				FOUND_ILLEGAL;
			// Instruction is r_instructions[OPX_1] rC, rA, OPX_2
			return make_disasm(inst, make_op(OT_reg, rC), make_op(OT_reg, rA), make_op(OT_int, OPX_2));
		}
		else
		{
			switch(OPX_1)
			{
				case INSTR_R_ERET:
					if(rA != 0x1d || rB != 0x1e || rC != 0 || OPX_2 != 0)
						FOUND_ILLEGAL;
					// Instruction is r_instructions[OPX_1]
					return make_disasm(inst);
					break;
				case INSTR_R_FLUSHP:
				case INSTR_R_SYNC:
					if((rA | rB | rC | OPX_2) != 0)
						FOUND_ILLEGAL;
					// Instruction is r_instructions[OPX_1]
					return make_disasm(inst);
					break;
				case INSTR_R_RET:
					if(rA != 0x1f || (rB | rC | OPX_2) != 0)
						FOUND_ILLEGAL;
					// Instruction is r_instructions[OPX_1]
					return make_disasm(inst);
					break;
				case INSTR_R_BRET:
					if(rA != 0x1e || (rB | rC | OPX_2) != 0)
						FOUND_ILLEGAL;
					// Instruction is r_instructions[OPX_1]
					return make_disasm(inst);
					break;
				case INSTR_R_FLUSHI:
				case INSTR_R_JMP:
				case INSTR_R_INITI:
					if((rB | rC | OPX_2) != 0)
						FOUND_ILLEGAL;
					// Instruction is r_instruction[OPX_1] rA
					return make_disasm(inst, make_op(OT_reg, rA));
					break;
				case INSTR_R_WRPRS:
					if((rB | OPX_2) != 0)
						FOUND_ILLEGAL;
					// Instruction is r_instruction[OPX_1] rC, rA
					return make_disasm(inst, make_op(OT_reg, rC), make_op(OT_reg, rA));
					break;
				case INSTR_R_NEXTPC:
					if((rA | rB | OPX_2) != 0)
						FOUND_ILLEGAL;
					// Instruction is r_instruction[OPX_1] rC
					return make_disasm(inst, make_op(OT_reg, rC));
					break;
				case INSTR_R_CALLR:
					if((rB | OPX_2) != 0 || rC != 0x1f)
						FOUND_ILLEGAL;
					// Instruction is r_instruction[OPX_1] rA
					return make_disasm(inst, make_op(OT_reg, rA));
					break;
				case INSTR_R_RDCTL:
					if((rA | rB) != 0)
						FOUND_ILLEGAL;
					// Instruction is r_instruction[OPX_1] rC, ctlOPX_2
					return make_disasm(inst, make_op(OT_reg, rC), make_op(OT_ctl_reg, OPX_2));
					break;
				case INSTR_R_TRAP:
					if((rA | rB) != 0 || rC != 0x1d)
						FOUND_ILLEGAL;
					if(OPX_2 != 0)
						// Instruction is r_instruction[OPX_1] OPX_2
						return make_disasm(inst, make_op(OT_int, OPX_2));
					else
						// Instruction is r_instruction[OPX_1]
						return make_disasm(inst);
					break;
				case INSTR_R_WRCTL:
					if((rB | rC) != 0)
						FOUND_ILLEGAL;
					// Instruction is r_instruction[OPX_1] ctlOPX_2, rA
					return make_disasm(inst, make_op(OT_ctl_reg, OPX_2), make_op(OT_reg, rA));
					break;
				case INSTR_R_BREAK:
					if((rA | rB) != 0 || rC != 0x1e)
						FOUND_ILLEGAL;
					if(OPX_2 != 0)
						// Instruction is r_instruction[OPX_1] OPX_2
						return make_disasm(inst, make_op(OT_int, OPX_2));
					else
						// Instruction is r_instruction[OPX_1]
						return make_disasm(inst);
					break;
				default:
					FOUND_ILLEGAL;
			}
		}
		
		
	}
	else
	{
		const char *inst = i_instructions[OP];
		
		const uint64 store_load = 0b0000000010100000101010001010100000000000101000001010100010101000ULL;
		if(OP == INSTR_CALL || OP == INSTR_JMPI)
		{
			// Instruction is i_instructions[OP] IMM26
			return make_disasm(inst, make_op(OT_address, IMM26<<2));
		}
		else if(OP == INSTR_CUSTOM)
		{
			bool a = (instr>>16) & 1;
			bool b = (instr>>15) & 1;
			bool c = (instr>>14) & 1;
			uint N = (instr>>6) & 0xff;
			
			return make_disasm(inst, make_op(OT_int, N),
				make_op(c ? OT_reg : OT_custom_reg, rC),
				make_op(a ? OT_reg : OT_custom_reg, rA),
				make_op(b ? OT_reg : OT_custom_reg, rB));
		}
		else if((OP & 3) == 0 && OP != INSTR_CALL)
		{
			// Instruction is i_instructions[OP] rB, rA, IMM16
			return make_disasm(inst, make_op(OT_reg, rB), make_op(OT_reg, rA), make_op(OT_int, (short)IMM16));
		}
		else if((1ULL<<OP) & store_load)
		{
			// Instruction is i_instructions[OP] rB, IMM16(rA)
			return make_disasm(inst, make_op(OT_reg, rB), make_op(OT_byte_offset, rA, (short)IMM16));
		}
		else if((OP & 0x07) == 0x06)
		{
			// Branch instruction
			// FIXME should we check 4-byte alignment?
			uint addr = pc + 4 + (short)IMM16;
			if(OP == INSTR_BR)
			{
				if((rA | rB) != 0)
					FOUND_ILLEGAL;
				// Instruction is i_instructions[OP] label
				return make_disasm(inst, make_op(OT_address, addr));
			}
			else
			{
				// Instruction is i_instructions[OP] rA, rB, label
				return make_disasm(inst, make_op(OT_reg, rA), make_op(OT_reg, rB), make_op(OT_address, addr));
			}
		}
		else
		{
			if(rB != 0)
				FOUND_ILLEGAL;
			// Instruction is i_instructions[OP] IMM16(rA)
			return make_disasm(inst, make_op(OT_byte_offset, rA, (short)IMM16));
		}
	}
}

void dump_operand(const Operand& op)
{
	int value = op.value;
	short value2 = op.value2;
	switch(op.type)
	{
		case OT_null: return;
		case OT_reg: printf("r%d", value); return;
		case OT_custom_reg: printf("c%d", value); return;
		case OT_ctl_reg: printf("ctl%d", value); return;
		case OT_byte_offset: printf("%hd(r%d)", value2, value); return;
		case OT_int: printf("%d", value); return;
		case OT_address: printf("%p", (void*)value); return;
	}
}

void dump_disasm(const Disasm& disasm)
{
	printf("%s", disasm.instruction);
	for(int i=0; i<4; i++)
	{
		if(disasm.ops[i].type != OT_null)
		{
			if(i != 0)
				putchar(',');
			putchar(' ');
			dump_operand(disasm.ops[i]);
		}
	}
	putchar('\n');
}

static char* str_append(char *dst, const char *src)
{
	for(;;)
	{
		if((*dst = *src) == '\0')
			break;
		dst++, src++;
	}
	return dst;
}

/*
 *  DumpDisasm()
 *
 *  Writes a Disasm to a string representation in a buffer that is returned.
 *  The content of that buffer will be valid until the next call to this function.
 */
const char *DumpDisasm(const Disasm& disasm)
{
	static char buffer[32];
	char *ptr = buffer + sprintf(buffer, "%s", disasm.instruction);
	for(int i=0; i<4; i++)
	{
		const Operand& op = disasm.ops[i];
		if(op.type != OT_null)
		{
			if(i != 0)
				*ptr++ = ',';
			*ptr++ = ' ';
			
			int value = op.value;
			short value2 = op.value2;
			switch(op.type)
			{
				case OT_null: break;
				case OT_reg: ptr = str_append(ptr, reg_names[value]); break;
				case OT_custom_reg: ptr += sprintf(ptr, "c%d", value); break;
				case OT_ctl_reg: ptr = str_append(ptr, ctrl_reg_names[value]); break;
				case OT_byte_offset: ptr += sprintf(ptr, "%hd(%s)", value2, reg_names[value]); break;
				case OT_int: ptr += sprintf(ptr, "%d", value); break;
				case OT_address: ptr += sprintf(ptr, "%p", (void*)value); break;
			}
		}
	}
	return buffer;
}

/*int main(int argc, char *argv[]){
	void* instr;
	scanf("%p", &instr);
	puts(DumpDisasm(DecompileInstruction(0x800080, (int)(size_t)instr)));
}*/
