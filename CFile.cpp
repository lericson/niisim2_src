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

#ifdef WINNT
#include <windows.h>
#endif

#include <cstdlib>
#include <string>
#include <gio/gio.h>
#include "resources.h"
#include "CFile.h"

using namespace std;

namespace {

string file_in_current_exe_directory(const char *path)
{
	char buf[8192];
	if(path[0] == '/'){
		return string();
	}
#ifdef WINNT
	// Get the full filepath to the program
	GetModuleFileName(NULL, buf, 8192);

	int len;
	for(len=strlen(buf); len>0; len--)
	{
		if(buf[len] == '\\')
		{
			buf[len+1] = '\0';
			break;
		}
	}
#else
	ssize_t len = readlink("/proc/self/exe", buf, 8192);
	if(len == 0)
	{
		return string();
	}
	
	while(buf[--len] != '/');
#endif
	
	return string(buf, len+1) + path;
}


} // end of unnamed namespace

/*
 *  CFile::CFile()
 *
 *  Opens a file for reading
 */
CFile::CFile(const char *path)
{
	data_input_stream = NULL;
	file = g_file_new_for_path(path);
	if(!g_file_query_exists(file, NULL))
	{
		bool try_resource = false;
		g_object_unref(file);
		file = NULL;
		
		// Then look in the executable's directory
		string new_path = file_in_current_exe_directory(path);
		
		if(new_path.empty())
		{
			try_resource = true;
		}
		else
		{
			file = g_file_new_for_path(new_path.c_str());
			if(!g_file_query_exists(file, NULL))
			{
				try_resource = true;
				g_object_unref(file);
				file = NULL;
			}
		}
		
		if(try_resource)
		{
			pair<const void*, size_t> resource = ResourceFind(path);
			if(resource.first == NULL)
			{
				throw FileDoesNotExistError(path);
			}
			else
			{
				input_stream = g_memory_input_stream_new_from_data(resource.first, resource.second, NULL);
				//filesize = resource.second;
			}
			return;
		}
	}
	input_stream = G_INPUT_STREAM(g_file_read(file, NULL, NULL));
}

CFile::~CFile()
{
	if(file != NULL)
		g_object_unref(file);
	if(input_stream != NULL)
		g_object_unref(input_stream);
	if(data_input_stream != NULL)
		g_object_unref(data_input_stream);
}

bool CFile::can_seek(void)
{
	return G_IS_SEEKABLE(input_stream) && g_seekable_can_seek(G_SEEKABLE(input_stream));
}

goffset CFile::seek_tell(void)
{
	return g_seekable_tell(G_SEEKABLE(input_stream));
}

bool CFile::seek_cur(goffset offset)
{
	return g_seekable_seek(G_SEEKABLE(input_stream), offset, G_SEEK_CUR, NULL, NULL);
}

bool CFile::seek_set(goffset offset)
{
	return g_seekable_seek(G_SEEKABLE(input_stream), offset, G_SEEK_SET, NULL, NULL);
}

bool CFile::seek_end(goffset offset)
{
	return g_seekable_seek(G_SEEKABLE(input_stream), offset, G_SEEK_END, NULL, NULL);
}

/*
 *  CFile::decompress()
 *
 *  Decompress the current input stream with GZIP
 */
CFile& CFile::decompress(void)
{
	GZlibDecompressor *decompressor = g_zlib_decompressor_new(G_ZLIB_COMPRESSOR_FORMAT_GZIP);
	GInputStream *stream = g_converter_input_stream_new(input_stream, G_CONVERTER(decompressor));
	g_object_unref(decompressor);
	g_object_unref(input_stream);
	input_stream = stream;
	
	return *this;
}

/*
 *  CFile::read()
 *
 *  Reads count next bytes into the buffer, or less if it hits the end of the file
 *
 *  Returns: The number of bytes written, or -1 on error
 */
gssize CFile::read(void *buffer, gsize count)
{
	return g_input_stream_read(input_stream, buffer, count, NULL, NULL);
}

pair<char*, size_t> CFile::read_whole_file(void)
{
	if(can_seek())
	{
		seek_end(0);
		goffset len = seek_tell();
		seek_set(0);
		char *ret = (char *)malloc(len+1);
		read(ret, len);
		ret[len] = '\0';
		return make_pair(ret, len);
	}
	else
	{
		char *buf = (char *)malloc(4096);
		size_t total_read = 0;
		for(;;)
		{
			gsize bytes_read;
			if (!g_input_stream_read_all(input_stream, buf+total_read, 4096, &bytes_read, NULL, NULL))
			{
				free(buf);
				return pair<char*, size_t>(NULL, 0);
			}
			total_read += bytes_read;
			if(bytes_read < 4096)
				break;
			buf = (char *)realloc(buf, total_read + 4096);
		}
		return make_pair(buf, total_read);
	}
}

/*
 *  CFile::read_line()
 *
 *  Reads the next line of the file
 *
 *  Returns: A string that must be freed with g_free containing the line (without the newlines), or NULL on error or at end of file.
 */
char *CFile::read_line(void)
{
	if(data_input_stream == NULL)
	{
		data_input_stream = g_data_input_stream_new(input_stream);
		if(data_input_stream == NULL)
		{
			return NULL;
		}
	}
	
	return g_data_input_stream_read_line(data_input_stream, NULL, NULL, NULL);
}
