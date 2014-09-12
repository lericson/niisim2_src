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

#include <cstring>
#include <utility>

using namespace std;

extern "C"
{
	extern const char* const resource_file_names[];
	extern const char* const resource_data[];
	extern const size_t resource_data_len[];
}

pair<const char*, size_t> ResourceFind(const char *path)
{
	for(size_t i=0; resource_file_names[i] != NULL; i++)
	{
		if(!strcmp(resource_file_names[i], path))
		{
			return pair<const char*, size_t>(resource_data[i], resource_data_len[i]);
		}
	}
	pair<const char*, size_t> ret = pair<const char*, size_t>((const char*)0, 0);
	return ret;
}