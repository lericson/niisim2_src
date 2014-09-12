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

#ifndef _CBOARD_H_
#define _CBOARD_H_

//#include <windows.h>
#include <string>
using namespace std;
#include "fileparser.h"
#include "CBoardDeviceGroup.h"
#include "CLcd.h"

struct InitBoardError
{
	string msg;
	InitBoardError(const string& str) : msg(str) {}
};

class CBoard
{
private:
	string bg_file;		// Name of the background file
	string name;		// Name of the board

	//HBITMAP bg_bitmap;	// Handler to the background bitmap

	POINT coords;		// Position of the board inside the window

	vector<CBoardDeviceGroup*> device_groups;		// List of all device groups on the board

	CBoardDeviceGroup *clicked_device_group;		// Pointer to the device group that was clicked
	UINT clicked_device_group_bit;					// The bit index in the device group that was clicked

	// Variables for the lcd control
	POINT lcd_coords;
	//HWND lcd_hWnd;
	GtkTextView *lcd_text_view;
	string lcd_name;
	bool lcd_available;
	//HFONT lcd_font;
	bool lcd_text_updated;
	string lcd_text_buf;

	// Private functions for the lcd control
	bool ParseDeviceGroup(const ParsedRowArguments& args);
	bool ParseDevice(const ParsedRowArguments& args);
	bool ParseLCD(const ParsedRowArguments& args);
public:
	CBoard();
	~CBoard();

	void CleanUp();

	void ShowCorrectImages(void);

	bool Init();
	void LoadBoard(char *file);
	void UpdateLCDText(char *text);

	const char*GetLCDName() {return lcd_name.c_str();};
	bool GetLCDTextUpdate() {return lcd_text_updated;};
	void WriteTextToLCD();

	CBoardDeviceGroup *GetDeviceGroup(const char *gname);
};

#endif
