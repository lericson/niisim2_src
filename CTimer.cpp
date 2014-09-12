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
#include "CTimer.h"

/*
 *	CTimer::CTimer()
 *
 *  Constructor for the CTimer class.
 *  Initializes all private variables to 0.
 */
CTimer::CTimer()
{
	name[0] = '\0';
	base = span = irq = 0;
	has_irq = false;
	period = init_period = 0;
	fixed_period = false;
	always_run = false;
	has_snapshot = false;
	
	snapshot = 0;
	TO = RUN = ITO = CONT = 0;
	counting = false;
	
	main_system.DeassertIRQ(irq);
}

/*
 *	CTimer::Reset()
 *
 *  Performs a reset
 */
void CTimer::Reset()
{
	// Clear snapshot and all control bits
	snapshot = 0;
	TO = RUN = ITO = CONT = 0;
	// Stop counting and reset counter to period
	period = init_period;
	counting = false;
	counter = period;
}

/*
 *	CTimer::Read()
 *
 *  Performs a read operation from an address mapped to the timer
 *
 *	Parameters: addr - The address to read from
 *				size - Size in bits of the returned data (8, 16 or 32)
 *
 *	Returns:	The data retrieved at address addr
 */
UINT CTimer::Read(UINT addr, UINT size)
{
	UINT data;

	data = 0;

	// status register
	if(addr == base)
	{
		data = (TO << 0) + (RUN << 1);
	}
	// control register
	else if(addr == (base+4))
	{
		data = (ITO << 0) + (CONT << 1);
	}
	// period_low register
	else if(addr == (base+8))
	{
		data = period & 0xFFFF;
	}
	// period_high register
	else if(addr == (base+12))
	{
		data = (period & 0xFFFF0000) >> 16;
	}
	// snapshot_low register
	else if(addr == (base+16))
	{
		if(has_snapshot)
			data = snapshot & 0xFFFF;
	}
	// snapshot_high register
	else if(addr == (base+20))
	{
		if(has_snapshot)
			data = (snapshot & 0xFFFF0000) >> 16;
	}

	return data;
}

/*
 *	CTimer::Write()
 *
 *  Performs a write operation to an address mapped to the timer
 *
 *	Parameters: addr - The address to write to
 *				size - Size in bits of the written data (8, 16 or 32)
 *				d    - The data to write
 */
void CTimer::Write(UINT addr, UINT size, UINT d)
{
	// status register
	if(addr == base)
	{
		// Write to status -> clear the TO bit
		TO = 0;
		// Deassert the IRQ
		if(has_irq)
			main_system.DeassertIRQ(irq);
	}
	// control register
	else if(addr == (base+4))
	{
		// ITO bit (interrupt on timeout)
		if(d & 0x1)
		{
			// Set to 1
			ITO = 1;
		}
		else
		{
			// Set to 0 and deassert the IRQ
			ITO = 0;
			if(has_irq)
				main_system.DeassertIRQ(irq);
		}

		// CONT bit
		if(d & 0x2)
			// Set to 1
			CONT = 1;
		else
			// Set to 0
			CONT = 0;

		// START bit
		if(d & 0x4)
		{
			// Set RUN to 1 to indicate that the timer is running
			RUN = 1;
			// Start the countdown
			counting = true;
		}

		// STOP bit
		if(d & 0x8)
		{
			// Check if this is an "always run" timer
			if(!always_run)
			{
				// If it isn't, stop the counting
				RUN = 0;
				counting = false;
			}
		}
	}
	// period_low register
	else if(addr == (base+8))
	{
		// Stop the counting
		counting = false;

		// Update the period unless it's a fixed one
		if(!fixed_period)
		{
			period = (period & 0xFFFF0000) + (d & 0xFFFF);
		}

		// Reset the counter
		counter = period;
	}
	// period_high register
	else if(addr == (base+12))
	{
		// Stop the counting
		counting = false;

		// Update the period unless it's a fixed one
		if(!fixed_period)
		{
			period = (period & 0x0000FFFF) + ((d & 0xFFFF) << 16);
		}

		// Reset the counter
		counter = period;
	}
	// snapshot registers
	else if((addr == (base+16)) ||
		    (addr == (base+20)))
	{
		// Copy counter to snapshot if there is a snapshot register available
		if(has_snapshot)
			snapshot = counter;
	}
}

/*
 *	CTimer::OnClock()
 *
 *  This function should get called every time the clock is updated by one.
 *	It handles the updating of the counter and resets it when it hits 0.
 *
 */
void CTimer::OnClock()
{
	// Return if we are not counting
	if(!counting)
		return;

	// Have we hit 0?
	if(counter == 0)
	{
		// If ITO is 1, generate an interrupt
		if(ITO && has_irq)
			main_system.AssertIRQ(irq);

		// Reset the counter
		counter = period;

		// Set timeout to 1
		TO = 1;
		// Stop the counting
		RUN = 0;
		counting = false;

		// If this is a continous timer or a "always run" timer, start the counting again
		if(always_run || CONT)
		{
			RUN = 1;
			counting = true;
		}
	}
	// The counter is not 0 so let's count down
	else
	{
		counter--;
	}
}