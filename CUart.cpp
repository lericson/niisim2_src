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

#include <stdio.h>
#include "sim.h"
#include "CUart.h"

/*
 *	CUart::CUart()
 *
 *  Constructor for the CUart class.
 *  Initializes all private variables to 0.
 */
CUart::CUart()
{
	name[0] = '\0';
	base = span = irq = 0;
	has_irq = false;
	c_console = NULL;
	buf = "";

	ITRDY = IRRDY = RxR = TxR = 0;
	RxD = TxD = 0;
}

/*
 *	CUart::Reset()
 *
 *  Performs a reset.
 *  Initializes registers to 0 and empties the read buffer
 */
void CUart::Reset()
{
	ITRDY = 0;
	IRRDY = 0;
	RxR = 0;
	TxR = 1;
	RxD = TxD = 0;

	buf = "";
}

/*
 *	CUart::Read()
 *
 *  Performs a read operation from an address mapped to the uart
 *
 *	Parameters: addr - The address to read from
 *				size - Size in bits of the returned data (8, 16 or 32)
 *
 *	Returns:	The data retrieved at address addr
 */
UINT CUart::Read(UINT addr, UINT size)
{
	UINT data;

	data = 0;

	// RxData register
	if(addr == base)
	{
		lock.lock();
		
		// Mask out 1 byte of data from the read buffer
		data = RxD & 0xFF;

		// Check if the read buffer has more text
		if(buf.length() > 0)
		{
			// If it has more text, prepare the RxD register to hold the next char
			RxD = buf[0];
			// Set RxR to 1 to signal that there is data to read
			RxR = 1;

			// Remove the char from the text buffer
			buf = buf.substr(1);
		}
		else
		{
			//FIXME The original hardware seems to always return the last thing that was read.
			// If there was no more text, set RxD and RxR to 0 to signal that 
			// there is no more data to read
			RxD = 0;
			RxR = 0;
		}

		// Check if read interrupts are enabled and there is no new data to be read
		if(IRRDY && !RxR)
		{
			// Acknowledge the interrupt by deasserting the irq signal
			if(has_irq)
				main_system.DeassertIRQ(irq);
		}
		
		lock.unlock();
	}
	// TxData register
	else if(addr == (base+4))
	{
		// Cannot read from this reg
	}
	// Status register
	else if(addr == (base+8))
	{
		data = (TxR << 6) + (RxR << 7);
	}
	// Control register
	else if(addr == (base+12))
	{
		data = (ITRDY << 6) + (IRRDY << 7);
	}

	return data;
}

/*
 *	CUart::Write()
 *
 *  Performs a write operation to an address mapped to the uart
 *
 *	Parameters: addr - The address to write to
 *				size - Size in bits of the written data (8, 16 or 32)
 *				d    - The data to write
 */
void CUart::Write(UINT addr, UINT size, UINT d)
{
	char text[256];

	// RxData register
	if(addr == base)
	{
		// Cannot write to this reg
	}
	// TxData register
	else if(addr == (base+4))
	{
		// Mask out 1 byte from the data
		TxD = d & 0xFF;

		// Print the char to the uart console if there is one mapped to this class
		if(c_console)
		{
			sprintf(text, "%c", TxD);
			c_console->AddText(text, false);
		}

		// Dont set TxR to 0 since we handle it here immdiately
		// TxR = 0;

		// Check if write interrupts are enabled
		if(ITRDY)
		{
			// Acknowledge the interrupt by deasserting the irq signal
			if(has_irq)
				main_system.DeassertIRQ(irq);
		}
	}
	// Status register
	else if(addr == (base+8))
	{
		// Cannot write to this reg
	}
	// Control register
	else if(addr == (base+12))
	{
		// Check if ITRDY bit is 1
		if(d & 0x40)
		{
			// Enable write interrupts and assert the IRQ
			ITRDY = 1;
			if(has_irq)
				main_system.AssertIRQ(irq);
		}
		else
		{
			// Disable write interrupts and deassert the IRQ
			ITRDY = 0;
			if(has_irq)
				main_system.DeassertIRQ(irq);
		}

		// Check if IRRDY bit is 1
		if(d & 0x80)
		{
			// Enable read interrupts
			IRRDY = 1;
		}
		else
		{
			// Disable read interrupts
			IRRDY = 0;
		}
	}
}

/*
 *	CUart::SendInput()
 *
 *  Takes an input text stream from the uart console and prepares the read buffer
 *  so that the program can read in the text.
 *
 *	Parameters: text - The text to add to the read buffer
 */
void CUart::SendInput(const char *text)
{
	lock.lock();
	
	// Add the text to the read buffer
	buf += text;

	RxD = buf[0];	
	RxR = 1;

	buf = buf.substr(1);

	// Check if read interrupts are enabled
	if(IRRDY)
	{
		// Issue a read interrupt
		if(has_irq)
			main_system.AssertIRQ(irq);
	}
	
	lock.unlock();
}

/*
 *	CJtag::SetConsole()
 *
 *  Maps this jtag class to a jtag console class
 */
void CUart::SetConsole(CConsole *c)
{
	c_console = c;
}
