/*
NIISim - Nios II Simulator, A simulator that is capable of simulating various systems containing Nios II cpus.
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

#include <cstdio>
#include <fstream>
#include <map>
#include <set>
#include <algorithm>

#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <gtksourceview/gtksourcemark.h>
#include <gtk/gtk.h>

#include "sim.h"
#include "CDebug.h"
#include "elf_read_debug.h"
#include "disassembler.h"
#include "CFile.h"

using namespace std;

namespace {

void add_implicit_breakpoints(const vector<pair<pair<int, int>, uint> >& matching_breakpoints, int start, bool adding);

GtkTreeView *tree_view;
GtkTreeStore *tree_store;
GtkNotebook *tab_container;

GtkWidget *step_into_button;
GtkWidget *step_over_button;
GtkWidget *step_return_button;
GtkWidget *continue_button;
GtkWidget *step_instruction_button;

GtkStatusbar *statusbar;
guint statusbar_context;
guint statusbar_message_id = 0;

GtkSourceBuffer *disasm_source_buffer;
GtkSourceView *disasm_source_view;

// The base address where the code starts (the .entry point always at 0x800000)
uint instruction_base_addr;
set<uint> instruction_breakpoints;
multimap<int, GtkSourceMark*> implicit_instruction_breakpoints_marks;

// Debug info from the ELF file. Store two so we can compare a reloaded file with the older version.
DebugInfo debug_infos[2];
int current_debug_info = 0;
#define debug_info debug_infos[current_debug_info]
multiset<uint> all_breakpoints;

bool add_breakpoint(uint addr)
{
	all_breakpoints.insert(addr);
	return all_breakpoints.count(addr) == 1;
}

bool remove_breakpoint(uint addr)
{
	pair<multiset<uint>::iterator, multiset<uint>::iterator> it = all_breakpoints.equal_range(addr);
	if(it.first == it.second)
		return false;
	
	--it.second;
	bool res = it.first == it.second; // Only one element found
	all_breakpoints.erase(it.first);
	return res;
}

struct TabPage
{
	int tab_id;
	
	string filepath;
	string filename;
	int file_id;
	
	GtkSourceView *source_view;
	GtkSourceBuffer *source_buffer;
	map<int, GtkSourceMark*> breakpoint_marks;
	multimap<int, GtkSourceMark*> implicit_breakpoint_marks;
	
	pair<int, uint> get_first_valid_breakpoint_line_from_line(int line);
	
	TabPage(string filepath, string filename, int file_id);
	void LineMark(GtkTextIter *iter, GdkEvent *event, GtkSourceView *view, int line0);
	void SetCurrentExecutingLineMark(int line);
	void RemoveCurrentExecutingLineMarks(void);
	
	void GoToLine(int line1);
	
	bool IsVisible(void);
	void Show(void);
	void Hide(void);
	void UnHide(void);
	
	void ReadSourceFile(void);
	
	void RemoveBreakpoints(void);
	void ClosePage(void);
	void RemovePage();
	
	static TabPage* FindTabPage(int file_id);
	static TabPage* AddTabPage(int file_id);
	static TabPage* FindOrAddTabPage(int file_id);
	static void RemoveAllPages(void);
};

vector<TabPage*> tab_pages;

TabPage* TabPage::FindTabPage(int file_id)
{
	for(size_t i=0, e=tab_pages.size(); i!=e; i++)
	{
		if(tab_pages[i]->file_id == file_id)
		{
			tab_pages[i]->UnHide();
			return tab_pages[i];
		}
	}
	return NULL;
}

TabPage* TabPage::AddTabPage(int file_id)
{
	const string& filepath = debug_info.source_files[file_id];
	
	TabPage *tab_page = new TabPage(filepath, filepath.substr(filepath.find_last_of('/')+1), file_id);
	tab_pages.push_back(tab_page);
	
	return tab_page;
}

TabPage* TabPage::FindOrAddTabPage(int file_id)
{
	if(TabPage* page = FindTabPage(file_id))
	{
		return page;
	}
	
	return AddTabPage(file_id);
}

void TabPage::RemoveAllPages(void){
	for(int i=tab_pages.size()-1; i>=0; i--)
	{
		tab_pages[i]->RemovePage();
		delete tab_pages[i];
	}
	tab_pages.clear();
}

gboolean gutter_tooltip_callback(GtkSourceGutter *gutter, GtkCellRenderer *renderer, GtkTextIter *iter, GtkTooltip *tooltip, gpointer user_data)
{
	// gtksourceview-2.10 has a bad API, so it's impossible to find out if the GtkCellRenderer is
	// the one rendering marks, or the one containing line numbers. However, I can use the following
	// hard-coded xpad and ypad values in the source code of gtksourceview.c to find out ;)
	gint xpad, ypad;
	gtk_cell_renderer_get_padding(renderer, &xpad, &ypad);
	if(xpad == 2 && ypad == 1)
	{
		// The cell renderer with the marks
		gtk_tooltip_set_text(tooltip, (const gchar*)user_data);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


TabPage::TabPage(string filepath, string filename, int file_id)
: filepath(filepath), filename(filename), file_id(file_id)
{
	/*const gchar * const * langs = gtk_source_language_manager_get_language_ids(
		gtk_source_language_manager_get_default());
	
	while(langs != NULL){
		puts(*langs++);
	}*/
	
	tab_id = tab_pages.size();
	
	
	source_view = (GtkSourceView*)gtk_source_view_new_with_buffer(
		source_buffer = gtk_source_buffer_new(NULL));
	
	// Try to find a syntax highlighting for the file
	gboolean result_uncertain;
	gchar *content_type = g_content_type_guess(filename.c_str(), NULL, 0, &result_uncertain);
	if(result_uncertain)
	{
		g_free(content_type);
		content_type = NULL;
	}
	gtk_source_buffer_set_language(
		source_buffer,
		gtk_source_language_manager_guess_language(
			gtk_source_language_manager_get_default(), filename.c_str(), content_type));
	g_free(content_type);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(source_view), FALSE);
	
	ReadSourceFile();
	
	GdkPixbuf *red = gdk_pixbuf_new_from_stream(CFile("red.png").get_input_stream(), NULL, NULL);
	GdkPixbuf *pink = gdk_pixbuf_new_from_stream(CFile("pink.png").get_input_stream(), NULL, NULL);
	GdkPixbuf *arrow = gdk_pixbuf_new_from_stream(CFile("arrow.png").get_input_stream(), NULL, NULL);
	gtk_source_view_set_mark_category_icon_from_pixbuf(source_view, "breakpoint", red);
	gtk_source_view_set_mark_category_icon_from_pixbuf(source_view, "implicit_breakpoint", pink);
	gtk_source_view_set_mark_category_icon_from_pixbuf(source_view, "current", arrow);
	gtk_source_view_set_mark_category_priority(source_view, "breakpoint", 2);
	gtk_source_view_set_mark_category_priority(source_view, "implicit_breakpoint", 1);
	gtk_source_view_set_mark_category_priority(source_view, "current", 3);
	g_object_unref(red);
	g_object_unref(pink);
	g_object_unref(arrow);
	
	// Set up tooltip
	GtkSourceGutter *gutter = gtk_source_view_get_gutter(source_view, GTK_TEXT_WINDOW_LEFT);
	g_signal_connect(G_OBJECT(gutter), "query-tooltip", G_CALLBACK(gutter_tooltip_callback), (gpointer)"Left click: Toggle breakpoint\nRight click: Go to instruction in disassembly window");
	gtk_widget_set_has_tooltip(GTK_WIDGET(source_view), TRUE);
	
	gtk_source_view_set_show_line_numbers(source_view, TRUE);
	gtk_source_view_set_show_line_marks(source_view, TRUE);
	g_signal_connect_swapped(G_OBJECT(source_view), "line-mark-activated", G_CALLBACK(&TabPage::LineMark), this);
	gtk_widget_show(GTK_WIDGET(source_view));
	
	// Scrolled window
	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(source_view));
	gtk_widget_show(scrolled_window);
	
	// Create the tab label and its close button
	GtkWidget *tab_label = gtk_label_new(filename.c_str());
	
	GtkWidget *tab_close_button = gtk_button_new();
	g_signal_connect_swapped(G_OBJECT(tab_close_button), "clicked", G_CALLBACK(&TabPage::ClosePage), this);
	GdkPixbuf *cross = gdk_pixbuf_new_from_stream(CFile("cross.png").get_input_stream(), NULL, NULL);
	gtk_container_add(GTK_CONTAINER(tab_close_button), gtk_image_new_from_pixbuf(cross)/*gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_BUTTON)*/);
	g_object_unref(cross);

	GtkWidget *tab_hbox = gtk_hbox_new(FALSE, 2);
	gtk_box_pack_start(GTK_BOX(tab_hbox), tab_label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(tab_hbox), tab_close_button, FALSE, FALSE, 0);
	gtk_widget_show_all(tab_hbox);
	
	// Add it to the notebook
	gtk_notebook_append_page(tab_container, scrolled_window, tab_hbox);
}

void TabPage::RemoveBreakpoints(void)
{
	//for(map<int, GtkSourceMark*>::iterator it = breakpoint_marks.begin(); it != breakpoint_marks.end(); ++it)
	while(breakpoint_marks.begin() != breakpoint_marks.end())
	{
		LineMark(NULL, NULL, NULL, breakpoint_marks.begin()->first-1);
	}
}

void TabPage::ClosePage(void)
{
	// Remove all breakpoints
	RemoveBreakpoints();
	
	// Simply hide it rather than removing it
	Hide();
}

void TabPage::RemovePage()
{
	// Remove all breakpoints
	RemoveBreakpoints();
	
	// Completely remove it from the notebook
	gtk_notebook_remove_page(tab_container, tab_id);
}

void TabPage::ReadSourceFile(void)
{
	ifstream ifs;
	ifs.open(filepath.c_str(), ios::in | ios::binary | ios::ate);
	if(!ifs.is_open())
	{	
		string buffer = "(file does not exist on this computer)";
		for(size_t i=1, e=debug_info.source_to_addr[file_id].size(); i<e; i++)
			buffer += '\n';
		
		gtk_text_buffer_set_text(GTK_TEXT_BUFFER(source_buffer), buffer.c_str(), -1);
	}
	else
	{
		size_t size = ifs.tellg();
		char *buffer = new char[size];
		ifs.seekg(0, ios::beg);
		ifs.read(buffer, size);
		ifs.close();
		if(!g_utf8_validate(buffer, size, NULL))
		{
			//fprintf(stderr, "File not valid utf8: %s\n", filepath.c_str());
			ShowErrorMessage(("File not valid UTF-8: " + filepath + "\n").c_str());
			string buf = "File not valid UTF-8";
			for(size_t i=1, e=debug_info.source_to_addr[file_id].size(); i<e; i++)
				buf += '\n';
			gtk_text_buffer_set_text(GTK_TEXT_BUFFER(source_buffer), buf.c_str(), -1);
		}
		else
		{
			gtk_text_buffer_set_text(GTK_TEXT_BUFFER(source_buffer), buffer, size);
		}
		delete[] buffer;
	}
}

// Both a callback and a function that gets called when hiding/destructing tab pages
// When used as callback: iter should be a valid GtkTextIter, line is undefined
// Otherwise, iter should be NULL and line should contain a correct line
void TabPage::LineMark(GtkTextIter *iter, GdkEvent *event, GtkSourceView *view, int line)
{
	// When destroying a tab page, iter is NULL and line already contains the line
	GtkTextIter iter2;
	if(iter != NULL)
	{
		line = gtk_text_iter_get_line(iter);
	}
	else
	{
		iter = &iter2;
		gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(source_buffer), iter, line);
	}
	
	vector<pair<pair<int, int>, uint> > matching_breakpoints = debug_info.GetMatchingBreakpoints(file_id, line+1);
	if(matching_breakpoints.empty())
		return;
	
	line = matching_breakpoints.front().first.second;
	
	// Right click -> Go to address in the disasm window
	if(event != NULL && event->button.button == 3)
	{
		GtkTextIter disasm_iter;
		line = (debug_info.source_to_addr[matching_breakpoints[0].first.first][line-1].front() - instruction_base_addr)/4;
		gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(disasm_source_buffer), &disasm_iter, line);
		gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(disasm_source_view), &disasm_iter, 0.04, FALSE, 0, 0);
		gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(disasm_source_buffer), &disasm_iter);
		return;
	}
	
	gtk_text_iter_set_line(iter, line-1); // Make it zero-indexed
	
	bool adding;
	
	map<int, GtkSourceMark*>::iterator it = breakpoint_marks.find(line);
	if(it != breakpoint_marks.end())
	{
		gtk_source_buffer_remove_source_marks(source_buffer, iter, iter, "breakpoint");
		breakpoint_marks.erase(it);
		if(remove_breakpoint(matching_breakpoints.front().second))
			main_debug.SetBreakpoint(matching_breakpoints.front().second);
		adding = false;
	}
	else
	{
		GtkSourceMark *mark = gtk_source_buffer_create_source_mark(source_buffer, NULL, "breakpoint", iter);
		breakpoint_marks.insert(make_pair(line, mark));
		if(add_breakpoint(matching_breakpoints.front().second))
			main_debug.SetBreakpoint(matching_breakpoints.front().second);
		adding = true;
	}
	
	
	add_implicit_breakpoints(matching_breakpoints, 1, adding);
	
	// Mark the addresses in the disasm window
	vector<uint>& v = debug_info.source_to_addr[matching_breakpoints[0].first.first][line-1];
	for(size_t i=0, e=v.size(); i!=e; i++)
	{
		GtkTextIter iter;
		int line0 = (v[i] - instruction_base_addr)/4;
		multimap<int, GtkSourceMark*>& marks = implicit_instruction_breakpoints_marks;
		
		multimap<int, GtkSourceMark*>::iterator it = marks.find(line0);
		
		if(adding)
		{
			if(it != marks.end()){
				// An implicit breakpoint already exists, so only update the count.
				marks.insert(*it);
			}
			else
			{
				gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(disasm_source_buffer), &iter, line0);
				GtkSourceMark *mark = gtk_source_buffer_create_source_mark(disasm_source_buffer, NULL, "implicit_breakpoint", &iter);
				marks.insert(make_pair(line0, mark));
			}
		}
		else
		{
			if(marks.count(line0) == 1)
			{
				// Only remove it when we are get to the last one.
				gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(disasm_source_buffer), &iter, line0);
				gtk_source_buffer_remove_source_marks(disasm_source_buffer, &iter, &iter, "implicit_breakpoint");
			}
			marks.erase(it);
		}
	}
	
}

void TabPage::GoToLine(int line1)
{
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(source_buffer), &iter, line1-1);
	gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(source_view), &iter, 0.04, FALSE, 0, 0);
	gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(source_buffer), &iter);
}

void TabPage::SetCurrentExecutingLineMark(int line)
{
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(source_buffer), &iter, line-1);
	GtkSourceMark *mark = gtk_source_buffer_create_source_mark(source_buffer, NULL, "current", &iter);
	
	// Scroll to it
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(source_view), GTK_TEXT_MARK(mark), 0.04, FALSE, 0, 0);
}

void TabPage::RemoveCurrentExecutingLineMarks(void)
{
	GtkTextIter start_iter, end_iter;
	
	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(source_buffer), &start_iter);
	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(source_buffer), &end_iter);
	gtk_source_buffer_remove_source_marks(source_buffer, &start_iter, &end_iter, "current");
}

bool TabPage::IsVisible(void)
{
	return gtk_notebook_get_current_page(tab_container) == tab_id;
}

void TabPage::Show(void)
{
	UnHide();
	gtk_notebook_set_current_page(tab_container, tab_id);
}

void TabPage::Hide(void)
{
	GtkWidget *page = gtk_notebook_get_nth_page(tab_container, tab_id);
	gtk_widget_hide(page);
}

void TabPage::UnHide(void)
{
	GtkWidget *page = gtk_notebook_get_nth_page(tab_container, tab_id);
	gtk_widget_show(page);
}

// Split up a path and map each unique directory to a number for simplicity
map<string, int> mapping;
vector<string> rev_mapping;
vector<int> string_path_to_int_path(const string& path){
	vector<int> ret;
	
	// Linux: path always starts on '/', remove it.
	// Windows: path may start on "C:\", keep it.
	size_t start = path.length() > 0 && path[0] == '/' ? 1 : 0;
	for(size_t pos=start, e=path.length(); pos<e;){
		size_t end = path.find_first_of("/\\", pos);
		string dir = path.substr(pos, end-pos);
		
		pos = end+1;
		if(end == string::npos)
			pos = string::npos;
		
		if(dir == "..")
		{
			ret.pop_back();
			continue;
		}
		
		map<string, int>::iterator it = mapping.find(dir);
		if(it != mapping.end())
		{
			ret.push_back(it->second);
		}
		else
		{
			rev_mapping.push_back(dir);
			ret.push_back(mapping.size());
			mapping.insert(make_pair(dir, mapping.size()));
		}
	}
	
	return ret;
}

void init_tree_store(const vector<string>& files)
{
	vector<pair<int, GtkTreeIter> > tree_path; // Used as a stack
	
	vector<int> cur;
	
	gtk_tree_store_clear(tree_store);
	
	for(size_t file_id=0, e=files.size(); file_id!=e; file_id++)
	{
		vector<int> path = string_path_to_int_path(files[file_id]);
		size_t same = mismatch(cur.begin(), cur.end(), path.begin()).first - cur.begin();
		
		tree_path.resize(path.size());
		for(size_t i=same; i<path.size(); i++)
		{
			GtkTreeIter cur_iter;
			gtk_tree_store_append(tree_store, &cur_iter, i == 0 ? NULL : &tree_path[i-1].second);
			gtk_tree_store_set(tree_store, &cur_iter,
					0, i == path.size() - 1 ? "gtk-file" : "gtk-directory",
					1, rev_mapping[path[i]].c_str(),
					2, i == path.size() - 1 ? files[file_id].c_str() : "",
					3, i == path.size() - 1 ? file_id : -1,
					-1);
			tree_path[i] = make_pair(path[i], cur_iter);
		}
		cur = path;
	}
}

void open_file_from_tree(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	GtkTreeModel *model = GTK_TREE_MODEL(tree_store);
	GtkTreeIter iter;
	GValue filepath_value = { 0 };
	GValue filename_value = { 0 };
	GValue file_id_value = { 0 };
	const char *filepath, *filename;
	TabPage *tab_page;
	int file_id;
	
	gtk_tree_model_get_iter(model, &iter, path);
	
	gtk_tree_model_get_value(model, &iter, 2, &filepath_value);
	filepath = g_value_get_string(&filepath_value);
	if(filepath[0] == '\0') // A directory
	{
		g_value_unset(&filepath_value);
		return;
	}
	
	gtk_tree_model_get_value(model, &iter, 1, &filename_value);
	filename = g_value_get_string(&filename_value);
	gtk_tree_model_get_value(model, &iter, 3, &file_id_value);
	file_id = g_value_get_int(&file_id_value);

	tab_page = TabPage::FindOrAddTabPage(file_id);
	if(tab_page != NULL)
		tab_page->Show();

	g_value_unset(&filepath_value);
	g_value_unset(&filename_value);
	g_value_unset(&file_id_value);
}

void add_implicit_breakpoints(const vector<pair<pair<int, int>, uint> >& matching_breakpoints, int start, bool adding)
{
	for(size_t i=start, e=matching_breakpoints.size(); i<e; i++)
	{
		int f_id = matching_breakpoints[i].first.first;
		int line = matching_breakpoints[i].first.second;
		uint addr = matching_breakpoints[i].second;
		
		TabPage *tab_page = TabPage::FindOrAddTabPage(f_id);
		
		GtkTextIter iter;
		gtk_text_buffer_get_start_iter((GtkTextBuffer*)tab_page->source_buffer, &iter);
		gtk_text_iter_set_line(&iter, line-1); // Make it zero-indexed
		
		multimap<int, GtkSourceMark*>::iterator it = tab_page->implicit_breakpoint_marks.find(line);
		if(adding)
		{
			if(it == tab_page->implicit_breakpoint_marks.end())
			{
				GtkSourceMark *mark = gtk_source_buffer_create_source_mark(tab_page->source_buffer, NULL, "implicit_breakpoint", &iter);
				tab_page->implicit_breakpoint_marks.insert(make_pair(line, mark));
			}
			else
			{
				tab_page->implicit_breakpoint_marks.insert(*it); // Add another one
			}
			if(add_breakpoint(addr))
				main_debug.SetBreakpoint(addr);
		}
		else
		{
			if(tab_page->implicit_breakpoint_marks.count(line) == 1)
				gtk_source_buffer_remove_source_marks(tab_page->source_buffer, &iter, &iter, "implicit_breakpoint");
			
			tab_page->implicit_breakpoint_marks.erase(it);
			if(remove_breakpoint(addr))
				main_debug.SetBreakpoint(addr);
		}
	}
}

void delete_current_executing_disasm_line_mark(void)
{
	GtkTextIter start_iter, end_iter;
	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(disasm_source_buffer), &start_iter);
	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(disasm_source_buffer), &end_iter);
	gtk_source_buffer_remove_source_marks(disasm_source_buffer, &start_iter, &end_iter, "current");
}

void set_current_executing_disasm_line_mark(uint addr)
{
	GtkTextIter iter;
	int line0 = (addr - instruction_base_addr)/4;
	
	// Remove the old one
	delete_current_executing_disasm_line_mark();
	
	// Add the new one
	gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(disasm_source_buffer), &iter, line0);
	GtkSourceMark *mark = gtk_source_buffer_create_source_mark(disasm_source_buffer, NULL, "current", &iter);
	
	// Scroll to it
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(disasm_source_view), GTK_TEXT_MARK(mark), 0.07, FALSE, 0, 0);
}

gboolean later_go_to_line(gpointer user_data)
{
	// Lock the GUI
	gdk_threads_enter();
	
	pair<TabPage*, int> *data = (pair<TabPage*, int>*)user_data;
	TabPage *tab_page = data->first;
	int line = data->second;
	
	tab_page->GoToLine(line);
	
	// Unlock the GUI
	gdk_threads_leave();
	
	delete data;
	
	return FALSE;
}

void toggle_instruction_breakpoint(GtkSourceView *view, GtkTextIter *iter, GdkEvent *event, int line)
{
	// If iter == NULL then line must be set by the caller
	GtkTextIter iter2;
	if(iter != NULL)
	{
		line = gtk_text_iter_get_line(iter);
	}
	else
	{
		iter = &iter2;
		gtk_text_buffer_get_iter_at_line(GTK_TEXT_BUFFER(disasm_source_buffer), iter, line);
	}
	
	uint addr = instruction_base_addr + line*4;
	
	// Right click -> Go to line in source code
	if (event != NULL && event->button.button == 3)
	{
		uint addr_with_source = debug_info.GetNearestPrecedingAddrWithSourceInformation(addr);
		if(addr_with_source == ~0)
			return;
		
		const vector<pair<int, int> >& lines = debug_info.AddrToSource(addr_with_source);
		if(lines.empty())
		{
			// Should not happen
			return;
		}
		
		TabPage *tab_page = TabPage::FindOrAddTabPage(lines[0].first);
		tab_page->Show();
		// This is needed because otherwise it won't scroll to the mark if the page was just opened...
		g_idle_add(later_go_to_line, new pair<TabPage*, int>(tab_page, lines[0].second));
		
		return;
	}
	
	bool adding;
	
	set<uint>::iterator it = instruction_breakpoints.find(addr);
	if(it != instruction_breakpoints.end())
	{
		gtk_source_buffer_remove_source_marks(disasm_source_buffer, iter, iter, "breakpoint");
		instruction_breakpoints.erase(it);
		if(remove_breakpoint(addr))
			main_debug.SetBreakpoint(addr);
		adding = false;
	}
	else
	{
		GtkSourceMark *mark = gtk_source_buffer_create_source_mark(disasm_source_buffer, NULL, "breakpoint", iter);
		instruction_breakpoints.insert(addr);
		if(add_breakpoint(addr))
			main_debug.SetBreakpoint(addr);
		adding = true;
	}
	
	uint addr_with_source = debug_info.GetNearestPrecedingAddrWithSourceInformation(addr);
	if(addr_with_source == ~0)
		return;
	
	vector<pair<pair<int, int>, uint> > source_matchings;
	const vector<pair<int, int> >& lines = debug_info.AddrToSource(addr_with_source);
	for(size_t k=0, e=lines.size(); k!=e; k++)
	{
		pair<pair<int, int>, uint> p;
		p.first = lines[k];
		p.second = addr_with_source;
		
		if(!count(source_matchings.begin(), source_matchings.end(), p))
			source_matchings.push_back(p);
	}
	
	add_implicit_breakpoints(source_matchings, 0, adding);
}

gboolean later_line_mark(gpointer user_data)
{
	// Lock the GUI
	gdk_threads_enter();
	
	pair<TabPage*, int> *data = (pair<TabPage*, int>*)user_data;
	TabPage *tab_page = data->first;
	int line = data->second;
	
	//printf("%s %d\n", tab_page->filename.c_str(), line);
	
	tab_page->SetCurrentExecutingLineMark(line);
	
	// Unlock the GUI
	gdk_threads_leave();
	
	delete data;
	
	return FALSE;
}

gboolean update_statusbar_callback(gpointer user_data)
{
	// Lock the GUI
	gdk_threads_enter();
	
	const char *text = (const char*)user_data;
	if(statusbar_message_id > 0)
		gtk_statusbar_remove(statusbar, statusbar_context, statusbar_message_id);
	
	statusbar_message_id = text != NULL ? gtk_statusbar_push(statusbar, statusbar_context, text) : 0;
	
	// Unlock the GUI
	gdk_threads_leave();
	return FALSE;
}

gboolean break_callback(gpointer user_data)
{
	// The system might have been stopped by the user in the last cpu cycle
	if(!main_system.IsSimulationRunning())
		return FALSE;
	
	// Lock the GUI
	gdk_threads_enter();
	
	uint addr = (uint)(size_t)user_data;
	
	// Set arrows
	main_debug.SetCurrentExecutingLineMarks(addr, true, true);
	
	// Update the run, pause and stop buttons
	SetSensitiveButtons(true, false, true);
	
	// Flush the windows with the latest information
	UpdateConsolesFunc();
	
	// Remove the statusbar message
	main_debug.UpdateStatusbar(NULL);
	
	// Unlock the GUI
	gdk_threads_leave();
	
	return FALSE;
}

} // end of unnamed namespace


void CDebug::Init(GtkBuilder *builder)
{
	// Init the user interface
	tree_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "debugFilesTreeView"));
	tree_store = GTK_TREE_STORE(gtk_builder_get_object(builder, "debugFilesTreeStore"));
	tab_container = GTK_NOTEBOOK(gtk_builder_get_object(builder, "tabOpenedFiles"));
	statusbar = GTK_STATUSBAR(gtk_builder_get_object(builder, "debugStatusBar"));
	
	g_signal_connect(G_OBJECT(tree_view), "row-activated", G_CALLBACK(open_file_from_tree), NULL);
	
	step_into_button = GTK_WIDGET(gtk_builder_get_object(builder, "btnStepInto"));
	step_over_button = GTK_WIDGET(gtk_builder_get_object(builder, "btnStepOver"));
	step_return_button = GTK_WIDGET(gtk_builder_get_object(builder, "btnStepReturn"));
	continue_button = GTK_WIDGET(gtk_builder_get_object(builder, "btnContinue"));
	step_instruction_button = GTK_WIDGET(gtk_builder_get_object(builder, "btnStepInstruction"));
	
	g_signal_connect_swapped(step_into_button, "clicked", G_CALLBACK(&CDebug::StepInto), this);
	g_signal_connect_swapped(step_over_button, "clicked", G_CALLBACK(&CDebug::StepOver), this);
	g_signal_connect_swapped(step_return_button, "clicked", G_CALLBACK(&CDebug::StepReturn), this);
	g_signal_connect_swapped(continue_button, "clicked", G_CALLBACK(&CDebug::Continue), this);
	g_signal_connect_swapped(step_instruction_button, "clicked", G_CALLBACK(&CDebug::StepInstruction), this);
	
	// Init the disassembled view
	GtkContainer *disasm_container = GTK_CONTAINER(gtk_builder_get_object(builder, "scrolledWindowDisasm"));
	disasm_source_buffer = gtk_source_buffer_new(NULL);
	disasm_source_view = (GtkSourceView*)gtk_source_view_new_with_buffer(disasm_source_buffer);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(disasm_source_view), FALSE);
	
	GdkPixbuf *red = gdk_pixbuf_new_from_stream(CFile("red.png").get_input_stream(), NULL, NULL);
	GdkPixbuf *pink = gdk_pixbuf_new_from_stream(CFile("pink.png").get_input_stream(), NULL, NULL);
	GdkPixbuf *arrow = gdk_pixbuf_new_from_stream(CFile("arrow.png").get_input_stream(), NULL, NULL);
	gtk_source_view_set_mark_category_icon_from_pixbuf(disasm_source_view, "breakpoint", red);
	gtk_source_view_set_mark_category_icon_from_pixbuf(disasm_source_view, "implicit_breakpoint", pink);
	gtk_source_view_set_mark_category_icon_from_pixbuf(disasm_source_view, "current", arrow);
	gtk_source_view_set_mark_category_priority(disasm_source_view, "breakpoint", 2);
	gtk_source_view_set_mark_category_priority(disasm_source_view, "implicit_breakpoint", 1);
	gtk_source_view_set_mark_category_priority(disasm_source_view, "current", 3);
	g_object_unref(red);
	g_object_unref(pink);
	g_object_unref(arrow);
	
	// Set up tooltip
	GtkSourceGutter *gutter = gtk_source_view_get_gutter(disasm_source_view, GTK_TEXT_WINDOW_LEFT);
	g_signal_connect(G_OBJECT(gutter), "query-tooltip", G_CALLBACK(gutter_tooltip_callback), (gpointer)"Left click: Toggle breakpoint\nRight click: Go to matching source code line");
	gtk_widget_set_has_tooltip(GTK_WIDGET(disasm_source_view), TRUE);
	
	gtk_source_view_set_show_line_numbers(disasm_source_view, TRUE);
	gtk_source_view_set_show_line_marks(disasm_source_view, TRUE);
	g_signal_connect(G_OBJECT(disasm_source_view), "line-mark-activated", G_CALLBACK(toggle_instruction_breakpoint), NULL);
	gtk_widget_show(GTK_WIDGET(disasm_source_view));
	gtk_container_add(disasm_container, (GtkWidget*)disasm_source_view);
	
	statusbar_context = gtk_statusbar_get_context_id(statusbar, "context");
	
	
	//GtkSourceMarkAttributes *attributes = gtk_source_mark_attributes_new();
	//gtk_source_mark_attributes_set_stock_id(attributes, GTK_STOCK_STOP);
	//gtk_source_view_set_mark_attributes(GTK_SOURCE_VIEW(widget), "breakpoint", attributes, 0);
}

/*
 *  CDebug::SetMemoryInfo()
 *
 *  Tell the debugger about the location and span of the sdram
 */
void CDebug::SetMemoryInfo(uint base, uint span)
{
	memory_base_addr = base;
	address_has_breakpoint.assign(span, false);
	all_source_breakpoints.assign(span, false);
}

/*
 * CDebug::LoadELFFile()
 *
 * Load a new or reloaded ELF file into the debugger. All old breakpoints are removed.
 */
void CDebug::LoadELFFile(const char *filedata)
{
	// Remove all previous breakpoints
	for(size_t i=0, e=tab_pages.size(); i!=e; i++)
	{
		tab_pages[i]->RemoveBreakpoints();
	}
	while(!instruction_breakpoints.empty())
	{
		toggle_instruction_breakpoint(NULL, NULL, NULL, (*instruction_breakpoints.begin() - instruction_base_addr)/4);
	}
	
	// Load debugging info
	current_debug_info = !current_debug_info;
	BuildDebugInfo(debug_info, filedata);
	if(debug_infos[0].source_files != debug_infos[1].source_files)
	{
		TabPage::RemoveAllPages();
		init_tree_store(debug_info.source_files);
	}
	else
	{
		// We reload all files, since they might have been updated.
		for(size_t i=0, e=tab_pages.size(); i!=e; i++)
		{
			tab_pages[i]->ReadSourceFile();
		}
	}
	
	address_has_breakpoint.assign(address_has_breakpoint.size(), false);
	all_source_breakpoints.assign(all_source_breakpoints.size(), false);
	
	for(size_t i=0, e=debug_info.addresses.size(); i!=e; i++)
	{
		all_source_breakpoints[debug_info.addresses[i] - memory_base_addr] = true;
	}
	
	// Load the Disassembly
	string disasm;
	
	pair<pair<uint*, size_t>, uint> entry_section = ELFReadSection(filedata, ".entry");
	pair<pair<uint*, size_t>, uint> exceptions_section = ELFReadSection(filedata, ".exceptions");
	pair<pair<uint*, size_t>, uint> text_section = ELFReadSection(filedata, ".text");
	pair<pair<uint*, size_t>, uint> code_section =
		make_pair(
			make_pair(entry_section.first.first,
					  entry_section.first.second + exceptions_section.first.second + text_section.first.second),
			entry_section.second);
	
	if(entry_section.second + (entry_section.first.second + exceptions_section.first.second)*4 != text_section.second)
	{
		puts("some failure while reading .entry, .exceptions and .text...");
	}
	
	for(size_t i=0; i<code_section.first.second; i++)
	{
		uint instr = code_section.first.first[i];
		uint addr = code_section.second + i*4;
		
		char buffer[64];
		int len = sprintf(buffer, "%p: %08x %s\n", (void*)addr, instr, DumpDisasm(DecompileInstruction(addr, instr)));
		if (len < 0)
			continue;
		disasm.append(buffer, len);
	}
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(disasm_source_buffer), disasm.c_str(), disasm.size());
	instruction_base_addr = code_section.second;
}

/*
 * CDebug::SetBreakpoints()
 *
 * Set an instruction breakpoint at each address in the vector
 */
void CDebug::SetBreakpoints(const vector<uint>& addrs)
{
	for(size_t i=0, e=addrs.size(); i!=e; i++)
		SetBreakpoint(addrs[i]);
}

void CDebug::BreakFromThread(uint addr)
{
	// Lower priority than scroll to mark, so we can delete them after they have been created instead of before...
	g_idle_add_full(G_PRIORITY_DEFAULT_IDLE+10, break_callback, (gpointer)(size_t)addr, NULL);
}

void CDebug::EnterFunctionFromThread(uint pc, uint sp)
{
	//call_stack.push_back(make_pair(pc, sp));
	call_stack_size++;
}

void CDebug::RetFromThread(uint pc, uint sp)
{
	// Maybe some assert that the pair pc and sp matches call_stack.back()
	//if(!call_stack.empty())
	//if(call_stack_size != 0)
	//{
		//call_stack.pop_back();
		call_stack_size--;
		if(debugging_state == STEP_OVER && call_stack_size < step_over_or_return_stack_frame)
			SetDebuggingState(STEP_INTO); // Stop at the next possible breakpoint
		else if(debugging_state == STEP_RETURN && call_stack_size < step_over_or_return_stack_frame)
			SetDebuggingState(STEP_INTO); // Stop at the next possible breakpoint
	//}
}

void CDebug::EnableButtons(void)
{
	gtk_widget_set_sensitive(step_into_button, TRUE);
	gtk_widget_set_sensitive(step_over_button, TRUE);
	gtk_widget_set_sensitive(step_return_button, TRUE);
	gtk_widget_set_sensitive(continue_button, TRUE);
	gtk_widget_set_sensitive(step_instruction_button, TRUE);
}

void CDebug::DisableButtons(void)
{
	gtk_widget_set_sensitive(step_into_button, FALSE);
	gtk_widget_set_sensitive(step_over_button, FALSE);
	gtk_widget_set_sensitive(step_return_button, FALSE);
	gtk_widget_set_sensitive(continue_button, FALSE);
	gtk_widget_set_sensitive(step_instruction_button, FALSE);
}

/*
 * CDebug::SetCurrentExecutingLineMarks()
 *
 * Sets markers in the disasm window and on each source code line that matches the current address
 */
void CDebug::SetCurrentExecutingLineMarks(uint addr, bool open_new_pages, bool show_page)
{
	uint addr_with_source = debug_info.GetNearestPrecedingAddrWithSourceInformation(addr);
	if(addr_with_source != ~0)
	{
		const vector<pair<int, int> >& file_lines = debug_info.AddrToSource(addr_with_source);
		
		for(int i=0, e=tab_pages.size(); i!=e; i++)
			tab_pages[i]->RemoveCurrentExecutingLineMarks();
		
		for(size_t i=0, e=file_lines.size(); i!=e; i++){
			pair<int, int> file_line = file_lines[i];
			
			//printf("%x %s %d\n", addr_with_source, debug_info.source_files[file_line.first].c_str(), file_line.second);
			
			TabPage *tab_page = TabPage::FindTabPage(file_line.first);
			bool already_visible = tab_page != NULL && tab_page->IsVisible();
			if(tab_page == NULL && open_new_pages)
				tab_page = TabPage::AddTabPage(file_line.first);
			
			if(tab_page)
			{
				if(already_visible)
				{
					tab_page->SetCurrentExecutingLineMark(file_line.second);
					//g_idle_add(later_line_mark, new pair<TabPage*, int>(tab_page, file_line.second));
				}
				else if(show_page)
				{
					tab_page->Show();
					// This is needed because otherwise it won't scroll to the mark...
					g_idle_add(later_line_mark, new pair<TabPage*, int>(tab_page, file_line.second));
				}
			}
		}
	}
	
	// Update the arrow in the disasm window
	set_current_executing_disasm_line_mark(addr);
}

void CDebug::RemoveAllExecutingLineMarks()
{
	for(int i=0, e=tab_pages.size(); i!=e; i++)
		tab_pages[i]->RemoveCurrentExecutingLineMarks();
	delete_current_executing_disasm_line_mark();
}

void CDebug::ResumeSimulation(int debug_state, bool save_stack_frame)
{
	RemoveAllExecutingLineMarks();
	bool paused = main_system.IsSimulationPaused();
	main_debug.SetDebuggingState(debug_state);
	if(save_stack_frame)
		step_over_or_return_stack_frame = call_stack_size;
	if(paused)
		main_system.UnPauseSimulationThread();
	
	SetSensitiveButtons(false, true, true);
}
void CDebug::StepInto()
{
	ResumeSimulation(STEP_INTO, false);
}
void CDebug::StepOver()
{
	ResumeSimulation(STEP_OVER, true);
}
void CDebug::StepReturn()
{
	ResumeSimulation(STEP_RETURN, true);
}
void CDebug::Continue()
{
	ResumeSimulation(CONTINUE, false);
}
void CDebug::StepInstruction()
{
	ResumeSimulation(STEP_INSTRUCTION, false);
}
void CDebug::UpdateStatusbar(const char *text)
{
	g_idle_add(update_statusbar_callback, (void*)text);
}
void CDebug::SetDebuggingState(int state)
{
	static const char *modes[] = {"Continue", "Step Into", "Step Over", "Step Return", "Step Instruction"};
	debugging_state = state;
	UpdateStatusbar(modes[state]);
}
void CDebug::Cleanup()
{
	for(size_t i=0, e=tab_pages.size(); i!=e; i++)
	{
		delete tab_pages[i];
	}
}
