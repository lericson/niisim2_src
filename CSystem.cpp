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

#include <vector>
#include <iostream>
#include <fstream>
using namespace std;
#include <cstdio>
#include <cstdlib>
#include "sim.h"
#include "CSystem.h"

#include "sim.h"
#include "CCpu.h"
#include "CSdram.h"
#include "CTimer.h"
#include "CJtag.h"
#include "CUart.h"
#include "CPio.h"
#include "CLcd.h"

void* SimThreadFunc(void *data)
{
	//UINT clock_ticks;

	// Infinite loop
	while(1)
	{
		// Check if simulation is running and that it's not paused
		if(main_system.IsSimulationRunning() && !main_system.IsSimulationPaused())
		{
			// Execute one instruction
			main_system.Step();
			
			// Wait 2ms if we are running in "slow mode")
			if(main_system.GetSimulationSpeed() == SIM_SLOW)
			{
				/*__int64 freq, start, end;

				// Simulate a 2 ms delay
				QueryPerformanceFrequency((LARGE_INTEGER*)&freq); 
				QueryPerformanceCounter((LARGE_INTEGER*)&start);
				
				end = start;
				while(end - start < (freq/500.0))
				{
					// Yield
					Sleep(0);
					QueryPerformanceCounter((LARGE_INTEGER*)&end);
				}*/
				CThread::Sleep(2);
			}
			else
			{
//#ifdef TESTING
				/*// Yield after 500 clock cycles/instructions
				if(main_system.GetClk() - clock_ticks >= 500)
				{
					clock_ticks = main_system.GetClk();
					Sleep(0);
				}*/
//#else
//				Sleep(0);
//#endif
			}
		}
		else if(main_system.IsSimulationQuitting())
		{
			break;
		}
		else
		{
			// Wait 200 ms if the simulation isn't running. This will make sure the program
			// doesn't use 100% of the cpu all the time
			CThread::Sleep(200);
		}
	}
	return 0;
}

/*
 *	CSystem::CSystem()
 *
 *  Constructor for the CSystem class.
 *  Initializes all private variables.
 */
CSystem::CSystem()
{
	elf_loaded = false;
	sdf_loaded = false;

	clk = 0;

	sim_running = sim_paused = false;
	sim_quitting = false;
	sim_speed = SIM_FAST;

	/*threadHandle = CreateThread(
					NULL,                   // default security attributes
					0,                      // use default stack size  
					SimThreadFunc,       // thread function name
					NULL,          // argument to thread function 
					0,                      // use default creation flags 
					&threadID);   // returns the thread identifier */
	thread.init(SimThreadFunc);

	generating_trace = false;
	trace_file = NULL;
}

/*
 *	CSystem::~CSystem()
 *
 *  Destructor for the CSystem class.
 *  Calls the CleanUp() function when the class object is deleted.
 *	Also closes the simulation thread
 */
CSystem::~CSystem()
{
	// Cleap up all loaded interfaces
	CleanUp();

	// Close the simulation thread if it exists
	/*if(threadHandle)
	{
		// If the simulation is running, stop it
		if(!sim_paused)
			SuspendThread(threadHandle);

		CloseHandle(threadHandle);
	}*/
}

/*
 *	CSystem::Cleanup()
 *
 *  Deletes the memory of all allocated devices.
 *  Also stops generating trace file.
 */
void CSystem::CleanUp()
{
	// Clean up all allocated classes
	for(UINT i=0; i<cpus.size(); i++)
		delete cpus[i];
	cpus.clear();
	
	sdrams.clear();
	uarts.clear();
	timers.clear();
	
	for(UINT i=0; i<mm_devices.size(); i++)
		delete mm_devices[i];
	mm_devices.clear();
	
	// Clear all mapped devices
	mapped_jtag = NULL;

	// Stop generating trace file
	if(generating_trace)
		StopGenerateTraceFile();
}

/*
 *	CSystem::ParseCpu()
 *
 *  Parses an AddCpu command from the .sdf file
 *
 *	Parameters: args - A vector that contains the line with the AddCpu command
 *
 *	Returns:	True if the parsing was successful and false if an error occured.
 */
bool CSystem::ParseCpu(const ParsedRowArguments& args)
{
	CCpu *cpu;
	
	if(!ArgsMatches(args, "snnn"))
		return false;

	cpu = new CCpu;

	// Name
	cpu->SetName(args[0].second.c_str());
	// Reset address
	cpu->SetResetAddress(atoi(args[1].second.c_str()));
	// Exception address
	cpu->SetExceptionAddress(atoi(args[2].second.c_str()));
	// Frequency
	cpu->SetFrequency(atoi(args[3].second.c_str()));

	// Add the cpu to the system
	cpus.push_back(cpu);
	return true;
}

/*
 *	CSystem::ParseSdram()
 *
 *  Parses an AddSdram command from the .sdf file
 *
 *	Parameters: line - A C string that contails the line with the AddSdram command
 *
 *	Returns:	True if the parsing was successful and false if an error occured.
 */
bool CSystem::ParseSdram(const ParsedRowArguments& args)
{
	UINT base, span;
	CSdram *sdram;

	if(!ArgsMatches(args, "snn"))
		return false;

	sdram = new CSdram;

	// Name
	sdram->SetName(args[0].second.c_str());
	// Base address
	base = atoi(args[1].second.c_str());
	sdram->SetBaseAddress(base);
	// Span
	span = atoi(args[2].second.c_str());
	sdram->SetSpan(span);
	
	// Tell the debugger about the memory
	main_debug.SetMemoryInfo(base, span);

	// Add the sdram to the system
	sdrams.push_back(sdram);

	// Add the sdram to the list of memory mapped devices
	mm_devices.push_back(sdram);
	return true;
}

/*
 *	CSystem::ParseUart()
 *
 *  Parses an AddUart command from the .sdf file
 *
 *	Parameters: line - A C string that contails the line with the AddUart command
 *
 *	Returns:	True if the parsing was successful and false if an error occured.
 */
bool CSystem::ParseUart(const ParsedRowArguments& args)
{
	UINT base, span;
	CUart *uart;

	if(!ArgsMatches(args, "snnn?"))
		return false;

	uart = new CUart;

	// Name
	uart->SetName(args[0].second.c_str());
	// Base address
	base = atoi(args[1].second.c_str());
	uart->SetBaseAddress(base);
	// Span
	span = atoi(args[2].second.c_str());
	uart->SetSpan(span);
	
	if (args.size() > 3)
		// IRQ
		uart->SetIRQ(atoi(args[3].second.c_str()));

	// Add the uart interface to the system
	uarts.push_back(uart);

	// Add the uart interface to the list of memory mapped devices
	mm_devices.push_back(uart);
	return true;
}

/*
 *	CSystem::ParseJtag()
 *
 *  Parses an AddJtag command from the .sdf file
 *
 *	Parameters: line - A C string that contails the line with the AddJtag command
 *
 *	Returns:	True if the parsing was successful and false if an error occured.
 */
bool CSystem::ParseJtag(const ParsedRowArguments& args)
{
	UINT base, span;
	CJtag *jtag;

	if(!ArgsMatches(args, "snnn?"))
		return false;

	jtag = new CJtag;

	// Name
	jtag->SetName(args[0].second.c_str());
	// Base address
	base = atoi(args[1].second.c_str());
	jtag->SetBaseAddress(base);
	// Span
	span = atoi(args[2].second.c_str());
	jtag->SetSpan(span);
	
	if (args.size() > 3)
		// IRQ
		jtag->SetIRQ(atoi(args[3].second.c_str()));

	//jtag->Reset();
	mm_devices.push_back(jtag);
	return true;
}

/*
 *	CSystem::ParseLcd()
 *
 *  Parses an AddLcd command from the .sdf file
 *
 *	Parameters: line - A C string that contails the line with the AddLcd command
 *
 *	Returns:	True if the parsing was successful and false if an error occured.
 */
bool CSystem::ParseLcd(const ParsedRowArguments& args)
{
	UINT base, span;
	CLcd *lcd;

	if(!ArgsMatches(args, "snn"))
		return false;

	lcd = new CLcd;

	// Name
	lcd->SetName(args[0].second.c_str());
	// Base address
	base = atoi(args[1].second.c_str());
	lcd->SetBaseAddress(base);
	// Span
	span = atoi(args[2].second.c_str());
	lcd->SetSpan(span);

	//lcd->Reset();

	mm_devices.push_back(lcd);
	return true;
}

/*
 *	CSystem::ParseTimer()
 *
 *  Parses an AddTimer command from the .sdf file
 *
 *	Parameters: line - A C string that contails the line with the AddTimer command
 *
 *	Returns:	True if the parsing was successful and false if an error occured.
 */
bool CSystem::ParseTimer(const ParsedRowArguments& args)
{
	UINT base, span;
	CTimer *timer;
	UINT period;

	if(!ArgsMatches(args, "snnnnsnnnn?"))
		return false;

	timer = new CTimer;

	// Name
	timer->SetName(args[0].second.c_str());
	// Base address
	base = atoi(args[1].second.c_str());
	timer->SetBaseAddress(base);
	// Span
	span = atoi(args[2].second.c_str());
	timer->SetSpan(span);
	// Frequency
	timer->SetFrequency(atoi(args[3].second.c_str()));
	// Period
	period = atoi(args[4].second.c_str());
	// Period unit
	if(args[5].second == "ms")
	{
		UINT freq = timer->GetFrequency();
		timer->SetPeriod(period*(freq/1000));
	}
	else if(args[5].second == "us")
	{
		UINT freq = timer->GetFrequency();
		timer->SetPeriod(period*(freq/1000000));
	}
	else
	{
		delete timer;
		return false;
	}
	// Fixed period
	timer->SetFixedPeriod(atoi(args[6].second.c_str()) != 0);
	// Always run
	timer->SetAlwaysRun(atoi(args[7].second.c_str()) != 0);
	// Has snapshot
	timer->SetHasSnapshot(atoi(args[8].second.c_str()) != 0);
	if(args.size() > 9)
		// IRQ
		timer->SetIRQ(atoi(args[9].second.c_str()));

	//timer->Reset();
	timers.push_back(timer);

	mm_devices.push_back(timer);
	return true;
}

/*
 *	CSystem::ParsePio()
 *
 *  Parses an AddPio command from the .sdf file
 *
 *	Parameters: line - A C string that contails the line with the AddPio command
 *
 *	Returns:	True if the parsing was successful and false if an error occured.
 */
bool CSystem::ParsePio(const ParsedRowArguments& args)
{
	UINT base, span;
	CPio *pio;

	if (!ArgsMatches(args, "snnsn?"))
		return false;

	pio = new CPio;

	// Name
	pio->SetName(args[0].second.c_str());
	// Base address
	base = atoi(args[1].second.c_str());
	pio->SetBaseAddress(base);
	// Span
	span = atoi(args[2].second.c_str());
	pio->SetSpan(span);
	// Type
	pio->SetType(args[3].second.c_str());
	
	// Delimeter
	if (args.size() > 4)
		// IRQ
		pio->SetIRQ(atoi(args[4].second.c_str()));


	//pio->Reset();

	mm_devices.push_back(pio);
	return true;
}

/*
 *	CSystem::ParseImportBoard()
 *
 *  Parses an ImportBoard command from the .sdf file
 *
 *	Parameters: line - A C string that contails the line with the ImportBoard command
 *
 *	Returns:	True if the parsing was successful and false if an error occured.
 */
bool CSystem::ParseImportBoard(const ParsedRowArguments& args)
{
	char filepath[256];

	if(!ArgsMatches(args, "s"))
		return false;
	
	// Name
	strcpy(filepath, args[0].second.c_str());

	// Load the board file
	main_board.LoadBoard(filepath);
	return true;
}

/*
 *	CSystem::ParseMap()
 *
 *  Parses a Map command from the .sdf file
 *
 *	Parameters: line - A C string that contails the line with the Map command
 *
 *	Returns:	True if the parsing was successful and false if an error occured.
 */
bool CSystem::ParseMap(const ParsedRowArguments& args)
{
	if(!ArgsMatches(args, "ss"))
		return false;
	
	const string& name = args[0].second;
	const string& board_identifier = args[1].second;
	
	for(UINT i=0; i<mm_devices.size(); i++)
	{
		if(name == mm_devices[i]->GetName())
		{
			if(board_identifier == "JTAG") if(CJtag *jtag = dynamic_cast<CJtag*>(mm_devices[i]))
			{
				// Save a pointer to the jtag interface
				mapped_jtag = jtag;
				// Map the jtag interface to the jtag console
				jtag->SetConsole(&jtag_console);

				return true;
			}
			if(board_identifier == "UART0") if(CUart *uart = dynamic_cast<CUart*>(mm_devices[i]))
			{
				// Save a pointer to the uart interface
				mapped_uart0 = uart;
				// Map the jtag interface to the uart0 console
				uart->SetConsole(&uart0_console);

				return true;
			}
			if(board_identifier == "UART1") if(CUart *uart = dynamic_cast<CUart*>(mm_devices[i]))
			{
				// Save a pointer to the uart interface
				mapped_uart1 = uart;
				// Map the jtag interface to the uart1 console
				uart->SetConsole(&uart1_console);

				return true;
			}
			// Check if the identifier is the name of an LCD device on the board
			if(board_identifier == main_board.GetLCDName()) if(CLcd *lcd = dynamic_cast<CLcd*>(mm_devices[i]))
			{
				// Map the lcd interface to the board console
				lcd->SetBoard(&main_board);
				
				return true;
			}
		}
	}

	// Look for the identifier in the list of board device groups
	CBoardDeviceGroup *device_group = main_board.GetDeviceGroup(board_identifier.c_str());

	// Return false if no device group was found
	if(!device_group)
	{
		puts(main_board.GetLCDName());
		return false;
	}

	// Check if the device group can be mapped to a pio interface
	if(device_group->IsPIO())
	{
		// Go through all pio interfaces to fine the one with a name that matches
		for(UINT i=0; i<mm_devices.size(); i++)
		{
			if(name == mm_devices[i]->GetName()) if(CPio *pio = dynamic_cast<CPio*>(mm_devices[i]))
			{
				// Type must match
				if(!strcmp(pio->GetType(), device_group->GetType()))
				{
					// Save a pointer to the pio interface in the device group
					device_group->SetPIOInterface(pio);
					// Map the pio interface to the device group
					pio->SetBoardDeviceGroup(device_group);

					return true;
				}
			}
		}
	}

	// Couldn't map, return false
	return false;
}

/*
 *	CSystem::LoadSystemDescriptionFile()
 *
 *  Loads an .sdf file.
 *
 *	Parameters: file - Filepath to the .sdf file
 *
 *	Throws: ParsingSyntaxError, ParsingError
 */
void CSystem::LoadSystemDescriptionFile(const char *file)
{
	char err_str[1024];

	// Stop the simulation if it is running
	if(sim_running)
		StopSimulation();

	// Delete the current system description
	CleanUp();
	// Delete the board
	main_board.CleanUp();
	elf_loaded = false;
	sdf_loaded = false;
	
	ParsedFile sdf_file = ParseFile(file);
	
	for(ParsedFile::iterator it = sdf_file.begin(); it != sdf_file.end(); ++it)
	{
		static const char *commands[] = {"AddCPU", "AddSDRAM", "AddUART", "AddJTAG", "AddLCD", "AddTimer", "AddPIO", "ImportBoard", "Map"};
		static bool (CSystem::*functions[])(const ParsedRowArguments&) = {&CSystem::ParseCpu, &CSystem::ParseSdram, &CSystem::ParseUart, &CSystem::ParseJtag, &CSystem::ParseLcd, &CSystem::ParseTimer, &CSystem::ParsePio, &CSystem::ParseImportBoard, &CSystem::ParseMap};
		
		for(int i=0; i<sizeof(commands)/sizeof(const char*); i++)
		{
			if(it->first == commands[i])
			{
				if(!(this->*functions[i])(it->second))
				{
					sprintf(err_str, "Error while parsing \'%s\'", commands[i]);
					throw ParsingError(string(err_str));
				}
			}
		}
	}

	// The .sdf file has been loaded successfully
	sdf_loaded = true;
}

/*
 *	CSystem::IsAddressValid()
 *
 *  Checks if and address is valid by looking if any device is mappped to the address.
 *
 *	Parameters: addr - The address to check
 *
 *	Returns:	True if the address was valid, otherwise false
 */
bool CSystem::IsAddressValid(UINT addr)
{
	MMDevice *mmd;

	// Loop through all memory mapped devices
	for(UINT i=0; i<mm_devices.size(); i++)
	{
		mmd = mm_devices[i];
		// Check if the address is in range
		if(addr >= mmd->GetBaseAddress() && addr < (mmd->GetBaseAddress() + mmd->GetSpan()))
		{
			// If it is, break and return true to signal that the address is valid
			return true;
		}
	}

	return false;
}

/*
 *	CSystem::Read()
 *
 *  Reads data from memory
 *
 *  Paramters:	addr - The address of the data
 *				size - The size of the data in bits
 *				io - A boolean. True means bypass any caches. False means look in caches.
 *				fetch - A boolean. True means this is an instruction fetch load. False means a normal load.
 *
 *	Returns:	The data retrieved at address addr
 */
UINT CSystem::Read(UINT addr, UINT size, bool io, bool fetch)
{
	MMDevice *mmd;

	// Are we generating a trace file and is io false?
	if(generating_trace && !io)
	{
		// Set up a load entry in the trace file
		int type = TRACE_FILE_LOAD;

		// Change to an instruction fetch if fetch was true
		if(fetch)
			type = TRACE_FILE_FETCH;

		// Print the entry to the trace file
		fprintf(trace_file, "%d %.8X\n", type, addr);
	}

	// Loop through all memory mapped devices
	for(UINT i=0; i<mm_devices.size(); i++)
	{
		mmd = mm_devices[i];
		// Check if the address is in range
		if(addr >= mmd->GetBaseAddress() && addr < (mmd->GetBaseAddress() + mmd->GetSpan()))
		{
			return mmd->Read(addr, size);
		}
	}

	// No device was found, return 0
	return 0;
}

/*
 *	CSystem::Write()
 *
 *  Writes data to memory
 *
 *  Paramters:	addr - The address of the data
 *				size - The size of the data in bits
 *				d - The data to write
 *				io - A boolean. True means bypass any caches. False means look in caches.
 *
 *	Returns:	The data retrieved at address addr
 */
void CSystem::Write(UINT addr, UINT size, UINT d, bool io)
{
	MMDevice *mmd;

	// Are we generating a trace file and is io false?
	if(generating_trace && !io)
	{
		// Set up a store entry in the trace file
		int type = TRACE_FILE_STORE;

		// Print the entry to the trace file
		fprintf(trace_file, "%d %.8X\n", type, addr);
	}

	// Loop through all memory mapped devices
	for(UINT i=0; i<mm_devices.size(); i++)
	{
		mmd = mm_devices[i];
		// Check if the address is in range
		if(addr >= mmd->GetBaseAddress() && addr < (mmd->GetBaseAddress() + mmd->GetSpan()))
		{
			mmd->Write(addr, size, d);
			break;
		}
	}
}

/*
 *	CSystem::LoadELFFile()
 *
 *  Loads an .elf file.
 *
 *	Parameters: file - Filepath to the .elf file
 *
 *	Throws: LoadELFFileError
 */
void CSystem::LoadELFFile(const char *file)
{
	FILE *f;
	size_t file_size;
	Elf32_Ehdr header;
	Elf32_Phdr p_header;
	vector<char> buf;
	const char *err_str;

	// Stop the simulation if it is running
	if(sim_running)
		StopSimulation();

	// Display and error message if no .sdf file has been loaded
	if(!sdf_loaded)
	{
		throw LoadELFFileError("Unable to load an .elf file before the system description file (.sdf) has been loaded!");
	}

	// Clean up old initialization data
	for(UINT i=0; i<sdrams.size(); i++)
		sdrams[i]->CleanUpInitData();

	// Stop generating trace file
	if(generating_trace)
		StopGenerateTraceFile();

	elf_loaded = false;

	// Open the .elf file
	f = fopen(file, "rb");

	// Return false if unable to open
	if(!f)
		throw LoadELFFileError("Could not open the .elf file.");
	
	// Safely close the file on exiting the function or throwing an exception
	struct OnExit {
		FILE *f;
		~OnExit() { fclose(f); }
	} _; _.f = f;

	// Read the header
	fread(&header, sizeof(Elf32_Ehdr), 1, f);

	// Check for magic number
	if(!(header.e_ident[EI_MAG0] == ELFMAG0 && header.e_ident[EI_MAG1] == ELFMAG1 &&
	     header.e_ident[EI_MAG2] == ELFMAG2 && header.e_ident[EI_MAG3] == ELFMAG3))
	{
		throw LoadELFFileError("Invalid .elf file!");
	}

	// Check for correct class
	if(header.e_ident[EI_CLASS] != ELFCLASS32)
	{
		throw LoadELFFileError("Invalid file class! Must be 32bit.");
	}

	// Check for correct data
	if(header.e_ident[EI_DATA] != ELFDATA2LSB)
	{
		throw LoadELFFileError("Invalid file encoding!");
	}

	// Check for correct version
	if(header.e_ident[EI_VERSION] != EV_CURRENT)
	{
		throw LoadELFFileError("Invalid file version!");
	}

	// Check for correct type
	if(header.e_type != ET_EXEC)
	{
		throw LoadELFFileError("Invalid file type! Must be an executable .elf file.");
	}

	// Check for NIOS2 machine code
	if(header.e_machine != EM_NIOS2)
	{
		throw LoadELFFileError("Invalid machine! Must be Nios II.");
	}

	// Check for correct version (again but another variable)
	if(header.e_version != EV_CURRENT)
	{
		throw LoadELFFileError("Invalid file version!");
	}

	// Check for correct header size
	if(header.e_phentsize != sizeof(Elf32_Phdr))
	{
		throw LoadELFFileError("Invalid program header size!");
	}

	// Loop through all program headers
	for(int i=0; i<header.e_phnum; i++)
	{
		// Read the program header from the file
		fseek(f, header.e_phoff + i*sizeof(Elf32_Phdr), SEEK_SET);
		fread(&p_header, sizeof(Elf32_Phdr), 1, f);

		// Check for correct type
		if(p_header.p_type != PT_LOAD)
		{
			throw LoadELFFileError("Invalid program header type!");
		}

		if(p_header.p_filesz > 0)
		{
			// Set up a buffer to hold the data
			buf.resize(p_header.p_filesz);

			// Read the data from the file
			fseek(f, p_header.p_offset, SEEK_SET);
			fread(&buf[0], 1, p_header.p_filesz, f);
		}

		// Copy the data to memory
		CopyDataToMemory(p_header.p_filesz > 0 ? &buf[0] : NULL, &p_header);
	}
	
	// Load debug information
	{
		// Seek to the end of the file
		fseek(f, 0, SEEK_END);
		
		// Get the file size
		file_size = ftell(f);
		
		// Rewind to the beginning
		rewind(f);
		
		// Read the whole file into memory and send it for debug parsing
		vector<char> whole_file(file_size);
		fread(&whole_file[0], 1, file_size, f);
		main_debug.LoadELFFile(&whole_file[0]);
	}

	// Assign PC to point to the entry point of the code
	cpus[0]->SetPC(header.e_entry);
	
	// Elf file was loaded successfully
	elf_loaded = true;
}

/*
 *	CSystem::CopyDataToMemory()
 *
 *  Copies data from an .elf file to an sdram device
 *
 *	Parameters: buf - A pointer to the data that is to be copied
 *			    p_header - A pointer to the program header that holds the address and size of the data
 *
 *	Throws: LoadELFFileError
 */
void CSystem::CopyDataToMemory(char *buf, Elf32_Phdr *p_header)
{
	CSdram *sdram;
	bool sdram_found = false;

	// If the size of the memory block is 0, there is nothing to copy so just return
	if(p_header->p_memsz == 0)
		return;

	// Loop through all sdrams
	for(UINT i=0; i<sdrams.size(); i++)
	{
		sdram = sdrams[i];

		// Check if the data fits into the sdram
		if(p_header->p_paddr >= sdram->GetBaseAddress() && 
		   (p_header->p_paddr + p_header->p_memsz) < (sdram->GetBaseAddress() + sdram->GetSpan()))
		{
			// Data fits, so copy it
			sdram->LoadData(buf, p_header->p_paddr, p_header->p_filesz);

			// Is the size of the file data less of than the size of the memory data?
			// If true it means the rest of the data in memory has to be filled up with zeroes.
			if(p_header->p_filesz < p_header->p_memsz)
			{
				char *zbuf;
				// Calculate the remaining size. (Size of memory data - size of file data)
				UINT size = p_header->p_memsz - p_header->p_filesz;

				// Set up a char buffer
				zbuf = new char[size];

				// Fill it with zeroes
				memset(zbuf, 0, size);

				// Copy the zeroes to the sdram
				sdram->LoadData(zbuf, p_header->p_paddr + p_header->p_filesz, size);

				delete [] zbuf;
			}

			// We found an sdram device in which the data could fit, 
			// so let's break out of the for loop
			sdram_found = true;
			break;
		}
	}
	
	// Check if an sdram device was found
	if(sdram_found == false)
	{
		char text[256];

		// No sdram device was found, display an error message and return false
		sprintf(text, "A segment of size %d bytes with base address 0x%.8X could not be placed in any SDRAM devices!", p_header->p_memsz, p_header->p_paddr);

		throw LoadELFFileError(text);
	}

	// Data was copied successfully
}

/*
 *	CSystem::Reset()
 *
 *  Performs a system reset by calling the Reset function for all devices.
 *	Also resets the clock count to 0
 */
void CSystem::Reset()
{	
	// Reset all devices
	for(UINT i=0; i<cpus.size(); i++)
		cpus[i]->Reset();
	for(UINT i=0; i<mm_devices.size(); i++)
		mm_devices[i]->Reset();

	// Reset the clock
	clk = 0;
}

/*
 *	CSystem::Step()
 *
 *  Performs a single simulation step by calling the OnClock() function for all CPUs and timers
 */
void CSystem::Step()
{
	// Call the OnClock function for all cpus
	for(UINT i=0; i<cpus.size(); i++)
		cpus[i]->OnClock();

	// Call the OnClock function for all timers
	for(UINT i=0; i<timers.size(); i++)
		timers[i]->OnClock();

	// Update the clock by 1
	clk++;
}

/*
 *	CSystem::AssertIRQ()
 *
 *  Asserts an IRQ signal
 *
 *	Parameters: irq - The IRQ number that is to be asserted
 */
void CSystem::AssertIRQ(UINT irq)
{
	// Assert the IRQ for all cpus
	for(UINT i=0; i<cpus.size(); i++)
		cpus[i]->AssertIRQ(irq);
}

/*
 *	CSystem::DeassertIRQ()
 *
 *  Deasserts an IRQ signal
 *
 *	Parameters: irq - The IRQ number that is to be deasserted
 */
void CSystem::DeassertIRQ(UINT irq)
{
	// Deassert the IRQ for all cpus
	for(UINT i=0; i<cpus.size(); i++)
		cpus[i]->DeassertIRQ(irq);
}

/*
 *	CSystem::StartSimulationThread()
 *
 *  Starts the simulation
 *
 *	Parameters: start_paused - True if the simulation is to be started in paused mode, otherwise false
 */
void CSystem::StartSimulationThread(bool start_paused)
{
	// Check if .sdf and .elf files are loaded
	if(elf_loaded && sdf_loaded) 
	{
		// If the simulation is not running, start it
		if(!sim_running)
		{
			sim_paused = start_paused;
			sim_running = true;
		}
	}
}

/*
 *	CSystem::PauseSimulationThread()
 *
 *  Pauses the simulation
 */
void CSystem::PauseSimulationThread()
{
	// Check if .sdf and .elf files are loaded
	if(elf_loaded && sdf_loaded) 
	{
		// If the simulation is running, pause it
		if(sim_running)
		{
			sim_paused = true;
		}
	}
}

/*
 *	CSystem::UnPauseSimulationThread()
 *
 *  Unpauses the simulation
 */
void CSystem::UnPauseSimulationThread()
{
	// Check if .sdf and .elf files are loaded
	if(elf_loaded && sdf_loaded) 
	{
		// If the simulation is running, unpause it
		if(sim_running && sim_paused)
		{
			sim_paused = false;
		}
	}
}

/*
 *	CSystem::StopSimulation()
 *
 *  Stops the simulation
 */
void CSystem::StopSimulation()
{
	// Check if .sdf and .elf files are loaded
	if(elf_loaded && sdf_loaded) 
	{
		// If the simulation is running, stop it
		if(sim_running)
		{
			sim_running = false;
			sim_paused = false;
		}
	}
}

/*
 *	CSystem::CloseSimulationThread()
 *
 *  Exits the simulation thread
 */
void CSystem::CloseSimulationThread()
{
	StopSimulation();
	sim_quitting = true;
	thread.join();
}

/*
 *	CSystem::SendInputToJTAG()
 *
 *  Sends input to the jtag interface that is mapped to JTAG
 *
 *	Parameters: text - The text which the user has typed in the console
 */
void CSystem::SendInputToJTAG(const string& text)
{
	// Check first if there is any interface mapped to JTAG
	if(mapped_jtag)
		mapped_jtag->SendInput(text);
}

/*
 *	CSystem::SendInputToUART0()
 *
 *  Sends input to the uart interface that is mapped to UART0
 *
 *	Parameters: text - The text which the user has typed in the console
 */
void CSystem::SendInputToUART0(const char *text)
{
	// Check first if there is any interface mapped to UART0
	if(mapped_uart0)
		mapped_uart0->SendInput(text);
}

/*
 *	CSystem::SendInputToUART1()
 *
 *  Sends input to the uart interface that is mapped to UART1
 *
 *	Parameters: text - The text which the user has typed in the console
 */
void CSystem::SendInputToUART1(const char *text)
{
	// Check first if there is any interface mapped to UART1
	if(mapped_uart1)
		mapped_uart1->SendInput(text);
}

/*
 *	CSystem::StartGenerateTraceFile()
 *
 *  Starts generating a trace file
 *
 *	Parameters: file - Full filepath to the dinero trace file
 */
void CSystem::StartGenerateTraceFile(const char *file)
{
	// Open the frace file
	trace_file = fopen(file, "w");
	
	// Return if the file could not be opened
	if(!trace_file)
		return;

	generating_trace = true;
}

/*
 *	CSystem::StopGenerateTraceFile()
 *
 *  Stops generating a trace file
 */
void CSystem::StopGenerateTraceFile()
{
	// Close the trace file if it is opened
	if(trace_file)
		fclose(trace_file);

	generating_trace = false;
}

