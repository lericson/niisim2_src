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

typedef unsigned int uint;

struct Disasm
{
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
	const char *instruction;
	Operand ops[4];
};

const char *DumpDisasm(const Disasm& disasm);
Disasm DecompileInstruction(uint pc, uint instr);
