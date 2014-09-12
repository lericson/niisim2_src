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

#ifndef _MMDEVICE_H_
#define _MMDEVICE_H_

#include <cstring>
#include "types.h"

// Base class of a memory mapped device
class MMDevice
{
protected:
	char name[256];		// The name of the interface
	UINT base;			// Base address the interface is mapped to
	UINT span;			// Number of bytes the interface is mapped to
public:
	MMDevice() {}
	virtual ~MMDevice() {};

	virtual void Reset() = 0;
	virtual UINT Read(UINT addr, UINT size) = 0;
	virtual void Write(UINT addr, UINT size, UINT d) = 0;

	void SetName(const char *n) { strcpy(name,  n); };
	const char *GetName() const { return name; };

	void SetBaseAddress(UINT addr) { base = addr; };
	UINT GetBaseAddress() const { return base; };

	virtual void SetSpan(UINT s) { span = s; };
	UINT GetSpan() const { return span; };
};

#endif
