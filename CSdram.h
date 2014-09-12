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

#ifndef _CSDRAM_H_
#define _CSDRAM_H_

#include <vector>

#include "MMDevice.h"

using namespace std;

class CSdram : public MMDevice
{
private:
	// Structure for an initial data block
	struct InitData
	{
		UINT addr;		// Base address of the data
		UINT size;		// Size in bytes of the data
		UCHAR *data;	// Pointer to the data
	};

	UCHAR *data;		// Pointer to the memory data
	vector<InitData> init_datas;	// List of initial data blocks
public:
	CSdram();
	~CSdram();
	
	void SetSpan(UINT s);

	void Reset();
	UINT Read(UINT addr, UINT size);
	void Write(UINT addr, UINT size, UINT d);

	void CleanUpInitData();

	void LoadData(char *buf, UINT addr, UINT size);
};

#endif
