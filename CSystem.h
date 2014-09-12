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

#ifndef _CSYSTEM_H_
#define _CSYSTEM_H_

//#include <windows.h>
#include <vector>
#include <string>
using namespace std;
//#include "sim.h"
//#include "CCpu.h"
//#include "CSdram.h"
//#include "CTimer.h"
//#include "CJtag.h"
//#include "CUart.h"
//#include "CPio.h"
//#include "CLcd.h"
#include "CThread.h"
#include "fileparser.h"

// Constants for string parsing
#define TOKEN_UNKNOWN	0
#define TOKEN_KEYWORD	1
#define TOKEN_NUMBER	2
#define TOKEN_STRING	3
#define TOKEN_DELIMETER	4
#define TOKEN_EOL		5

// Constants defining the memory mapped device type
#define MM_SDRAM		0
#define MM_UART			1
#define MM_JTAG			2
#define MM_LCD			3
#define MM_TIMER		4
#define MM_PIO			5

#define EINIDENT 16

// Constants used for the elf file format
#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_PAD 7
#define EI_NIDENT 16

#define ELFMAG0  0x7F
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

#define ELFCLASSNONE 0
#define ELFCLASS32 1
#define ELFCLASS64 2

#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4
#define ET_LOPROC 0xff00
#define ET_HIPROC 0xffff

#define EM_NONE 0 // No machine
#define EM_M32 1 // AT&T WE 32100
#define EM_SPARC 2 // SPARC
#define EM_386 3 // Intel 80386
#define EM_68K 4 // Motorola 68000
#define EM_88K 5 // Motorola 88000
#define EM_860 7 // Intel 80860
#define EM_MIPS 8 // MIPS RS3000
#define EM_NIOS2 0x71 // Nios II

#define EV_NONE 0
#define EV_CURRENT 1

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_LOPROC 0x70000000
#define PT_HIPROC 0x7fffffff

// Constants defining the simulation speed
#define SIM_FAST 0
#define SIM_SLOW 1

// Constants for dinero trace file format
#define TRACE_FILE_LOAD  0
#define TRACE_FILE_STORE 1
#define TRACE_FILE_FETCH 2

class CCpu;
class CUart;
class CLcd;
class CTimer;
class CPio;
class CSdram;
class CJtag;

class MMDevice;

struct Elf32_Ehdr{
	unsigned char e_ident[EINIDENT];
	USHORT e_type;
	USHORT e_machine;
	UINT e_version;
	UINT e_entry;
	UINT e_phoff;
	UINT e_shoff;
	UINT e_flags;
	USHORT e_ehsize;
	USHORT e_phentsize;
	USHORT e_phnum;
	USHORT e_shentsize;
	USHORT e_shnum;
	USHORT e_shstrndx;
};

struct Elf32_Phdr{
	UINT p_type;
	UINT p_offset;
	UINT p_vaddr;
	UINT p_paddr;
	UINT p_filesz;
	UINT p_memsz;
	UINT p_flags;
	UINT p_align;
};

struct ParsingError
{
	string msg;
	ParsingError(const string& str) : msg(str) {}
};

struct LoadELFFileError
{
	string msg;
	LoadELFFileError(const string& str) : msg(str) {}
};

class CSystem
{
private:
	// Structure of a memory mapped device
	/*struct MMDevice
	{
		UINT base;	// Base address
		UINT span;	// Size in memory (in bytes)
		UINT type;	// Type of device
		union
		{
			CSdram *sdram;
			CUart *uart;
			CJtag *jtag;
			CLcd *lcd;
			CTimer *timer;
			CPio *pio;
		} c;	// Pointer to the device
	};*/

	vector<CCpu*> cpus;			// List of cpus in the system
	vector<CUart*> uarts;		// List of uarts in the system
	//vector<CLcd*> lcds;			// List of lcdss in the system
	vector<CTimer*> timers;		// List of timers in the system
	//vector<CPio*> pios;			// List of pios in the system
	vector<CSdram*> sdrams;		// List of sdrams in the system
	//vector<CJtag*> jtags;		// List of jtags in the system

	vector<MMDevice*> mm_devices;	// List of memory mapped devices in the system

	CJtag *mapped_jtag;			// Pointer to the jtag interface that is mapped to the jtag console
	CUart *mapped_uart0;		// Pointer to the uart interface that is mapped to the uart0 console
	CUart *mapped_uart1;		// Pointer to the uart interface that is mapped to the uart1 console

	bool elf_loaded;			// True if an elf file is loaded
	bool sdf_loaded;			// True if an sdf file is loaded

	UINT clk;					// The system clock

	CThread thread;				// Handle to the simulation thread
	bool sim_running, sim_paused;	// True if simulation is running and paused respectively
	bool sim_quitting;
	UINT sim_speed;				// The simulation speed

	bool generating_trace;		// True if a trace file is being generated
	FILE *trace_file;			// Handle to the trace file

	// Private functions used to parse the sdf file
	bool ParseCpu(const ParsedRowArguments& args);
	bool ParseSdram(const ParsedRowArguments& args);
	bool ParseUart(const ParsedRowArguments& args);
	bool ParseJtag(const ParsedRowArguments& args);
	bool ParseLcd(const ParsedRowArguments& args);
	bool ParseTimer(const ParsedRowArguments& args);
	bool ParsePio(const ParsedRowArguments& args);
	bool ParseMap(const ParsedRowArguments& args);
	bool ParseImportBoard(const ParsedRowArguments& args);

	void CopyDataToMemory(char *buf, Elf32_Phdr *p_header);
	void CleanUp();
public:
	CSystem();
	~CSystem();

	bool IsAddressValid(UINT addr);
	UINT Read(UINT addr, UINT size, bool io, bool fetch);
	void Write(UINT addr, UINT size, UINT d, bool io);
	void Reset();

	bool IsSimulationRunning() { return sim_running;};
	bool IsSimulationPaused() { return sim_paused;};
	bool IsSimulationQuitting() { return sim_quitting; }

	void LoadSystemDescriptionFile(const char *file);
	void LoadELFFile(const char *file);
	bool IsELFFileLoaded() { return elf_loaded;};

	void Step();
	void AssertIRQ(UINT irq);
	void DeassertIRQ(UINT irq);

	bool HasCPU() {return cpus.size() ? true : false;};
	CCpu *GetCPU(int cpu) {return cpus[cpu];};

	void StartSimulationThread(bool start_paused = false);
	void PauseSimulationThread();
	void UnPauseSimulationThread();
	void StopSimulation();
	void CloseSimulationThread();

	void SetSimulationSpeed(UINT speed) {sim_speed = speed;};
	UINT GetSimulationSpeed() {return *(volatile UINT*)&sim_speed;};

	bool IsGeneratingTraceFile() {return generating_trace;};
	void StartGenerateTraceFile(const char *file);
	void StopGenerateTraceFile();

	inline UINT GetClk() { return clk; };
	
	void SendInputToJTAG(const string& text);
	void SendInputToUART0(const char *text);
	void SendInputToUART1(const char *text);
};

#endif
