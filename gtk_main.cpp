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

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <cstdio>
#include <cstdlib>

#include "CDebug.h"
#include "CCpu.h"
#include "sim.h"
#include "CFile.h"

// Exported variables
CSystem main_system;					// The main system
CBoard main_board;						// The I/O board
CConsole jtag_console(CONSOLE_JTAG);	// The JTAG console
CConsole uart0_console(CONSOLE_UART0);	// The UART0 console
CConsole uart1_console(CONSOLE_UART1);	// The UART1 console
CDebug main_debug; // The debug window

GtkTreeView *reg_tree_view;
GtkListStore *reg_list_store;
GtkTreeIter reg_iters[19];
GtkCellRendererText *col_right_regs;

GtkWidget *main_window;
GtkWidget *about_dialog;
GtkWidget *board_window;
GtkFixed  *board_area;
GtkWidget *register_window;
GtkWidget *debug_window;

GtkCheckMenuItem *board_window_toggle;
GtkCheckMenuItem *register_window_toggle;
GtkCheckMenuItem *jtag_window_toggle;
GtkCheckMenuItem *uart0_window_toggle;
GtkCheckMenuItem *uart1_window_toggle;
GtkCheckMenuItem *debug_window_toggle;

GtkWidget *run_cpu_button, *run_cpu_menu_item;
GtkWidget *pause_cpu_button, *pause_cpu_menu_item;
GtkWidget *stop_cpu_button, *stop_cpu_menu_item;

GtkCheckMenuItem *cpu_slow_mode_menu_item;

GtkCheckMenuItem *trace_menu_toggle;
GtkToggleToolButton *trace_button;

static string last_elf_file;

extern "C" G_MODULE_EXPORT void MenuStop(gpointer sender, gpointer user_data);

static void InitRegisterListView()
{
	static const char reg_names[32][16] = {"r0 / zero", "r1 / at", "r2",  "r3",
		"r4", "r5", "r6", "r7",
		"r8", "r9", "r10", "r11",
		"r12", "r13", "r14", "r15",
		"r16", "r17", "r18", "r19",
		"r20", "r21", "r22", "r23",
		"r24 / et", "r25 / bt", "r26 / gp", "r27 / sp",
		"r28 / fp", "r29 / ea", "r30 / ba", "r31 / ra"};
	
	const char* text = "";
	
	for(int i=0; i<19; i++)
	{
		if (i < 16)
			text = reg_names[i];
		else if (i == 16)
			text = "status";
		else if (i == 17)
			text = "ienable";
		else if (i == 18)
			text = "pc";
		
		gtk_list_store_append(reg_list_store, &reg_iters[i]);
		gtk_list_store_set(reg_list_store, &reg_iters[i],
			0, text,
			1, "0x00000000",
			-1);
		
	}
	for(int i=0; i<18; i++)
	{
		if (i < 16)
			text = reg_names[i+16];
		else if (i == 16)
			text = "estatus";
		else if (i == 17)
			text = "ipending";
		
		gtk_list_store_set(reg_list_store, &reg_iters[i],
			2, text,
			3, "0x00000000",
			-1);
	}
}

/*
 *	update_register_window()
 *
 *  Updates the register values in the register list view
 */
static void update_register_window()
{
	// Check if there is a cpu in the system
	if(main_system.HasCPU())
	{
		char text[16];
		CCpu *cpu = main_system.GetCPU(0);

		text[0] = '\0';

		// Update all 32 registers
		for(int i=0; i<16; i++)
		{
			sprintf(text, "0x%.8X", cpu->GetReg(i));
			gtk_list_store_set(reg_list_store, &reg_iters[i], 1, text, -1);

			sprintf(text, "0x%.8X", cpu->GetReg(i+16));
			gtk_list_store_set(reg_list_store, &reg_iters[i], 3, text, -1);
			
		}

		// status and estatus
		sprintf(text, "0x%.8X", cpu->GetCtrlReg(0));
		gtk_list_store_set(reg_list_store, &reg_iters[16], 1, text, -1);

		sprintf(text, "0x%.8X", cpu->GetCtrlReg(1));
		gtk_list_store_set(reg_list_store, &reg_iters[16], 3, text, -1);

		// ienable and ipending
		sprintf(text, "0x%.8X", cpu->GetCtrlReg(3));
		gtk_list_store_set(reg_list_store, &reg_iters[17], 1, text, -1);

		sprintf(text, "0x%.8X", cpu->GetCtrlReg(4));
		gtk_list_store_set(reg_list_store, &reg_iters[17], 3, text, -1);

		// pc
		sprintf(text, "0x%.8X", cpu->GetPC());
		gtk_list_store_set(reg_list_store, &reg_iters[18], 1, text, -1);
	}
}

void ShowErrorMessage(const char *msg)
{
	GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
		"%s", msg);
	
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

void SetSensitiveButtons(bool run, bool pause, bool stop)
{
	gtk_widget_set_sensitive(run_cpu_button, run);
	gtk_widget_set_sensitive(run_cpu_menu_item, run);
	gtk_widget_set_sensitive(pause_cpu_button, pause);
	gtk_widget_set_sensitive(pause_cpu_menu_item, pause);
	gtk_widget_set_sensitive(stop_cpu_button, stop);
	gtk_widget_set_sensitive(stop_cpu_menu_item, stop);
}

static void open_elf_file(const char* filename)
{
	// FIXME ask user to stop simulation if running
	MenuStop(NULL, NULL);
	try
	{
		main_system.LoadELFFile(filename);
		main_system.Reset();
	}
	catch(const LoadELFFileError& err)
	{
		ShowErrorMessage(err.msg.c_str());
	}
}

static void open_sdf_file(const char* filename)
{
	try
	{
		main_system.LoadSystemDescriptionFile(filename);
		main_system.Reset();
	}
	catch(const ParsingError& err)
	{
		//puts(err.msg.c_str());
		ShowErrorMessage(err.msg.c_str());
	}
	catch(const InitBoardError& err)
	{
		//puts(err.msg.c_str());
		ShowErrorMessage(err.msg.c_str());
	}
}

/*
 * edited()
 * 
 * Callback function when the user changes a register.
 */
extern "C"
G_MODULE_EXPORT
void edited(GtkCellRendererText *renderer, gchar* path, gchar* new_text, gpointer user_data)
{
	if(!main_system.HasCPU())
		return;
	
	int col = (renderer == col_right_regs)*2+1;
	int row = atoi(path);
	
	if(col == 3 && row == 18)
		return;
	
	int val = 0;
	if(sscanf(new_text, "%10x", &val) != 1)
		return;
	
	char buf[16];
	sprintf(buf, "0x%.8X", val);
	
	gtk_list_store_set(reg_list_store, &reg_iters[row],
		col, buf, -1);
	
	CCpu *cpu = main_system.GetCPU(0);
	
	if(row >= 0 && row <= 15)
		if(col == 1)
			cpu->SetReg(row, val);
		else
			cpu->SetReg(row + 16, val);
	else if(row == 16 && col == 1)
		cpu->SetCtrlReg(0, val);
	else if(row == 16 && col == 3)
		cpu->SetCtrlReg(1, val);
	else if(row == 17 && col == 1)
		cpu->SetCtrlReg(3, val);
	else if(row == 17 && col == 3)
		cpu->SetCtrlReg(4, val);
	else if(row == 18 && col == 1)
		cpu->SetPC(val);
}

extern "C"
{

G_MODULE_EXPORT
void MenuOpenFile(gpointer sender, gpointer user_data)
{
	GtkWidget *dialog;
	GtkFileFilter *filter;
	
	dialog = gtk_file_chooser_dialog_new("Open ELF File",
		GTK_WINDOW(main_window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);
	
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "*.elf");
	gtk_file_filter_add_pattern(filter, "*.elf");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		last_elf_file = filename;
		open_elf_file(filename);
		g_free(filename);
		
		SetSensitiveButtons(true, false, false);
	}
	
	gtk_widget_destroy(dialog);
}

G_MODULE_EXPORT
void MenuLoadSystemDescription(gpointer sender, gpointer user_data)
{
	GtkWidget *dialog;
	GtkFileFilter *filter;
	
	dialog = gtk_file_chooser_dialog_new("Open SDF File",
		GTK_WINDOW(main_window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);
	
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "*.sdf");
	gtk_file_filter_add_pattern(filter, "*.sdf");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	
	bool ok = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT;
	gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	
	gtk_widget_destroy(dialog);
	
	if(ok)
	{
		open_sdf_file(filename);
	}
	g_free(filename);
}

G_MODULE_EXPORT
void MenuGenerateTraceFile(gpointer sender, gpointer user_data)
{
	static bool avoid_recursion_state = false;
	
	if(avoid_recursion_state)
		return;
	
	avoid_recursion_state = true;
	
	if(main_system.IsGeneratingTraceFile())
	{
		main_system.StopGenerateTraceFile();
		gtk_toggle_tool_button_set_active(trace_button, false);
		gtk_check_menu_item_set_active(trace_menu_toggle, false);
		goto Return;
	}
	
	if(!main_system.IsELFFileLoaded())
	{
		ShowErrorMessage("No .elf file loaded.");
		gtk_toggle_tool_button_set_active(trace_button, false);
		gtk_check_menu_item_set_active(trace_menu_toggle, false);
		goto Return;
	}
	
	GtkWidget *dialog;
	GtkFileFilter *filter;
	
	dialog = gtk_file_chooser_dialog_new("Save Trace File",
		GTK_WINDOW(main_window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
		NULL);
	
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
	
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Dinero trace file (*.din)");
	gtk_file_filter_add_pattern(filter, "*.din");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		main_system.StartGenerateTraceFile(filename);
		gtk_toggle_tool_button_set_active(trace_button, true);
		gtk_check_menu_item_set_active(trace_menu_toggle, true);
		g_free(filename);
	}
	
	gtk_widget_destroy(dialog);
	
	Return:
	avoid_recursion_state = false;
}

G_MODULE_EXPORT
void MenuReloadProgramFile(gpointer sender, gpointer user_data)
{
	// FIXME Maybe ask the user to stop the system if it's running
	if(!last_elf_file.empty())
		open_elf_file(last_elf_file.c_str());
}

G_MODULE_EXPORT
void MenuRunCpu(gpointer sender, gpointer user_data)
{
	main_system.SetSimulationSpeed(gtk_check_menu_item_get_active(cpu_slow_mode_menu_item) ? SIM_SLOW : SIM_FAST);
	
	// Check if simulation is running
	if(!main_system.IsSimulationRunning())
	{
		// It is not running so lets reset the system and start the simulation
		main_system.Reset();
		main_system.StartSimulationThread();
		main_debug.Continue();
	}
	else
	{
		// Simulation is running so lets unpause it and set it in continue mode
		main_debug.Continue();
	}
	main_debug.EnableButtons();
	SetSensitiveButtons(false, true, true);
	
}

G_MODULE_EXPORT
void MenuToggleCpuSlow(gpointer sender, gpointer user_data)
{
	bool active = gtk_check_menu_item_get_active(cpu_slow_mode_menu_item);
	if(!active && main_system.IsSimulationRunning() && !main_system.IsSimulationPaused())
		main_debug.RemoveAllExecutingLineMarks();
	main_system.SetSimulationSpeed(active ? SIM_SLOW : SIM_FAST);
}

G_MODULE_EXPORT
void MenuSingleStep(gpointer sender, gpointer user_data)
{
	// Check if simulation is running
	if(!main_system.IsSimulationRunning())
	{
		// It is not running so lets reset the system and start the simulation in paused mode
		main_system.Reset();
		main_system.StartSimulationThread(true);
	}
	// Do a single step
	main_system.Step();
}

G_MODULE_EXPORT
void MenuPause(gpointer sender, gpointer user_data)
{
	if(main_system.IsSimulationRunning() && !main_system.IsSimulationPaused())
	{
		//main_system.PauseSimulationThread();
		main_debug.StepInstruction();
		gtk_check_menu_item_set_active(debug_window_toggle, TRUE);
		gtk_widget_set_visible(debug_window, TRUE);
	}
}

G_MODULE_EXPORT
void MenuStop(gpointer sender, gpointer user_data)
{
	if(main_system.IsSimulationRunning())
	{
		main_system.StopSimulation();
		main_debug.RemoveAllExecutingLineMarks();
		main_debug.DisableButtons();
		SetSensitiveButtons(true, false, false);
	}
}

G_MODULE_EXPORT
void PopupConsolesMenu(gpointer user_data, gpointer sender)
{
	GtkMenu *menu = (GtkMenu*)user_data;
	gtk_menu_popup(menu, NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time());
}

G_MODULE_EXPORT
gboolean MenuToggleIOBoardWindow(gpointer sender, gpointer user_data)
{
	bool visible = !gtk_widget_get_visible(board_window);
	gtk_check_menu_item_set_active(board_window_toggle, visible);
	gtk_widget_set_visible(board_window, visible);
	
	// Wrong image positions might be shown if we don't do this
	if(visible)
		UpdateConsolesFunc();
	
	return TRUE;
}

G_MODULE_EXPORT
gboolean MenuToggleRegisterWindow(gpointer sender, gpointer user_data)
{
	bool visible = !gtk_widget_get_visible(register_window);
	gtk_check_menu_item_set_active(register_window_toggle, visible);
	gtk_widget_set_visible(register_window, visible);
	if(visible)
		update_register_window();
	
	return TRUE;
}

G_MODULE_EXPORT
gboolean MenuJtag(gpointer sender, gpointer user_data)
{
	bool visible = !gtk_widget_get_visible(jtag_console.GetWindow());
	gtk_check_menu_item_set_active(jtag_window_toggle, visible);
	gtk_widget_set_visible(jtag_console.GetWindow(), visible);
	
	return TRUE;
}

G_MODULE_EXPORT
gboolean MenuUart0(gpointer sender, gpointer user_data)
{
	bool visible = !gtk_widget_get_visible(uart0_console.GetWindow());
	gtk_check_menu_item_set_active(uart0_window_toggle, visible);
	gtk_widget_set_visible(uart0_console.GetWindow(), visible);
	
	return TRUE;
}

G_MODULE_EXPORT
gboolean MenuUart1(gpointer sender, gpointer user_data)
{
	bool visible = !gtk_widget_get_visible(uart1_console.GetWindow());
	gtk_check_menu_item_set_active(uart1_window_toggle, visible);
	gtk_widget_set_visible(uart1_console.GetWindow(), visible);
	
	return TRUE;
}

G_MODULE_EXPORT
gboolean MenuToggleDebugWindow(gpointer sender, gpointer user_data)
{
	bool visible = !gtk_widget_get_visible(debug_window);
	gtk_check_menu_item_set_active(debug_window_toggle, visible);
	gtk_widget_set_visible(debug_window, visible);
	
	return TRUE;
}


// End extern "C"
}

static void show_about_dialog(GtkMenuItem *mnuAbout, gpointer user_data)
{
	gtk_widget_show(about_dialog);
}

void UpdateConsolesFunc()
{
	// Update JTAG console if the buffer isn't empty
	if(!jtag_console.IsBufferEmpty())
	{
		jtag_console.Update();
	}
	// Update UART0 console if the buffer isn't empty
	if(!uart0_console.IsBufferEmpty())
	{
		uart0_console.Update();
	}
	// Update UART1 console if the buffer isn't empty
	if(!uart1_console.IsBufferEmpty())
	{
		uart1_console.Update();
	}
	// Update the LCD if there is new text written to it
	if(main_board.GetLCDTextUpdate())
	{
		main_board.WriteTextToLCD();
	}
	// Update the register window if the window is visible
	if(gtk_widget_get_visible(register_window))
	{
		update_register_window();
	}
	main_board.ShowCorrectImages();
	if(main_system.HasCPU())
	{
		if(main_system.GetSimulationSpeed() == SIM_SLOW)
			main_debug.SetCurrentExecutingLineMarks(main_system.GetCPU(0)->GetPC(), false, false);
	}
	
	// Send all pending commands to the windowing system
	gdk_flush();
}

static gboolean report_error(gpointer user_data)
{
	char *msg = (char*)user_data;
	
	// Lock the GUI
	gdk_threads_enter();
	
	ShowErrorMessage(msg);
	
	// Unlock the GUI
	gdk_threads_leave();
	
	delete[] msg;
	
	return FALSE;
}

void ReportError(const char *msg)
{
	size_t len = strlen(msg)+1;
	char *buf = new char[len];
	memcpy(buf, msg, len);
	g_idle_add(report_error, buf);
}

static gboolean update_consoles_func(gpointer user_data)
{
	if(!main_system.IsSimulationRunning() || main_system.IsSimulationPaused())
		return TRUE;
	
	// Lock the GUI
	gdk_threads_enter();
	
	UpdateConsolesFunc();
	
	// Unlock the GUI
	gdk_threads_leave();
	
	return TRUE; //Do not remove
}

int main(int argc, char *argv[]){
	GtkBuilder *builder;
	GtkWidget *about_dialog_menu_item;
	
	g_thread_init(NULL);
	gdk_threads_init();
	gdk_threads_enter();
	
	gtk_init(&argc, &argv);
	
	builder = gtk_builder_new();
	
	GError *error = NULL;
	
	pair<char*, size_t> ui_xml = CFile("ui.xml.gz").decompress().read_whole_file();
	if (!gtk_builder_add_from_string(builder, ui_xml.first, ui_xml.second, &error)){
		g_print("Msg: %s\n", error->message);
		g_free(error);
		free(ui_xml.first);
		return 1;
	}
	free(ui_xml.first);
	
	main_window = GTK_WIDGET(gtk_builder_get_object(builder, "winMain"));
	gtk_builder_connect_signals(builder, NULL);
	
	register_window_toggle = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "mnuRegisters"));
	board_window_toggle = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "mnuBoard"));
	jtag_window_toggle = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "mnuJtag"));
	uart0_window_toggle = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "mnuUart0"));
	uart1_window_toggle = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "mnuUart1"));
	debug_window_toggle = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "mnuDebug"));
	
	about_dialog = GTK_WIDGET(gtk_builder_get_object(builder, "dlgAbout"));
	about_dialog_menu_item = GTK_WIDGET(gtk_builder_get_object(builder, "mnuAbout"));
	
	register_window = GTK_WIDGET(gtk_builder_get_object(builder, "winRegisters"));
	reg_tree_view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "regTreeView"));
	reg_list_store = GTK_LIST_STORE(gtk_builder_get_object(builder, "regListStore"));
	col_right_regs = GTK_CELL_RENDERER_TEXT(gtk_builder_get_object(builder, "colRightRegs"));
	
	board_window = GTK_WIDGET(gtk_builder_get_object(builder, "winBoard"));
	board_area = GTK_FIXED(gtk_builder_get_object(builder, "boardArea"));
	
	debug_window = GTK_WIDGET(gtk_builder_get_object(builder, "winDebug"));
	
	run_cpu_button = GTK_WIDGET(gtk_builder_get_object(builder, "btnRun"));
	run_cpu_menu_item = GTK_WIDGET(gtk_builder_get_object(builder, "mnuRun"));
	pause_cpu_button = GTK_WIDGET(gtk_builder_get_object(builder, "btnPause"));
	pause_cpu_menu_item = GTK_WIDGET(gtk_builder_get_object(builder, "mnuPause"));
	stop_cpu_button = GTK_WIDGET(gtk_builder_get_object(builder, "btnStop"));
	stop_cpu_menu_item = GTK_WIDGET(gtk_builder_get_object(builder, "mnuStop"));
	
	cpu_slow_mode_menu_item = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "mnuCpuSlowMode"));
	
	trace_button = GTK_TOGGLE_TOOL_BUTTON(gtk_builder_get_object(builder, "btnTrace"));
	trace_menu_toggle = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "mnuTrace"));
	
	GtkContainer *toolbar = GTK_CONTAINER(gtk_builder_get_object(builder, "mainToolbar"));
	
	GtkToolButton *board_btn = GTK_TOOL_BUTTON(gtk_builder_get_object(builder, "btnIOBoard"));
	GtkToolButton *consoles_btn = GTK_TOOL_BUTTON(gtk_builder_get_object(builder, "btnConsoles"));
	GtkToolButton *registers_btn = GTK_TOOL_BUTTON(gtk_builder_get_object(builder, "btnRegisters"));
	
	GtkImage *consoles_image = GTK_IMAGE(gtk_builder_get_object(builder, "imgConsoles"));
	
	GdkPixbuf *board_image_pixbuf = gdk_pixbuf_new_from_stream(CFile("board.png").get_input_stream(), NULL, NULL);
	GtkWidget *board_image = gtk_image_new_from_pixbuf(board_image_pixbuf);
	gtk_widget_show(board_image);
	gtk_tool_button_set_icon_widget(board_btn, board_image);
	
	GdkPixbuf *consoles_image_pixbuf = gdk_pixbuf_new_from_stream(CFile("consoles.png").get_input_stream(), NULL, NULL);
	gtk_image_set_from_pixbuf(consoles_image, consoles_image_pixbuf);
	
	GdkPixbuf *registers_image_pixbuf = gdk_pixbuf_new_from_stream(CFile("registers.png").get_input_stream(), NULL, NULL);
	GtkWidget *registers_image = gtk_image_new_from_pixbuf(registers_image_pixbuf);
	gtk_widget_show(registers_image);
	gtk_tool_button_set_icon_widget(registers_btn, registers_image);
	
	gtk_window_set_icon(GTK_WINDOW(main_window), board_image_pixbuf);
	gtk_window_set_icon(GTK_WINDOW(board_window), board_image_pixbuf);
	gtk_window_set_icon(GTK_WINDOW(register_window), registers_image_pixbuf);
	g_object_unref(board_image_pixbuf);
	g_object_unref(registers_image_pixbuf);
	
	// Due to a bug in Glade, we must set this explicitly
	GValue false_value = { 0 };
	g_value_init(&false_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&false_value, FALSE);
	gtk_container_child_set_property(toolbar, GTK_WIDGET(consoles_btn), "homogeneous", &false_value);
	g_value_unset(&false_value);
	
	jtag_console.Init(MenuJtag, consoles_image_pixbuf);
	uart0_console.Init(MenuUart0, consoles_image_pixbuf);
	uart1_console.Init(MenuUart1, consoles_image_pixbuf);
	g_object_unref(consoles_image_pixbuf);
	
	main_debug.Init(builder);
	
	
	g_object_unref(G_OBJECT(builder));
	
	g_signal_connect(G_OBJECT(about_dialog_menu_item), "activate", G_CALLBACK(show_about_dialog), NULL);
	
	InitRegisterListView();
	
	// Update the windows every 100 ms
	g_timeout_add(100, update_consoles_func, NULL);
	
	gtk_widget_show(main_window);
	
	// Open the default .sdf automatically at startup
	open_sdf_file("datorteknik.sdf");
	
	gtk_main();
	gdk_threads_leave();
	
	main_system.CloseSimulationThread();
	
	main_debug.Cleanup();
	
	return 0;
}

