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

using namespace std;
#include <cstdio>
#include <cstdlib>
#include "sim.h"
#include "CBoard.h"
#include "CBoardDeviceGroup.h"
#include "CBoardDevice.h"
#include "CFile.h"

// Modifies the path so that it also work in Linux
static void fix_filename(string& str)
{
	for(size_t i=0; i<str.length(); i++)
		if(str[i] == '\\')
			str[i] = '/';
}

/*
 *	CBoard::CBoard()
 *
 *  Constructor for the CBoard class.
 *  Initializes all private variables.
 */
CBoard::CBoard()
{
	name = "";
	bg_file = "";

	coords.x = coords.y = 0;

	lcd_coords.x = lcd_coords.y = 0;
	//lcd_hWnd = NULL;
	lcd_name = "";
	lcd_available = false;
}

/*
 *	CBoard::~CBoard()
 *
 *  Destructor for the CBoard class.
 *  Calls the CleanUp() function when the class object is deleted.
 */
CBoard::~CBoard()
{
	CleanUp();
}

/*
 *	CBoard::Cleanup()
 *
 *  Deletes the memory of all allocated device groups.
 *  Also deletes the background bitmap, lcd control and lcd control.
 */
void CBoard::CleanUp()
{
	// Clean up all allocated classes
	for(UINT i=0; i<device_groups.size(); i++)
		delete device_groups[i];
	device_groups.clear();

	// No lcd available
	lcd_available = false;
	
	//FIXME remove all objects in GtkFixed
}

/*
 *	CBoard::ParseDeviceGroup()
 *
 *  Parses an AddDeviceGroup command from the .board file
 *
 *	Parameters: line - A C string that contails the line with the AddDeviceGroup command
 *
 *	Returns:	True if the parsing was successful and false if an error occured.
 */
bool CBoard::ParseDeviceGroup(const ParsedRowArguments& args)
{
	CBoardDeviceGroup *d_group;

	if(!ArgsMatches(args, "ss"))
		return false;

	// Allocate memory for a new device group
	d_group = new CBoardDeviceGroup;

	d_group->SetName(args[0].second.c_str());
	d_group->SetType(args[1].second.c_str());

	// Set this device group to be compatible with a PIO interface
	d_group->SetPIO(true);

	// Add the device group to our vector
	device_groups.push_back(d_group);

	return true;
}

/*
 *	CBoard::ParseDevice()
 *
 *  Parses an AddDevice command from the .board file
 *
 *	Parameters: line - A C string that contails the line with the AddDevice command
 *
 *	Returns:	True if the parsing was successful and false if an error occured.
 */
bool CBoard::ParseDevice(const ParsedRowArguments& args)
{
	string type, group, image;
	POINT coords;
	UINT bit;

	if(!ArgsMatches(args, "ssnsnn"))
		return false;

	// Type
	type = args[0].second;
	// Device group name
	group = args[1].second;
	// Bit
	bit = atoi(args[2].second.c_str());
	// Image filename
	image = args[3].second;
	fix_filename(image);
	// x-coord
	coords.x = atoi(args[4].second.c_str());
	// y-coord
	coords.y = atoi(args[5].second.c_str());

	// Loop through all device groups to find the one we are looking for
	bool found = false;
	for(UINT i=0; i<device_groups.size(); i++)
	{
		// Check the name to see if it matches
		if(!strcmp(group.c_str(), device_groups[i]->GetName()))
		{
			// Add the device to the device group
			if(!device_groups[i]->AddDevice(type, group, bit, image, coords))
			{
				// Return false if the device counldn't be added
				return false;
			}

			return true;
		}
	}

	// Return false if the device group wasn't found
	return false;
}

/*
 *	CBoard::ParseLCD()
 *
 *  Parses an AddLCD command from the .board file
 *
 *	Parameters: line - A C string that contails the line with the AddLCD command
 *
 *	Returns:	True if the parsing was successful and false if an error occured.
 */
bool CBoard::ParseLCD(const ParsedRowArguments& args)
{
	// Destroy the lcd window if it already exists
	//FIXME ?
	
	if(!ArgsMatches(args, "snn"))
		return false;

	lcd_name = args[0].second;
	lcd_coords.x = atoi(args[1].second.c_str());
	lcd_coords.y = atoi(args[2].second.c_str());

	// Signal that we have an lcd on the board
	lcd_available = true;

	return true;
}

/*
 *	CBoard::LoadBoard()
 *
 *  Loads and parses a .board file
 *
 *	Parameters: file - The file path and name of the .board file
 *
 *	Returns:	True if the .board file was loaded and false if an error occured.
 */
void CBoard::LoadBoard(char *file)
{
	char err_str[1024];

	// Wipe everything on the board before we start parsing
	CleanUp();
	
	// Make it work in Linux too
	for(int i=0; i<strlen(file); i++)
		if(file[i] == '\\')
			file[i] = '/';
	
	try
	{
		ParsedFile board_file = ParseFile(file);
	
		for(ParsedFile::iterator it = board_file.begin(); it != board_file.end(); ++it)
		{
			const string& command = it->first;
			if(command == "SetName")
			{
				if(!ArgsMatches(it->second, "s"))
				{
					sprintf(err_str, "Error while parsing SetName in \'%s\'", file);
					throw ParsingError(err_str);
				}
				name = it->second[0].second;
			}
			else if(command == "SetBackgroundImage")
			{
				if(!ArgsMatches(it->second, "s"))
				{
					sprintf(err_str, "Error while parsing SetBackgroundImage in \'%s\'", file);
					throw ParsingError(err_str);
				}
				bg_file = it->second[0].second;
				fix_filename(bg_file);
			}
			else if(command == "AddDeviceGroup")
			{
				if(!ParseDeviceGroup(it->second))
				{
					sprintf(err_str, "Error while parsing AddDeviceGroup in \'%s\'", file);
					throw ParsingError(err_str);
				}
			}
			else if(command == "AddDevice")
			{
				if(!ParseDevice(it->second))
				{
					sprintf(err_str, "Error while parsing AddDevice in \'%s\'", file);
					throw ParsingError(err_str);
				}
			}
			else if(command == "AddLCD")
			{
				if(!ParseLCD(it->second))
				{
					sprintf(err_str, "Error while parsing AddLCD in \'%s\'", file);
					throw ParsingError(err_str);
				}
			}
		}

		// Initialize the board
		if(!Init())
		{
			// Initializing failed, display error message and return false
			sprintf(err_str, "Error while trying to initialize the board!");
			throw InitBoardError(err_str);
		}
	}
	catch(FileDoesNotExistError& e)
	{
		throw InitBoardError("The board file '" + e.path + "' does not exist!");
	}
}

/*
 *	CBoard::Init()
 *
 *  Initializes the board
 *
 *	Returns:	True if the initializing was successful and false if it failed.
 */
bool CBoard::Init()
{
	// Load the background image
	GdkPixbuf *bg_pixbuf = gdk_pixbuf_new_from_stream(CFile(bg_file.c_str()).get_input_stream(), NULL, NULL);
	if(bg_pixbuf == NULL)
		return false;
	
	// Set the GtkFixed board area to the background size
	GtkRequisition control_size;
	control_size.width = gdk_pixbuf_get_width(bg_pixbuf);
	control_size.height = gdk_pixbuf_get_height(bg_pixbuf);
	gtk_widget_size_request(GTK_WIDGET(board_area), &control_size);
	
	// And add it to the window
	GtkWidget *bg_image = gtk_image_new_from_pixbuf(bg_pixbuf);
	gtk_fixed_put(board_area, bg_image, 0, 0);
	gtk_widget_show(bg_image);
	g_object_unref(bg_pixbuf);

	// Initialize all device groups
	for(UINT i=0; i<device_groups.size(); i++)
	{
		if(!device_groups[i]->Init(board_area))
			return false;
	}

	// Check if there is a LCD available
	if(lcd_available)
	{
		// Create the LCD window
		lcd_text_view = (GtkTextView*)gtk_text_view_new();
		gtk_text_view_set_editable(lcd_text_view, false);
		control_size.width = LCD_WIDTH;
		control_size.height = LCD_HEIGHT;
		gtk_widget_size_request((GtkWidget*)lcd_text_view, &control_size);
		gtk_fixed_put(board_area, (GtkWidget*)lcd_text_view, lcd_coords.x, lcd_coords.y);
		gtk_widget_show((GtkWidget*)lcd_text_view);
	}

	// Set the window title
	gtk_window_set_title(GTK_WINDOW(board_window), name.c_str());
	return true;
}

/*
 * CBoard::ShowCorrectImages()
 *
 * Moves the viewpoint of all device groups to the correct position
 */
void CBoard::ShowCorrectImages()
{
	for(UINT i=0; i<device_groups.size(); i++)
		device_groups[i]->ShowCorrectImages();
}

/*
 *	CBoard::GetDeviceGroup()
 *
 *  Finds a device group with a specific name
 *
 *	Parameters: gname - The name of the device group
 *
 *	Returns:	A pointer to the device group or NULL if we didn't find it
 */
CBoardDeviceGroup *CBoard::GetDeviceGroup(const char *gname)
{
	// Loop through all device groups
	for(UINT i=0; i<device_groups.size(); i++)
	{
		// Check if the name matches
		if(!strcmp(device_groups[i]->GetName(), gname))
			return device_groups[i];
	}

	// No matches, return NULL
	return NULL;
}

/*
 *	CBoard::UpdateLCDText()
 *
 *  Updates the lcd text buffer with new text
 *
 *	Parameters: text - The text to print on the lcd
 */
void CBoard::UpdateLCDText(char *text)
{
	lcd_text_buf = "";

	for(int i=0; i<LCD_TEXT_LEN; i++)
	{
		// Insert a new line
		if(i == LCD_TEXT_LEN/2)
			lcd_text_buf += '\n';

		if(text[i])
		{
			// Replace illegal characters by a space
			if(text[i] >= '!' && text[i] <= '}')
				lcd_text_buf += text[i];
			else
				lcd_text_buf += ' ';
		}
	}

	lcd_text_updated = true;
}

/*
 *	CBoard::WriteTextToLCD()
 *
 *  Writes the text from the lcd text buffer to the lcd window
 */
void CBoard::WriteTextToLCD()
{
	// Check if there is an lcd window created before we add the text
	//if(lcd_hWnd)
	//	SetWindowText(lcd_hWnd, lcd_text_buf.c_str());
	//FIXME
	
	if(lcd_available)
	{
		gtk_text_buffer_set_text(gtk_text_view_get_buffer(lcd_text_view), lcd_text_buf.c_str(), lcd_text_buf.length());
	}

	lcd_text_updated = false;
}
