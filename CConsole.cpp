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

#include <cstdlib>
#include <gtk/gtk.h>
#include <string>
#include "sim.h"
#include "CConsole.h"
#include "CFile.h"

static bool clicked_on_clear = false;
static bool not_by_user = false;

// callback jtag insert text, before
static void insert_text(GtkTextBuffer*, GtkTextIter *location, gchar *text, gint len, gpointer user_data)
{
	if(not_by_user)
		return;

	CConsole *console = (CConsole*)user_data;
	console->OnInsertText(location, text, len);
}
// callback jtag insert text, after
static void inserted_text(GtkTextBuffer*, GtkTextIter *location, gchar* text, gint len, gpointer user_data)
{
	if(not_by_user)
		return;
	
	CConsole *console = (CConsole*)user_data;
	console->OnInsertedText(location, text, len);
}

// callback uart
static gboolean key_press(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	char buf[2];
	int console_id = (int)(size_t)user_data;
	
	//printf("%d %x\n", (int)event->key.keyval, (int)event->key.keyval);
	
	int key = event->key.keyval;
	
	// Some commonly used keys: tab, backspace, enter etc.
	if((key >= 0xff00) && key <= 0xff20)
		key &= 0xff;
	
	if(key > 255)
		return FALSE;
	
	buf[0] = key;
	buf[1] = '\0';
	
	if(console_id == CONSOLE_UART0)
	{
		if(main_system.IsSimulationRunning())
			main_system.SendInputToUART0(buf);
	}
	else if(console_id == CONSOLE_UART1)
	{
		if(main_system.IsSimulationRunning())
			main_system.SendInputToUART1(buf);
	}
	
	return FALSE;
}

/*
 *	CConsole::CConsole()
 *
 *  Constructor of the CConsole class
 *
 *  Initializes all private variables to 0.
 */
CConsole::CConsole(int console_id) : console_id(console_id)
{
	buf = "";

	is_editing = false;
	edit_start_pos = 0;
}

/*
 *	CConsole::~CConsole()
 *
 *  Destructor of the CConsole class.
 */
CConsole::~CConsole()
{
}

/*
 *	CConsole::Clear()
 *
 *  Clears the text in the console
 */
void CConsole::Clear()
{
	// Empty the text buffed
	buf = "";
	is_editing = false;
	// Update the console window with no text
	clicked_on_clear = true;
	gtk_text_buffer_set_text(text_buffer, "", 0);
	clicked_on_clear = false;
}

/*
 *  CConsole::Init()
 *
 *  Initializes a console window from a GtkBuilder ui file.
 */
void CConsole::Init(gboolean (*close_window_function)(gpointer sender, gpointer user_data), GdkPixbuf *window_icon)
{
	GtkButton* clear_button;
	GtkBuilder *builder = gtk_builder_new();
	
	GError *error = NULL;
	
	pair<char*, size_t> console_xml = CFile("console.xml.gz").decompress().read_whole_file();
	if (!gtk_builder_add_from_string(builder, console_xml.first, console_xml.second, &error)){
		g_print("Msg: %s\n", error->message);
		g_free(error);
		free(console_xml.first);
		return;
	}
	free(console_xml.first);
	
	window = GTK_WIDGET(gtk_builder_get_object(builder, "winConsole"));
	text_view = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "txtView"));
	text_buffer = GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "txtBuffer"));
	clear_button = GTK_BUTTON(gtk_builder_get_object(builder, "btnClear"));
	
	gtk_window_set_icon(GTK_WINDOW(window), window_icon);
	
	g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(close_window_function), NULL);
	
	g_object_unref(G_OBJECT(builder));
	
	PangoFontDescription *font = pango_font_description_from_string("monospace");
	gtk_widget_modify_font(GTK_WIDGET(text_view), font);
	pango_font_description_free(font);
	
	if(console_id == CONSOLE_JTAG)
	{
		g_signal_connect_swapped(G_OBJECT(text_buffer), "delete-range", G_CALLBACK(&CConsole::OnDeleteRange), this);
		g_signal_connect(G_OBJECT(text_buffer), "insert-text", G_CALLBACK(insert_text), this);
		g_signal_connect_after(G_OBJECT(text_buffer), "insert-text", G_CALLBACK(inserted_text), this);
	}
	else if(console_id == CONSOLE_UART0 || console_id == CONSOLE_UART1)
	{
		g_signal_connect(G_OBJECT(text_view), "key-press-event", G_CALLBACK(key_press), (gpointer)console_id);
	}
	
	static const char *titles[] = {"JTAG Console", "UART0 Console", "UART1 Console"};
	gtk_window_set_title(GTK_WINDOW(window), titles[console_id]);
	
	g_signal_connect_swapped(G_OBJECT(clear_button), "clicked", G_CALLBACK(&CConsole::Clear), this);
}

/*
 *	CConsole::AddText()
 *
 *  Adds text to the console
 *
 *  Parameters: t - a string that contains the text that is to be added to the console
 *				update - a boolean that indicates if the edit control is to be updated 
 *						 immediately with the new text
 */
void CConsole::AddText(const string& t, bool update)
{
	// Lock the mutex
	lock.lock();

	for(UINT i=0; i<t.length(); i++)
	{
		if(t[i] == '\r' && t.c_str()[i+1] == '\n')
		{
			buf += '\r';
			buf += '\n';
			i++;
			continue;
		}

		if(t[i] == '\n' || t[i] == '\r')
		{
			buf += '\r';
			buf += '\n';
		}
		else
		{
			buf += t[i];
		}
	}

	// Unlock the mutex
	lock.unlock();

	if(update)
		Update();
}

/*
 *	CConsole::AddText()
 *
 *  Adds text to the console
 *
 *  Parameters: t - a C string that contains the text that is to be added to the console
 *				update - a boolean that indicates if the edit control is to be updated 
 *						 immediately with the new text
 */
void CConsole::AddText(char *t, bool update)
{
	// Lock the mutex
	lock.lock();

	for(UINT i=0; i<strlen(t); i++)
	{
		if(t[i] == '\r' && t[i+1] == '\n')
		{
			buf += '\r';
			buf += '\n';
			i++;
			continue;
		}

		if(t[i] == '\n' || t[i] == '\r')
		{
			buf += '\r';
			buf += '\n';
		}
		else
		{
			buf += t[i];
		}
	}

	// Unlock the mutex
	lock.unlock();

	if(update)
		Update(); 
}

/*
 *	CConsole::AddText()
 *
 *  Adds a character to the console
 *
 *  Parameters: t - The character that is to be added to the console
 *				update - a boolean that indicates if the edit control is to be updated 
 *						 immediately with the new text
 */
void CConsole::AddText(char t, bool update)
{
	// Lock the mutex
	lock.lock();

	if(t == '\n' || t == '\r')
	{
		buf += '\r';
		buf += '\n';
	}
	else
	{
		buf += t;
	}

	// Unlock the mutex
	lock.unlock();

	if(update)
		Update(); 
}

/*
 *	CConsole::Update()
 *
 *  Prints the text from the console text buffer to the edit control
 */
void CConsole::Update()
{
	UINT len;
	string text;

	// Check if the buffer has text
	if(buf.length() > 0)
	{
		// Lock the mutex
		lock.lock();

		// Copy the text and reset the buffer
		text = buf;
		buf = "";

		// Unlock the mutex
		lock.unlock();
		
		// Make the string a valid UTF-8 string
		size_t start = 0;
		while(start < text.length())
		{
			const gchar *end;
			g_utf8_validate(text.c_str() + start, text.length() - start, &end);
			if(end != text.c_str() + text.length())
			{
				text[end - text.c_str()] = ' ';
				end++;
			}
			start = end - text.c_str();
		}
		len = g_utf8_strlen(text.c_str(), text.length());

		// Update the edit control with the new text
		GtkTextIter pos;
		gtk_text_buffer_get_end_iter(text_buffer, &pos);
		if(is_editing)
		{
			gtk_text_iter_set_offset(&pos, edit_start_pos);
			edit_start_pos += len;
		}
		not_by_user = true;
		gtk_text_buffer_insert(text_buffer, &pos, text.c_str(), text.length());
		not_by_user = false;
		
		// It is not scrolled correctly if not done this way...
		g_idle_add((GSourceFunc)(&CConsole::ScrollToBottom), this);
	}
}

gboolean CConsole::ScrollToBottom()
{
	gdk_threads_enter();
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter(text_buffer, &iter);
	gtk_text_view_scroll_to_iter(text_view, &iter, 0.0, FALSE, 0, 0);
	gdk_threads_leave();
	
	return FALSE;
}

// Callback jtag when deleting text in the TextView
void CConsole::OnDeleteRange(GtkTextIter *start, GtkTextIter *end)
{
	int start_index, end_index;
	
	if(is_editing)
	{
		start_index = gtk_text_iter_get_offset(start);
		end_index = gtk_text_iter_get_offset(end);
		
		if(start_index >= edit_start_pos)
		{
			return;
		}
	}
	
	// Abort the deletion if we didn't click on the Clear button
	if(!clicked_on_clear)
		*start = *end;
}

// called from the callback function with the same name, jtag
void CConsole::OnInsertText(GtkTextIter *location, gchar *text, gint len)
{
	if(!is_editing)
	{
		// The user has not yet started to type text.
		// Set the insert position at the end of the text buffer.
		// Set a mark at this position (where the user starts typing).
		GtkTextIter end_iter;
		gtk_text_buffer_get_end_iter(text_buffer, &end_iter);
		edit_start_pos = gtk_text_iter_get_offset(&end_iter);
		*location = end_iter;
		is_editing = true;
	}
	else
	{
		// The user may not type anything before the mark set above.
		int location_index = gtk_text_iter_get_offset(location);
		if(location_index < edit_start_pos)
		{
			gtk_text_buffer_get_end_iter(text_buffer, location);
		}
	}
	
	// Only text up to the last newline is sent to the system
	for(int i=len-1; i>=0; i--){
		if(text[i] == '\n')
		{
			GtkTextIter start_iter;
			gtk_text_buffer_get_end_iter(text_buffer, &start_iter); // Only to initialize the iterator
			gtk_text_iter_set_offset(&start_iter, edit_start_pos);
			gchar *before = gtk_text_buffer_get_text(text_buffer, &start_iter, location, FALSE);
			
			string buf = before;
			g_free(before);
			buf.append(text, i+1);
			
			if(main_system.IsSimulationRunning())
				main_system.SendInputToJTAG(buf);
			break;
		}
	}

}
// called from the callback function with the same name, jtag
void CConsole::OnInsertedText(GtkTextIter *location, gchar *text, gint len)
{
	for(gint i=0; i<len; i++)
	{
		if(text[i] == '\n')
		{
			// After some text has been sent to the system, the edit mark position is set to the position right after that text.
			edit_start_pos = gtk_text_iter_get_offset(location) - gtk_text_iter_get_line_offset(location);
			return;
		}
	}
}
