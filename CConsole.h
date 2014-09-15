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

#ifndef _CCONSOLE_H_
#define _CCONSOLE_H_

#include <gtk/gtk.h>
#include <string>
using namespace std;
#include "CThread.h"

enum {
	CONSOLE_JTAG,
	CONSOLE_UART0,
	CONSOLE_UART1
};

class CConsole
{
private:
	GtkWidget *window;
	GtkTextView *text_view;
	GtkTextBuffer *text_buffer;
	string buf;				//

	bool is_editing;		// True if the user is typing new text in the console
	UINT edit_start_pos;	// Position of the first character the user typed
	int console_id;

	CMutex lock;			// Handle to a lock that will protect the buffer buf
public:
	CConsole(int console_id);
	~CConsole();
	
	void Init(gboolean (*close_window_function)(gpointer sender, gpointer user_data), GdkPixbuf *window_icon);
	
	GtkWidget *GetWindow() { return window; }

	static void Clear(CConsole *self);
	void AddText(const string& t, bool update = true);
	void AddText(char *t, bool update = true);
	void AddText(char t, bool update = true);
	void Update();
	bool IsBufferEmpty() { return (buf.length() == 0);};
	
	static void OnDeleteRange(GtkTextIter *start, GtkTextIter *end, CConsole *self);
	void OnInsertText(GtkTextIter *location, gchar *text, gint len);
	void OnInsertedText(GtkTextIter *location, gchar *text, gint len);
	static gboolean ScrollToBottom(CConsole *self);
};

#endif
