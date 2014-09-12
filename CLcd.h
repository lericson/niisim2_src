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

#ifndef _CLCD_H_
#define _CLCD_H_

#include "MMDevice.h"

class CBoard;

#define LCD_TEXT_LEN 32
#define LCD_WIDTH 175
#define LCD_HEIGHT 40

class CLcd : public MMDevice
{
private:
	char text[LCD_TEXT_LEN];	// The text in the lcd window
	UINT cursor;				// Position of the cursor

	CBoard *mapped_board;		// Pointer to the board that this lcd interface is mapped to
public:
	CLcd();
	~CLcd() {};

	void Reset();
	UINT Read(UINT addr, UINT size);
	void Write(UINT addr, UINT size, UINT d);

	void SetBoard(CBoard *b);
};

#endif
