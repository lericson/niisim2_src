g++ *.cpp -O2 -c -mms-bitfields -ID:/gtk/include/gtksourceview-2.0 -ID:/gtk/include/libxml2 -ID:/gtk/include/gtk-2.0 -ID:/gtk/lib/gtk-2.0/include -ID:/gtk/include/atk-1.0 -ID:/gtk/include/cairo -ID:/gtk/include/gdk-pixbuf-2.0 -ID:/gtk/include/pango-1.0 -ID:/gtk/include/glib-2.0 -ID:/gtk/lib/glib-2.0/include -ID:/gtk/include -ID:/gtk/include/freetype2 -ID:/gtk/include/libpng14
g++ resource_creator.o -o resource_creator
rm resource_creator.o
./resource_creator "arrow.png" "cross.png" "pink.png" "red.png" "ui.xml.gz" "console.xml.gz" "board.png" "consoles.png" "registers.png" "datorteknik.sdf" "boards/de2.board" "images/7segled.png" "images/bg.png" "images/greenled.png" "images/pushbutton.png" "images/redled.png" "images/toggleswitch.png" > resource_data.s
gcc resource_data.s -c
g++ *.o -o prog -O2 -LD:/gtk/lib -lgtk-win32-2.0 -lglib-2.0 -lgobject-2.0 -lgio-2.0 -lgthread-2.0 -lgdk-win32-2.0 -lgtksourceview-2.0 -lgdk_pixbuf-2.0 -lpango-1.0 -mwindows
