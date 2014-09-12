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

#ifndef _CBOARDDEVICE_H_
#define _CBOARDDEVICE_H_

//#include <windows.h>
#include <string>
using namespace std;

class CBoardDeviceGroup;

class CBoardDevice
{
private:
	string type;		// Type of device
	string group;		// Name of the device group this device is a member of
	string image_file;	// Name of the image file for this device
	
	CBoardDeviceGroup *device_group;

	UINT bit;			// Start bit in the device group this device is mapped to
	UINT bit_span;		// Span of bits for this device (1 for buttons and leds, 7 for ssleds)

	POINT coords;		// Position of the device on the board

	GtkViewport *viewport;
	UINT bitmap_x, bitmap_y;	// Coords of the subbitmap inside the bitmap
	UINT width, height;			// Width and height of the bitmap
	
public:
	CBoardDevice();
	~CBoardDevice();

	const char *GetType() { return type.c_str(); };
	void SetType(const char *t) {type = t; };

	const char *GetGroup() { return group.c_str(); };
	void SetGroup(const char *g) {group = g; };

	UINT GetBit() { return bit; };
	void SetBit(UINT b) {bit = b; };

	UINT GetBitSpan() { return bit_span; };
	void SetBitSpan(UINT b) {bit_span = b; };

	const char *GetImageFile() { return image_file.c_str(); };
	void SetImageFile(const char *i) {image_file = i; };

	POINT GetCoords() { return coords; };
	void SetCoords(POINT p) {coords.x = p.x; coords.y = p.y; };

	bool Init(GtkFixed *board_area, CBoardDeviceGroup *device_group);
	//void Draw(HDC hDC, POINT pos);
	//RECT GetRect();
	void Click();
	void ReleaseClick();
	
	void SetData(UINT data);
	UINT GetData() {return bitmap_x;};
	
	void ShowCorrectImage();
};

#endif