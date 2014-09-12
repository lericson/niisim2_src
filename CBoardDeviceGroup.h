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

#ifndef _CBOARDDG_H_
#define _CBOARDDG_H_

#include <string>
using namespace std;
#include "CPio.h"

class CBoardDevice;

class CPio;

class CBoardDeviceGroup
{
private:
	string name;		// Name of the device group
	string type;		// Type of the device group ("in"|"out")

	bool is_pio;		// Set to true if this device group is mapped to a pio interface

	CPio *mapped_pio;	// Pointer to the pio interface this device group is mapped to

	vector<CBoardDevice*> devices;		// List of devices in this device group
public:
	CBoardDeviceGroup();
	~CBoardDeviceGroup();

	void CleanUp();
	bool Init(GtkFixed *board_area);
	//void Draw(HDC hDC, POINT pos);
	void ShowCorrectImages();
	bool AddDevice(string t, string g, UINT b, string i, POINT c);
	bool HasPoint(UINT x, UINT y, UINT *bit);
	void Click(UINT bit);
	void ReleaseClick(UINT bit);
	void UpdatedByClick(CBoardDevice *device);

	const char *GetName() { return name.c_str(); };
	void SetName(const char *n) {name = n; };

	const char *GetType() { return type.c_str(); };
	void SetType(const char *t) {type = t; };

	bool IsPIO() {return is_pio;};
	void SetPIO(bool p) {is_pio = p;};

	void SetPIOInterface(CPio *p) {mapped_pio = p;};

	void SetData(UINT data);
	UINT GetData();
};

#endif