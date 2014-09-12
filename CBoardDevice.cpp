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
#include "CBoardDevice.h"
#include "CBoardDeviceGroup.h"
#include "CFile.h"

static gboolean device_button_pressed(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	CBoardDevice *device = (CBoardDevice*)user_data;
	device->Click();
	
	return FALSE;
}
static gboolean device_button_released(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	CBoardDevice *device = (CBoardDevice*)user_data;
	device->ReleaseClick();
	
	return FALSE;
}

/*
 *	CBoardDevice::CBoardDevice()
 *
 *  Constructor of the CBoardDevice class
 *
 *  Initializes all private variables to 0.
 */
CBoardDevice::CBoardDevice()
{
	type = "";
	group = "";
	image_file = "";

	bit = bit_span = 0;

	coords.x = coords.y = 0;

	bitmap_x = bitmap_y = 0;
}

/*
 *	CBoardDevice::~CBoardDevice()
 *
 *  Destructor for the CBoardDevice class.
 */
CBoardDevice::~CBoardDevice()
{
}

/*
 *	CBoardDevice::Init()
 *
 *  Initializes the device
 *
 *	Returns:	True if the initializing was successful and false if it failed.
 */
bool CBoardDevice::Init(GtkFixed *board_area, CBoardDeviceGroup *device_group)
{
	this->device_group = device_group;

	// Load the bitmap
	GdkPixbuf *bg_pixbuf = gdk_pixbuf_new_from_stream(CFile(image_file.c_str()).get_input_stream(), NULL, NULL);
	if(bg_pixbuf == NULL)
		return false;

	bitmap_x = bitmap_y = 0;
	
	width = gdk_pixbuf_get_width(bg_pixbuf);
	height = gdk_pixbuf_get_height(bg_pixbuf);

	// Check if the type is LED, PUSH or TOGGLE
	if(!strcmp(type.c_str(), "LED") ||
	   !strcmp(type.c_str(), "PUSH") ||
	   !strcmp(type.c_str(), "TOGGLE"))
	{
		// Set the appropriate width and height of the bitmap
		width = width / 2;

		// Check if PUSH because pushbuttons have the value 1 if they are up
		if(!strcmp(type.c_str(), "PUSH"))
			bitmap_x = 1;
	}
	// Check if the type is SSLED
	else if(!strcmp(type.c_str(), "SSLED"))
	{
		// Set the appropriate width and height of the bitmap
		width = width / 8;
		height = height / 16;
	}
	
	GtkWidget *bg_viewport = gtk_viewport_new(NULL, NULL);
	gtk_widget_set_events(bg_viewport, GDK_BUTTON_RELEASE_MASK);
	g_signal_connect(G_OBJECT(bg_viewport), "button-press-event", G_CALLBACK(device_button_pressed), this);
	g_signal_connect(G_OBJECT(bg_viewport), "button-release-event", G_CALLBACK(device_button_released), this);
	gtk_widget_show(bg_viewport);
	
	GtkWidget *bg_image = gtk_image_new_from_pixbuf(bg_pixbuf);
	gtk_widget_show(bg_image);
	
	gtk_container_add((GtkContainer*)bg_viewport, bg_image);
	gtk_viewport_set_shadow_type((GtkViewport*)bg_viewport, GTK_SHADOW_NONE);
	gtk_widget_set_size_request(bg_viewport, width, height);
	gtk_adjustment_set_upper(gtk_viewport_get_hadjustment((GtkViewport*)bg_viewport), width);
	gtk_adjustment_set_upper(gtk_viewport_get_vadjustment((GtkViewport*)bg_viewport), height);
	gtk_fixed_put(board_area, bg_viewport, coords.x, coords.y);
	
	g_object_unref(bg_pixbuf);
	
	viewport = (GtkViewport*)bg_viewport;
	
	ShowCorrectImage();

	return true;
}

/*
 *	CBoardDevice::SetData()
 *
 *  Updates the bitmap when the PIO interface this device group is mapped to has changed its data
 *
 *	Parameters: data - The new data to this device from the PIO interface
 */
void CBoardDevice::SetData(UINT data)
{
	// Check if the type is LED
	if(!strcmp(type.c_str(), "LED"))
	{
		// Update the bitmap coordinates
		bitmap_x = data;
		bitmap_y = 0;
	}
	// Check if the type is SSLED
	else if(!strcmp(type.c_str(), "SSLED"))
	{
		// Update the bitmap coordinates
		bitmap_x = data & 0x7;
		bitmap_y = data >> 3;
	}
	else
	{
		return;
	}
	
	/*done_adjusting = false;
	g_idle_add(show_correct_image, this);
	while(!done_adjusting);*/
}

/*
 *	CBoardDevice::Click()
 *
 *  Processes a click action for this device
 */
void CBoardDevice::Click()
{
	// Check if the type is TOGGLE
	if(!strcmp(type.c_str(), "TOGGLE"))
	{
		// Toggle the bitmap
		if(bitmap_x == 0)
			bitmap_x = 1;
		else
			bitmap_x = 0;
	}
	// Check if the type is PUSH
	else if(!strcmp(type.c_str(), "PUSH"))
	{
		// Set the bitmap to the pushed down one
		bitmap_x = 0;
	}
	else
	{
		return;
	}
	
	ShowCorrectImage();
	device_group->UpdatedByClick(this);
}

/*
 *	CBoardDevice::ReleaseClick()
 *
 *  Processes a click action for this device
 */
void CBoardDevice::ReleaseClick()
{
	// Check if the type is PUSH
	if(!strcmp(type.c_str(), "PUSH"))
	{
		// Set the bitmap to the pushed up one
		bitmap_x = 1;
		ShowCorrectImage();
		device_group->UpdatedByClick(this);
	}
}

/*
 *  CBoardDevice::ShowCorrectImage()
 *
 *  Move the viewpoint to show the correct image.
 */
void CBoardDevice::ShowCorrectImage()
{
	gtk_adjustment_set_value(gtk_viewport_get_hadjustment(viewport), bitmap_x*width);
	gtk_adjustment_set_value(gtk_viewport_get_vadjustment(viewport), bitmap_y*height);
}
