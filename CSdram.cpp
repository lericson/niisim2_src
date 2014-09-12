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

#include <string>
#include <cstdio>
#include <cstring>
#include "CSdram.h"
using namespace std;

/*
 *	CSdram::CSdram()
 *
 *  Constructor for the CSdram class.
 *  Initializes all private variables to 0.
 */
CSdram::CSdram()
{
	name[0] = '\0';
	base = span = 0;
	data = NULL;
}

/*
 *	CSdram::~CSdram()
 *
 *  Destructor for the CSdram class.
 *  Deletes the allocated memory data and performs some cleanups
 */
CSdram::~CSdram()
{
	// Is data allocated?
	if(data)
		// Delete it
		delete [] data;
	
	// Clean up initialized data
	CleanUpInitData();
}

/*
 *	CSdram::SetSpan()
 *
 *  Sets the size in bytes of the memory
 *
 *	Parameters: s - The size of the memory in bytes
 */
void CSdram::SetSpan(UINT s)
{
	span = s;

	// Is data allocated?
	if(data)
		// Delete it
		delete [] data;

	// Allocated new data with the new size
	data = new UCHAR[span];
}

/*
 *	CSdram::Reset()
 *
 *  Performs a reset by copying the init_data to the actual sdram data
 */
void CSdram::Reset()
{
	// Loop through all init data blocks
	for(UINT i=0; i<init_datas.size(); i++)
	{
		// Copy the init data to the sdram data
		memcpy(&(data[init_datas[i].addr]), init_datas[i].data, init_datas[i].size);
	}
}

/*
 *	CSdram::Read()
 *
 *  Performs a read operation from an address mapped to the SDram
 *
 *	Parameters: addr - The address to read from
 *				size - Size in bits of the returned data (8, 16 or 32)
 *
 *	Returns:	The data retrieved at address addr
 */
UINT CSdram::Read(UINT addr, UINT size)
{
	UINT p_addr;
	UINT ret = 0;

	// Calculate the offset in our data variable that holds the memory
	p_addr = addr - base;

	// Check if address is in range
	if(p_addr >= 0 && p_addr < base)
	{
		if(size == 8)
			ret = data[p_addr];
		else if(size == 16)
			ret = data[p_addr] + (data[p_addr+1] << 8);
		else if(size == 32)
			ret = data[p_addr] + (data[p_addr+1] << 8) + 
				 (data[p_addr+2] << 16) + (data[p_addr+3] << 24);
	}

	return ret;
}

/*
 *	CSdram::Write()
 *
 *  Performs a write operation to an address mapped to the Sdram
 *
 *	Parameters: addr - The address to write to
 *				size - Size in bits of the written data (8, 16 or 32)
 *				d    - The data to write
 */
void CSdram::Write(UINT addr, UINT size, UINT d)
{
	UINT p_addr;

	// Calculate the offset in our data variable that holds the memory
	p_addr = addr - base;

	// Check if it is in range
	if(p_addr >= 0 && p_addr < base)
	{		
		if(size == 8)
		{
			data[p_addr] = d & 0xFF;
		}
		else if(size == 16)
		{
			data[p_addr] = d & 0xFF;
			data[p_addr+1] = (d >> 8) & 0xFF;
		}
		else if(size == 32)
		{
			data[p_addr] = d & 0xFF;
			data[p_addr+1] = (d >> 8) & 0xFF;
			data[p_addr+2] = (d >> 16) & 0xFF;
			data[p_addr+3] = (d >> 24) & 0xFF;
		}
	}
}

/*
 *	CSdram::LoadData()
 *
 *  Loads predefined data to the sdram data. Also copies it into the init data blocks so
 *  that if the sdram is then reset, the sdram will be initialized correctly
 *
 *	Parameters: but - A pointer to the data buffer
 *				addr - The address of the data in the sdram
 *				size - Size in bytes of data
 */
void CSdram::LoadData(char *buf, UINT addr, UINT size)
{
	UINT p_addr;
	InitData id;

	// Calculate the offset in our data variable that holds the memory
	p_addr = addr - base;

	// Check if it is in range
	if(p_addr >= 0 && p_addr < base)
	{
		// Copy the data to the sdram
		memcpy(&(data[p_addr]), buf, size);

		// Save the data in our vector incase we need to reinitialize the sdram later on
		id.data = new UCHAR[size];
		id.addr = p_addr;
		id.size = size;
		// Copy the data to the init data block
		memcpy(id.data, buf, size);
		init_datas.push_back(id);
	}
}

/*
 *	CSdram::CleanUpInitData()
 *
 *  Clears all init data
 */
void CSdram::CleanUpInitData()
{
	// Loop through all init data blocks and delete them
	for(UINT i=0; i<init_datas.size(); i++)
		delete [] init_datas[i].data;

	init_datas.clear();
}
