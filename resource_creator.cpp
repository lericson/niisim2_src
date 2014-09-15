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

#include <string>
#include <iostream>

using namespace std;

static void int_to_string(unsigned int i, string& out){
	if (i / 1000000000U != 0){
		out += (i / 1000000000U) + '0';
		i %= 1000000000U;
	}
	if (i / 100000000U != 0){
		out += (i / 100000000U) + '0';
		i %= 100000000U;
	}
	if (i / 10000000U != 0){
		out += (i / 10000000U) + '0';
		i %= 10000000U;
	}
	if (i / 1000000U != 0){
		out += (i / 1000000U) + '0';
		i %= 1000000U;
	}
	if (i / 100000U != 0){
		out += (i / 100000U) + '0';
		i %= 100000U;
	}
	if (i / 10000U != 0){
		out += (i / 10000U) + '0';
		i %= 10000U;
	}
	if (i / 1000U != 0){
		out += (i / 1000U) + '0';
		i %= 1000U;
	}
	if (i / 100U != 0){
		out += (i / 100U) + '0';
		i %= 100U;
	}
	if (i / 10U != 0){
		out += (i / 10U) + '0';
		i %= 10U;
	}
	out += i + '0';
}

static void escaped_string(const char* str, string& out){
	out += '"';
	while(*str != '\0'){
		if ((*str >= '0' && *str <= '9') || (*str >= 'a' && *str <= 'z') || (*str >= 'A' && *str <= 'Z') || *str == '.' || *str == '_' || *str == '/'){
			out += *str;
		} else {
			out += '\\';
			out += (*str >> 6) + '0';
			out += (*str >> 3 & 7) + '0';
			out += (*str & 7) + '0';
		}
		str++;
	}
	out += '"';
}

#ifndef __linux
#define PERIOD ""
#define UNDERSCORE "_"
#else
#define PERIOD "."
#define UNDERSCORE ""
#endif

int main(int argc, char *argv[]){
	const char *word = (sizeof(void*) == 8) ? "	.quad	" : "	.long	";
	string out =
	".globl " UNDERSCORE "resource_file_names\n"
#ifndef __posix
	"	.section	.rdata,\"dr\"\n"
#else
	"	.section	.rodata\n"
#endif
	"	.align 16\n"
#ifdef __posix
	"	.type	resource_file_names, @object\n"
	"	.size	resource_file_names, ";
	int_to_string(sizeof(void*) * (argc-1+1), out)
#endif
	;
	out += "\n" UNDERSCORE "resource_file_names:\n";
	
	for(int i=1; i<argc; i++){
		out += word;
		out += PERIOD "Lresource_file_name_";
		int_to_string(i, out);
		out += '\n';
	}
	out += word;
	out += "0\n";
	
	out += ".globl " UNDERSCORE "resource_data\n"
#ifdef __linux
	"	.type	resource_data, @object\n"
	"	.size	resource_data, ";
	int_to_string(sizeof(void*) * (argc-1), out)
#endif
	;
	out += "\n" UNDERSCORE "resource_data:\n";
	for(int i=1; i<argc; i++){
		out += word;
		out += PERIOD "Lresource_file_data_";
		int_to_string(i, out);
		out += '\n';
	}
	
	out += ".globl " UNDERSCORE "resource_data_len\n"
#ifdef __posix
	"	.type	resource_data_len, @object\n"
	"	.size	resource_data_len, ";
	int_to_string(sizeof(void*) * (argc-1), out)
#endif
	;
	out += "\n" UNDERSCORE "resource_data_len:\n";
	for(int i=1; i<argc; i++){
		out += word;
		out += PERIOD "Lresource_file_data_end_";
		int_to_string(i, out);
		out += "-" PERIOD "Lresource_file_data_";
		int_to_string(i, out);
		out += '\n';
	}
	
	for(int i=1; i<argc; i++){
		string resource_file_name_id = PERIOD "Lresource_file_name_";
		int_to_string(i, resource_file_name_id);
		
		out += resource_file_name_id + ":\n"
		"	.string	";
		escaped_string(argv[i], out);
		out += '\n';
	}
	
	for(int i=1; i<argc; i++){
		out += "	.align 8\n"
		PERIOD "Lresource_file_data_";
		int_to_string(i, out);
		out += ":\n	.incbin	";
		escaped_string(argv[i], out);
		out += "\n" PERIOD "Lresource_file_data_end_";
		int_to_string(i, out);
		out += ":\n";
	}

#ifdef __posix
	out += "	.section	.note.GNU-stack,\"\",@progbits\n";
#endif
	
	cout << out;
}