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

#ifndef _CTIMER_H_
#define _CTIMER_H_

#include "MMDevice.h"

class CTimer : public MMDevice
{
private:
	UINT irq;			// The IRQ number
	bool has_irq;		// True if the interface has an IRQ number
	UINT init_period;	// The initial period of the timer

	// Internal registers
	UINT period;		// The current period
	UINT frequency;		// The current frequency
	bool fixed_period, always_run, has_snapshot, counting;
	UINT counter, snapshot;
	UINT TO, RUN, ITO, CONT;
public:
	CTimer();
	~CTimer() {};

	void Reset();
	UINT Read(UINT addr, UINT size);
	void Write(UINT addr, UINT size, UINT d);

	void OnClock();
	bool IsCounting() { return counting; };

	void SetIRQ(UINT i) { irq = i; has_irq = true; };
	bool HasIRQ() { return has_irq; };
	UINT GetIRQ() { return irq; };

	void SetPeriod(UINT p) { init_period = period = p; };
	UINT GetPeriod() { return period; };

	void SetFrequency(UINT f) { frequency = f; };
	UINT GetFrequency() { return frequency; };

	void SetFixedPeriod(bool p) { fixed_period = p; };
	bool GetFixedPeriod() { return fixed_period; };

	void SetAlwaysRun(bool p) { always_run = p; };
	bool GetAlwaysRun() { return always_run; };

	void SetHasSnapshot(bool p) { has_snapshot = p; };
	bool GetHasSnapshot() { return has_snapshot; };
};

#endif