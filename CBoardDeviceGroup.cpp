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

#include "sim.h"
#include "CPio.h"
#include "CBoardDevice.h"
#include "CBoardDeviceGroup.h"

/*
 *	CBoardDeviceGroup::CBoardDeviceGroup()
 *
 *  Constructor of the CBoardDeviceGroup class
 *
 *  Initializes all private variables to 0.
 */
CBoardDeviceGroup::CBoardDeviceGroup()
{
	name = "";
	type = "";
	is_pio = false;

	mapped_pio = NULL;
}

/*
 *	CBoardDeviceGroup::~CBoardDeviceGroup()
 *
 *  Destructor for the CBoardDeviceGroup class.
 *  Calls the CleanUp() function when the class object is deleted.
 */
CBoardDeviceGroup::~CBoardDeviceGroup()
{
	CleanUp();
}

/*
 *	CBoardDeviceGroup::CBoardDeviceGroup()
 *
 *  Deletes the memory of all allocated devices.
 */
void CBoardDeviceGroup::CleanUp()
{
	// Clean up all allocated classes
	for(UINT i=0; i<devices.size(); i++)
		delete devices[i];
	devices.clear();

	mapped_pio = NULL;
}

/*
 *	CBoardDeviceGroup::AddDevice()
 *
 *  Adds a new device to this device group class
 * 
 *	Parameters: t - The type of device
 *				g - The name of the device group
 *				b - The bit the device is mapped to
 *				i - The bitmap filename for the device
 *				c - The coordinates of the device bitmap
 *
 *	Returns:	True if the device was added, otherwise false
 */
bool CBoardDeviceGroup::AddDevice(string t, string g, UINT b, string i, POINT c)
{
	CBoardDevice *device;

	// Allocate memory for a new device
	device = new CBoardDevice;

	device->SetGroup(g.c_str());
	// Check for invalid device type
	if(strcmp(t.c_str(), "LED") && strcmp(t.c_str(), "SSLED") &&
	   strcmp(t.c_str(), "PUSH") && strcmp(t.c_str(), "TOGGLE"))
	{
		// Return false if there was an invalid type
		delete device;
		return false;
	}
	device->SetType(t.c_str());
	device->SetBit(b);
	// If the type is SSLED (a seven segment LED) set the bit span to 7
	if(!strcmp(t.c_str(), "SSLED"))
		device->SetBitSpan(7);
	// Otherwise set the bit span to 1
	else
		device->SetBitSpan(1);
	device->SetImageFile(i.c_str());
	device->SetCoords(c);

	// Add the device to our vector
	devices.push_back(device);

	return true;
}

/*
 *	CBoardDeviceGroup::Init()
 *
 *  Initializes the devices in this device group
 *
 *	Returns:	True if the initializing was successful, otherwise false
 */
bool CBoardDeviceGroup::Init(GtkFixed *board_area)
{
	// Loop through all devices and initialize them
	for(UINT i=0; i<devices.size(); i++)
	{
		// Return false if a device couldn't be initialized
		if(!devices[i]->Init(board_area, this))
			return false;
	}

	return true;
}

void CBoardDeviceGroup::ShowCorrectImages()
{
	for(UINT i=0; i<devices.size(); i++)
		devices[i]->ShowCorrectImage();
}

/*
 *	CBoardDeviceGroup::SetData()
 *
 *  Calls the SetData function of all devices in this device group.
 *  This function is called whenever the PIO interface this device group is mapped to 
 *  has changed its data register.
 *
 *	Parameters: data - The new data from the PIO interface
 */
void CBoardDeviceGroup::SetData(UINT data)
{
	UINT d;

	// Return if the device group is of type "out"
	if(strcmp(type.c_str(), "out"))
		return;
	
	// Loop through all devices
	for(UINT i=0; i<devices.size(); i++)
	{
		// Check for type LED
		if(!strcmp(devices[i]->GetType(), "LED"))
		{
			devices[i]->SetData((data >> devices[i]->GetBit()) & 0x1);
		}
		// Check for type SSLED
		else if(!strcmp(devices[i]->GetType(), "SSLED"))
		{
			devices[i]->SetData((data >> devices[i]->GetBit()) & 0x7F);
		}
	}

#if 0 // Slower algorithm:
	// Loop through all 32 bits in the 4 byte data register
	for(int i=0; i<32; i++)
	{
		d = data >> i;

		// Loop through all devices
		for(UINT j=0; j<devices.size(); j++)
		{
			// Check if the device is mapped to bit i
			if(devices[j]->GetBit() == i)
			{
				// Check for type LED
				if(!strcmp(devices[j]->GetType(), "LED"))
				{
					devices[j]->SetData(d & 0x1);
					break;
				}
				// Check for type SSLED
				else if(!strcmp(devices[j]->GetType(), "SSLED"))
				{
					devices[j]->SetData(d & 0x7F);
					// Increment i by 6 since an SSLED has a bit span of 7
					i+=6;
					break;
				}
			}
		}
	}
#endif
}

/*
 *	CBoardDeviceGroup::GetData()
 *
 *  Calls the GetData function of all devices in this device group.
 *  This function is called whenever the PIO interface this device group is mapped to
 *  tries to retrieve a new value from the toggle switches or push buttons in this device group
 *
 *	Returns:	The data from the device group
 */
UINT CBoardDeviceGroup::GetData()
{
	UINT data, bit_data;

	data = 0;

	// Check if the type is "in"
	if(!strcmp(type.c_str(), "in"))
	{
		// Loop through all devices and get their data
		for(UINT i=0; i<devices.size(); i++)
		{
			// Get the data
			bit_data = devices[i]->GetData();
			// Shift it accordingly
			bit_data <<= devices[i]->GetBit();
			// Add it to the data variable
			data |= bit_data;
		}
	}

	// Return the data
	return data;
}

/*
 *  CBoardDeviceGroup::UpdatedByClick()
 *
 *  When a CBoardDevice has been pressed or released by the user, notify the system about this.
 */
void CBoardDeviceGroup::UpdatedByClick(CBoardDevice *device)
{
	// Return if the type is not "in"
	if(strcmp(type.c_str(), "in"))
		return;
	
	// If a PIO interface if mapped to this device group, 
	// notify it that the data has been changed
	if(mapped_pio)
		mapped_pio->UpdateData(GetData(), device->GetBit());
}
