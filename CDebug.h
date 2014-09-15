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

#ifndef _CDEBUG_H_
#define _CDEBUG_H_

#include <vector>
#include <gtk/gtk.h>

using namespace std;

typedef unsigned int uint;

enum {
	CONTINUE,
	STEP_INTO,
	STEP_OVER,
	STEP_RETURN,
	STEP_INSTRUCTION
};

class CDebug
{
private:
	//vector<pair<uint, uint> > call_stack;
	int call_stack_size;
	vector<bool> all_source_breakpoints;
	vector<bool> address_has_breakpoint;
	uint memory_base_addr;
	int step_over_or_return_stack_frame;
	int debugging_state; // 0 == continue, 1 == step into, 2 == step over, 3 == step return, 4 == step instruction
	void ResumeSimulation(int debug_state, bool save_stack_frame);
	
public:
	CDebug() : call_stack_size(0), memory_base_addr(0), step_over_or_return_stack_frame(0), debugging_state(0) {}
	void Cleanup();
	
	void Init(GtkBuilder *builder);
	void SetMemoryInfo(uint base, uint span);
	void LoadELFFile(const char *filedata);
	void SetBreakpoint(uint addr){ address_has_breakpoint[addr - memory_base_addr].flip(); }
	void SetBreakpoints(const vector<uint>& addrs);
	bool AddressIsBreakpoint(uint addr){
		addr -= memory_base_addr;
		switch(debugging_state){
			case CONTINUE:
			case STEP_RETURN:
				return address_has_breakpoint[addr];
			case STEP_OVER:
				if(address_has_breakpoint[addr])
					return true;
				if(step_over_or_return_stack_frame < call_stack_size)
					return false;
				// Fallthrough
			case STEP_INTO:
				return all_source_breakpoints[addr] || address_has_breakpoint[addr];
			case STEP_INSTRUCTION:
				return true;
		}
	}
	void Break(uint addr);
	void BreakFromThread(uint addr);
	void EnterFunctionFromThread(uint pc, uint sp);
	void RetFromThread(uint pc, uint sp);
	
	void EnableButtons(void);
	void DisableButtons(void);
	
	static void StepInto(CDebug *self);
	static void StepOver(CDebug *self);
	static void StepReturn(CDebug *self);
	static void Continue(CDebug *self);
	static void StepInstruction(CDebug *self);
	
	void UpdateStatusbar(const char *text);
	void SetDebuggingState(int state);
	
	void SetCurrentExecutingLineMarks(uint addr, bool open_new_pages, bool show_page);
	void RemoveAllExecutingLineMarks(void);
};

#endif
