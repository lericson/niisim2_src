CXXFLAGS=-O2 -pipe

all: gtk_main.o CBoard.o CBoardDevice.o CBoardDeviceGroup.o CConsole.o CCpu.o CJtag.o CLcd.o CPio.o \
	CSdram.o CSystem.o CTimer.o CUart.o CDebug.o CThread.o CFile.o resources.o fileparser.o elf_read_debug.o disassembler.o resource_data.o
	
	g++ gtk_main.o CBoard.o CBoardDevice.o CBoardDeviceGroup.o CConsole.o CCpu.o CJtag.o CLcd.o CPio.o \
	CSdram.o CSystem.o CTimer.o CUart.o CDebug.o CThread.o CFile.o resources.o fileparser.o elf_read_debug.o disassembler.o resource_data.o -o prog \
	`pkg-config gtk+-2.0 gmodule-2.0 gio-2.0 gthread-2.0 gtksourceview-2.0 --libs`


CBoard.o: CBoard.cpp
	g++ CBoard.cpp -c `pkg-config gtk+-2.0 --cflags` $(CXXFLAGS)

CBoardDevice.o: CBoardDevice.cpp
	g++ CBoardDevice.cpp -c `pkg-config gtk+-2.0 --cflags` $(CXXFLAGS)

CBoardDeviceGroup.o: CBoardDeviceGroup.cpp
	g++ CBoardDeviceGroup.cpp -c `pkg-config gtk+-2.0 --cflags` $(CXXFLAGS)

CConsole.o: CConsole.cpp
	g++ CConsole.cpp -c `pkg-config gtk+-2.0 --cflags` $(CXXFLAGS)

CCpu.o: CCpu.cpp
	g++ CCpu.cpp -c `pkg-config gtk+-2.0 --cflags` $(CXXFLAGS)

CJtag.o: CJtag.cpp
	g++ CJtag.cpp -c `pkg-config gtk+-2.0 --cflags` $(CXXFLAGS)

CLcd.o: CLcd.cpp
	g++ CLcd.cpp -c `pkg-config gtk+-2.0 --cflags` $(CXXFLAGS)

CPio.o: CPio.cpp
	g++ CPio.cpp -c `pkg-config gtk+-2.0 --cflags` $(CXXFLAGS)

CSdram.o: CSdram.cpp
	g++ CSdram.cpp -c `pkg-config gtk+-2.0 --cflags` $(CXXFLAGS)

CSystem.o: CSystem.cpp
	g++ CSystem.cpp -c `pkg-config gtk+-2.0 --cflags` $(CXXFLAGS)

CTimer.o: CTimer.cpp
	g++ CTimer.cpp -c `pkg-config gtk+-2.0 --cflags` $(CXXFLAGS)

CUart.o: CUart.cpp
	g++ CUart.cpp -c `pkg-config gtk+-2.0 --cflags` $(CXXFLAGS)

CDebug.o: CDebug.cpp
	g++ CDebug.cpp -c `pkg-config gtk+-2.0 gtksourceview-2.0 --cflags` -I. $(CXXFLAGS)

CThread.o: CThread.cpp
	g++ CThread.cpp -c `pkg-config gtk+-2.0 --cflags` $(CXXFLAGS)

CFile.o: CFile.cpp
	g++ CFile.cpp -c `pkg-config gio-2.0 --cflags` $(CXXFLAGS)

resources.o: resources.cpp
	g++ resources.cpp -c $(CXXFLAGS)

fileparser.o: fileparser.cpp
	g++ fileparser.cpp -c `pkg-config gtk+-2.0 --cflags` $(CXXFLAGS)

elf_read_debug.o: elf_read_debug.cpp
	g++ elf_read_debug.cpp -c $(CXXFLAGS)

disassembler.o: disassembler.cpp
	g++ disassembler.cpp -c $(CXXFLAGS)

resource_creator: resource_creator.cpp
	g++ resource_creator.cpp -o resource_creator $(CXXFLAGS)

resource_data.s: resource_creator
	./resource_creator \
		"arrow.png" \
		"cross.png" \
		"pink.png" \
		"red.png" \
		"ui.xml.gz" \
		"console.xml.gz" \
		"board.png" \
		"consoles.png" \
		"registers.png" \
		"datorteknik.sdf" \
		"boards/de2.board" \
		"images/7segled.png" \
		"images/bg.png" \
		"images/greenled.png" \
		"images/pushbutton.png" \
		"images/redled.png" \
		"images/toggleswitch.png" \
		> resource_data.s

resource_data.o: resource_data.s
	gcc resource_data.s -c $(CXXFLAGS)

gtk_main.o: gtk_main.cpp
	g++ gtk_main.cpp -c `pkg-config gmodule-2.0 gtk+-2.0 --cflags` $(CXXFLAGS)

clean:
	rm *.o *.s resource_creator
