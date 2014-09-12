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
#include "CLcd.h"
#include "sim.h"
using namespace std;

/*
 *	CLcd::CLcd()
 *
 *  Constructor for the CLcd class.
 *  Initializes all private variables to 0.
 */
CLcd::CLcd()
{
	name[0] = '\0';
	base = span = 0;

	memset(text, 0, LCD_TEXT_LEN);
	cursor = 0;

	mapped_board = NULL;
}

/*
 *	CLcd::Reset()
 *
 *  Performs a reset by clearing the text buffer and setting the cursor to 0
 */
void CLcd::Reset()
{
	memset(text, 0, LCD_TEXT_LEN);
	cursor = 0;
}

/*
 *	CLcd::Read()
 *
 *  Performs a read operation from an address mapped to the LCD
 *
 *	Parameters: addr - The address to read from
 *				size - Size in bits of the returned data (8, 16 or 32)
 *
 *	Returns:	The data retrieved at address addr
 */
UINT CLcd::Read(UINT addr, UINT size)
{
	UINT data;

	data = 0;
	// Instruction register
	if(addr == base)
	{
		
	}
	// Data register
	else if(addr == (base + 4))
	{

	}

	return data;
}

/*
 *	CLcd::Write()
 *
 *  Performs a write operation to an address mapped to the LCD
 *
 *	Parameters: addr - The address to write to
 *				size - Size in bits of the written data (8, 16 or 32)
 *				d    - The data to write
 */
void CLcd::Write(UINT addr, UINT size, UINT d)
{
	// Instruction register
	if(addr == base)
	{
		if(d == 1)
		{
			memset(text, 0, LCD_TEXT_LEN);
			cursor = 0;

			if(mapped_board)
				mapped_board->UpdateLCDText(text);
		}
		else if(d == 0x80)
		{
			cursor = 0;
		}
		else if(d == 0xC0)
		{
			cursor = LCD_TEXT_LEN/2;
		}
	}
	// Data register
	else if(addr == (base + 4))
	{
		text[cursor] = d;
		cursor++;
		if(cursor == LCD_TEXT_LEN)
			cursor = 0;

		if(mapped_board)
			mapped_board->UpdateLCDText(text);
	}
}

/*
 *	CLcd::SetBoard()
 *
 *  Maps a board class to the lcd
 *
 *	Parameters:	b - A pointer to the board
 */
void CLcd::SetBoard(CBoard *b)
{
	mapped_board = b;
}