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

#ifndef _CJTAG_H_
#define _CJTAG_H_

#include "MMDevice.h"

class CConsole;

class CJtag : public MMDevice
{
private:
	UINT irq;			// The IRQ number
	bool has_irq;		// True if the interface has an IRQ number

	// Internal registers for this interface
	UINT WE, RE, WI, RI, AC;
	UINT w_fifo, r_fifo;

	CConsole *c_console;	// Pointer to the console this interface is mapped to
	string buf;				// Text buffer with text that has been typed in from the console
	CMutex lock;			// Lock for the buffer
public:
	CJtag();
	~CJtag() {};

	void Reset();
	UINT Read(UINT addr, UINT size);
	void Write(UINT addr, UINT size, UINT d);

	void SetIRQ(UINT i) { irq = i; has_irq = true; };
	bool HasIRQ() { return has_irq; };
	UINT GetIRQ() { return irq; };

	void SetConsole(CConsole *c);
	void SendInput(const string& text);
};

#endif
