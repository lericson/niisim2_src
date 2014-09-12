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

#ifndef _CPIO_H_
#define _CPIO_H_

#include <vector>
using namespace std;
#include "MMDevice.h"

class CBoardDeviceGroup;

class CPio : public MMDevice
{
private:
	UINT irq;			// The IRQ number
	bool has_irq;		// True if the interface has an IRQ number
	char type[8];		// Type of pio interface ("in" or "out")

	// Internal registers
	UINT data_reg, interrupt_mask_reg, edge_cap_reg;

	CBoardDeviceGroup *device_group;	// Pointer to the device group this pio is mapped to
public:
	CPio();
	~CPio() {};

	void Reset();
	UINT Read(UINT addr, UINT size);
	void Write(UINT addr, UINT size, UINT d);

	void SetIRQ(UINT i) { irq = i; has_irq = true; };
	bool HasIRQ() { return has_irq; };
	UINT GetIRQ() { return irq; };

	void SetType(const char *t) { strcpy(type,  t); };
	const char *GetType() { return type; };

	void SetBoardDeviceGroup(CBoardDeviceGroup *dev) {device_group = dev;};
	void UpdateData(UINT data, UINT bit);
};

#endif