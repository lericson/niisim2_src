/*
NIISim - Nios II Simulator, A simulator that is capable of simulating various systems containing Nios II cpus.
Copyright (C) 2010,2011 Emil Bäckström

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
#include "CConsole.h"
#include "CJtag.h"

/*
 *	CJtag::CJtag()
 *
 *  Constructor for the CJtag class.
 *  Initializes all private variables to 0.
 *  Initializes control register bits to 0.
 *  Initializes FIFO read and write queues.
 */
CJtag::CJtag()
{
	name[0] = '\0';
	base = span = irq = 0;
	has_irq = false;
	c_console = NULL;
	buf = "";

	// Set control register bits to 0
	RE = 0;
	WE = 0;
	RI = 0;
	WI = 0;
	AC = 0;

	// Init FIFO queues
	w_fifo = 64;
	r_fifo = 0;
}

/*
 *	CJtag::Reset()
 *
 *  Performs a reset.
 *  Initializes control register bits to 0.
 *  Initializes FIFO read and write queues.
 */
void CJtag::Reset()
{
	buf = "";

	// Set control register bits to 0
	RE = 0;
	WE = 0;
	RI = 0;
	WI = 0;
	AC = 0;

	// Init FIFO queues
	w_fifo = 64;
	r_fifo = 0;
}

/*
 *	CJtag::Read()
 *
 *  Performs a read operation from an address mapped to the jtag uart
 *
 *	Parameters: addr - The address to read from
 *				size - Size in bits of the returned data (8, 16 or 32)
 *
 *	Returns:	The data retrieved at address addr
 */
UINT CJtag::Read(UINT addr, UINT size)
{
	UINT data;

	data = 0;

	// Data register
	if(addr == base)
	{
		lock.lock();
		
		// Do we have characters in the read FIFO?
		if(r_fifo > 0)
		{
			int r_fifo_send;

			// Read one character
			r_fifo--;

			// Make sure we say we have 63 characters left 
			// even if our character buffer has more chars.
			// This is because FIFO_SIZE is set to 64 in system.h
			r_fifo_send = r_fifo;
			if(r_fifo_send >= 63)
				r_fifo_send = 63;

			// Prepare the data
			data = ((buf[buf.length() - r_fifo - 1]) & 0xFF) + (1 << 15) + (r_fifo_send << 16);

			// Disable read interrupt
			RI = 0;
			if(has_irq)
				main_system.DeassertIRQ(irq);
		}
		else
		{
			// No characters in FIFO queue so return 0
			data = (0 << 15) + (r_fifo << 16);
		}
		
		lock.unlock();
	}
	// Status register
	else if(addr == (base + 4))
	{
		data = (RE << 0) + (WE << 1) + (RI << 8) + (WI << 9) + (AC << 10) + (w_fifo << 16);
	}

	return data;
}

/*
 *	CJtag::Write()
 *
 *  Performs a write operation to an address mapped to the jtag
 *
 *	Parameters: addr - The address to write to
 *				size - Size in bits of the written data (8, 16 or 32)
 *				d    - The data to write
 */
void CJtag::Write(UINT addr, UINT size, UINT d)
{
	char text[256];

	// Data register
	if(addr == base)
	{
		// If a console is mapped to this jtag class,
		// print the character to the console
		if(c_console)
		{
			sprintf(text, "%c", (d & 0xFF));
			c_console->AddText(text, false);
		}

		// Disable write interrupts
		WI = 0;
		if(has_irq)
			main_system.DeassertIRQ(irq);
	}
	// Control register
	else if(addr == (base + 4))
	{
		// Read interrupt enable
		if(d & 0x1)
		{
			// Enable read interrupts
			RE = 1;
		}
		else
		{
			// Disable read interrupts
			RE = 0;
		}

		// Write interrupt enable
		if(d & 0x2)
		{
			// Enable write interrupts
			WE = 1;

			// Issue a write interrupt immediately
			WI = 1;
			if(has_irq)
				main_system.AssertIRQ(irq);

			// Indicate JTAG activity
			AC = 1;
		}
		else
		{
			// Disable write interrupts
			WE = 0;
			// Disable any pending write interrupts
			WI = 0;
			if(has_irq)
				main_system.DeassertIRQ(irq);
		}

		// Check activity bit
		if(d & 0x400)
		{
			// Clear the activity bit
			AC = 0;
		}
	}
}

/*
 *	CJtag::SendInput()
 *
 *  Takes an input text stream from the jtag console and prepares the read FIFO 
 *  so that the program can read in the text.
 *
 *	Parameters: text - The text to add to the read buffer
 */
void CJtag::SendInput(const string& text)
{
	lock.lock();
	
	// Add the text to the read buffer
	buf = text;

	// Set the FIFO to match the length of the text string
	r_fifo = buf.length();

	// Check if read interrupts are enabled
	if(RE)
	{
		// Issue a read interrupt
		RI = 1;
		if(has_irq)
			main_system.AssertIRQ(irq);
	}
	// Indicate JTAG activity
	AC = 1;
	
	lock.unlock();
}

/*
 *	CJtag::SetConsole()
 *
 *  Maps this jtag class to a jtag console class
 */
void CJtag::SetConsole(CConsole *c)
{
	c_console = c;
}
