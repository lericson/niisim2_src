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

/*

This file provides a simple platform-independent API for looking up and reading files.
Filenames with relative paths also relate to the directory where the executable file resides.

*/

#include <gio/gio.h>

struct FileDoesNotExistError
{
	string path;
	FileDoesNotExistError(string path) : path(path) {}
};

class CFile
{
	GFile *file;
	GInputStream *input_stream;
	GDataInputStream *data_input_stream;
	
public:
	~CFile();
	CFile(const char *path);
	CFile& decompress(void);
	
	GInputStream *get_input_stream(void) { return input_stream; }
	
	bool can_seek(void);
	goffset seek_tell(void);
	
	bool seek_cur(goffset offset);
	bool seek_set(goffset offset);
	bool seek_end(goffset offset);
	
	gssize read(void *buffer, gsize count);
	
	pair<char*, size_t> read_whole_file(void);
	
	char *read_line(void);
};
