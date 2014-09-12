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
#include "CPio.h"

/*
 *	CPio::CPio()
 *
 *  Constructor for the CPio class.
 *  Initializes all private variables to 0.
 */
CPio::CPio()
{
	name[0] = '\0';
	base = span = irq = 0;
	has_irq = false;
	strcpy(type, "in");

	device_group = NULL;

	data_reg = 0;
	interrupt_mask_reg = 0;
	edge_cap_reg = 0;
}

/*
 *	CPio::Reset()
 *
 *  Performs a reset by setting the data, interrupt mask and edge cap registers to 0.
 *  Then if the PIO interface is mapped to a board device group that is of type "in", 
 *  retrieve the data and store it in the data register.
 */
void CPio::Reset()
{
	// Reset all registers
	data_reg = 0;
	interrupt_mask_reg = 0;
	edge_cap_reg = 0;

	// Check if a device group is mapped to this PIO
	if(device_group)
	{
		// Check if the type is "in"
		if(!strcmp(device_group->GetType(), "in"))
		{
			// Get the data
			data_reg = device_group->GetData();
		}
	}
}

/*
 *	CPio::Read()
 *
 *  Performs a read operation from an address mapped to the PIO
 *
 *	Parameters: addr - The address to read from
 *				size - Size in bits of the returned data (8, 16 or 32)
 *
 *	Returns:	The data retrieved at address addr
 */
UINT CPio::Read(UINT addr, UINT size)
{
	UINT data;

	data = 0;

	// Data register
	if(addr == base)
	{
		// Check if the type is "in"
		if(!strcmp(type, "in"))
		{
			data = data_reg;
		}
	}
	// Direction register
	else if(addr == (base + 4))
	{
		// Not implemented
	}
	// Interrupt mask register
	else if(addr == (base + 8))
	{
		data = interrupt_mask_reg;
	}
	// Edge capture register
	else if(addr == (base + 12))
	{
		data = edge_cap_reg;
	}

	return data;
}

/*
 *	CPio::Write()
 *
 *  Performs a write operation to an address mapped to the PIO
 *
 *	Parameters: addr - The address to write to
 *				size - Size in bits of the written data (8, 16 or 32)
 *				d    - The data to write
 */
void CPio::Write(UINT addr, UINT size, UINT d)
{
	//HDC hDC;

	// Data register
	if(addr == base)
	{
		// Check if type is "out"
		if(!strcmp(type, "out"))
		{
			// Store the new data in the data register
			data_reg = d;

			// Check if this PIO interface is mapped to a board device group
			if(device_group)
			{
				// Notify the device group that the data has been updates
				device_group->SetData(d);
				// Redraw the device group if the board window is visible
				/*if(IsWindowVisible(hBoard))
				{
					hDC = GetDC(hBoard);
					device_group->Draw(hDC, main_board.GetCoords());
					ReleaseDC(hBoard, hDC);
				}*/
				//FIXME
			}
		}
	}
	// Direction register
	else if(addr == (base + 4))
	{
		// Not implemented
	}
	// Interrupt mask register
	else if(addr == (base + 8))
	{
		interrupt_mask_reg = d;
	}
	// Edge capture register
	else if(addr == (base + 12))
	{
		// Set edge cap register to 0
		edge_cap_reg = 0;
		// Deassert any IRQ
		if(has_irq)
			main_system.DeassertIRQ(irq);
	}
}

/*
 *	CPio::UpdateData()
 *
 *  Signals the PIO interface that the data has been changed.
 *
 *	Parameters: data - The new data
 *				bit - The bit that was changed in the data register
 */
void CPio::UpdateData(UINT data, UINT bit)
{
	// Store the new data in the data register
	data_reg = data;
	// Update the edge cap regsiter accordingly
	edge_cap_reg |= (1 << bit);

	// Check if this bit can generate an interrupt
	if(interrupt_mask_reg & (1 << bit))
	{
		// Assert an interrupt
		if(has_irq)
			main_system.AssertIRQ(irq);
	}
}