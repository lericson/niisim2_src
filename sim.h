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

#ifndef _SIM_H_
#define _SIM_H_

//#define TESTING		// Undefine this when building release and not testing

//#include <windows.h>
#include "types.h"
#include "CSystem.h"
#include "CConsole.h"
#include "CBoard.h"
#include "CDebug.h"

/*
void				InitRegisterListView();
void				UpdateRegisterWindow();
bool				GetTraceFilePath(char *file, int size);
void				ParseCommandLine(char *cmdline);
int					GetPathFromCommandLine(char *cmdline, string &path);
char*				GetNextToken(char *in, char *out, int *token_type);
bool				HexToInt(const char *hex, int *number);
BOOL				CreateChildWindows();
void				UpdateMenuBar();
void				UpdateToolbarButtons();
void				LoadFile(int type);
*/
//DWORD WINAPI		SimThreadFunc(LPVOID lpParam);
//DWORD WINAPI		UpdateConsolesFunc(LPVOID lpParam);
//DWORD WINAPI		IPSThreadFunc(LPVOID lpParam);

// External global variables
/*extern HINSTANCE hInst;
extern HWND hWnd, hWndToolbar;	// Main window and toolbar handles
extern HWND hBoard;				// I/O Board window handle
extern HWND hJTAGConsole;		// JTAG UART console window handle
extern HWND hUART0Console;		// UART0 console window handle
extern HWND hUART1Console;		// UART1 console window handle
extern HWND hRegisters;			// I/O Board registers window handle
extern HWND hCacheSim;			// Cache simulator window handle*/

void UpdateConsolesFunc(void);
void SetSensitiveButtons(bool run, bool pause, bool stop);

// Show a message dialog on the screen
void ShowErrorMessage(const char *msg);
// Same as ShowErrorMessage but can be used from the non-GUI thread
void ReportError(const char *msg);

extern GtkWidget* board_window;
extern GtkFixed*  board_area;
extern CDebug main_debug;

extern CSystem main_system;		// Handler to the system
extern CBoard main_board;		// Handler to the board
extern CConsole jtag_console;	// Handler to the jtag console
extern CConsole uart0_console;	// Handler to the uart0 console
extern CConsole uart1_console;	// Handler to the uart1 console

#endif
