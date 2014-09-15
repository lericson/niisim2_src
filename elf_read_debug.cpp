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
#ifdef __linux
#include <elf.h>
#endif

#include <vector>
#include <string>
#include <set>
#include <algorithm>

#include <cstdio>
#include <cstdlib>

#ifndef __linux
#include "elf.h"
#endif
#include "elf_read_debug.h"

/*

This file implements reading the DWARF2 debug information from an ELF executable file according to the specification.
The extracted information is the mapping between source code lines and instruction addresses.

*/

using namespace std;

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char ubyte;
typedef signed char sbyte;

#define PACKED __attribute__((__packed__)) __attribute__((__gcc_struct__))

static unsigned long long int get_uleb128(const char *&pos)
{
	unsigned long long int res = 0;
	int bit_index = 0;
	do
	{
		res |= ((unsigned long long int)(*pos & 0x7f)) << bit_index;
		bit_index += 7;
	}
	while(*pos++ & 0x80);
	
	return res;
}

static long long int get_sleb128(const char *&pos)
{
	long long int res = 0;
	int bit_index = 0;
	do
	{
		res |= ((long long int)(*pos & 0x7f)) << bit_index;
		bit_index += 7;
	}
	while(*pos++ & 0x80);
	
	if(res & (1<<(bit_index-1)))
	{
		res |= (-1LL)<<bit_index;
	}
	
	return res;
}

struct FileName
{
	const char* filename;
	uint directory_index;
	uint last_modified;
	uint file_length;
};

struct SourceMatrixRow
{
	uint address;
	uint file;
	uint line;
	uint column;
	bool is_stmt;
	bool basic_block;
	bool end_sequence;
	bool _padding;
};

struct DebugLineEntry
{
	vector<const char*> include_directories;
	vector<FileName> file_names;
	vector<SourceMatrixRow> source_matrix;
	
	void swap(DebugLineEntry& other)
	{
		include_directories.swap(other.include_directories);
		file_names.swap(other.file_names);
		source_matrix.swap(other.source_matrix);
	}
};

DebugLineEntry DecodeDebugLineEntry(const char *debug_line_entry)
{
	DebugLineEntry ret;
	
	struct EntryHeader
	{
		uint total_length;
		ushort version;
		uint prologue_length;
		ubyte minimum_instruction_length;
		ubyte default_is_stmt;
		sbyte line_base;
		ubyte line_range;
		ubyte opcode_base;
		ubyte standard_opcode_lengths[0];
	} PACKED;
	
	const EntryHeader *header = (const EntryHeader*)debug_line_entry;
	
	vector<const char*>& include_directories = ret.include_directories;
	vector<FileName>& file_names = ret.file_names;
	
	const char *pos = debug_line_entry + sizeof(EntryHeader) + header->opcode_base-1;
	const char *end_pos = debug_line_entry + sizeof(uint) + header->total_length;
	
	for(;;)
	{
		if(*pos == '\0')
		{
			pos++;
			break;
		}
		include_directories.push_back(pos);
		pos += strlen(pos) + 1;
	}
	
	for(;;)
	{
		if(*pos == '\0')
		{
			pos++;
			break;
		}
		FileName filename;
		filename.filename = pos;
		pos += strlen(pos) + 1;
		filename.directory_index = get_uleb128(pos);
		filename.last_modified = get_uleb128(pos);
		filename.file_length = get_uleb128(pos);
		file_names.push_back(filename);
	}
	
	vector<SourceMatrixRow>& source_matrix = ret.source_matrix;
	
	SourceMatrixRow state;
	
	enum
	{
		EXTENDED = 0,
		DW_LNS_copy,
		DW_LNS_advance_pc,
		DW_LNS_advance_line,
		DW_LNS_set_file,
		DW_LNS_set_column,
		DW_LNS_negate_stmt,
		DW_LNS_set_basic_block,
		DW_LNS_const_add_pc,
		DW_LNS_fixed_advance_pc
	};
	
	enum
	{
		DW_LNE_end_sequence = 1,
		DW_LNE_set_address,
		DW_LNE_define_file
	};
	
	reset_registers:
	
	state.address = 0;
	state.file = 1;
	state.line = 1;
	state.column = 0;
	state.is_stmt = header->default_is_stmt;
	state.basic_block = 0;
	state.end_sequence = 0;
	
	while(pos < end_pos)
	{
		switch(*pos++)
		{
			case EXTENDED:
				{
					get_uleb128(pos);
					switch(*pos++)
					{
						case DW_LNE_end_sequence:
							state.end_sequence = true;
							source_matrix.push_back(state);
							goto reset_registers;
						case DW_LNE_set_address:
							state.address = *(uint*)pos;
							pos += 4;
							break;
						case DW_LNE_define_file:
						{
							FileName filename;
							filename.filename = pos;
							pos += strlen(pos) + 1;
							filename.directory_index = get_uleb128(pos);
							filename.last_modified = get_uleb128(pos);
							filename.file_length = get_uleb128(pos);
							file_names.push_back(filename);
							break;
						}
					}
				}
				break;
			default:
			{
				uint opcode = (uint)(ubyte)pos[-1];
				uint adjusted_opcode = opcode - header->opcode_base;
				state.line += header->line_base + (adjusted_opcode % header->line_range);
				state.address += (adjusted_opcode / header->line_range) * header->minimum_instruction_length;
				// Fallthrough
			}
			case DW_LNS_copy:
				source_matrix.push_back(state);
				state.basic_block = false;
				break;
			case DW_LNS_advance_pc:
				state.address += get_uleb128(pos)*header->minimum_instruction_length;
				break;
			case DW_LNS_advance_line:
				state.line += get_sleb128(pos);
				break;
			case DW_LNS_set_file:
				state.file = get_uleb128(pos);
				break;
			case DW_LNS_set_column:
				state.column = get_uleb128(pos);
				break;
			case DW_LNS_negate_stmt:
				state.is_stmt = !state.is_stmt;
				break;
			case DW_LNS_set_basic_block:
				state.basic_block = true;
				break;
			case DW_LNS_const_add_pc:
				state.address += (255-header->opcode_base) / header->line_range;
				break;
			case DW_LNS_fixed_advance_pc:
				state.address += *(ushort*)pos;
				pos += 2;
				break;
		}
	}
	
	/*for(
		int i=0;
		i<source_matrix.size();
		i++){
		const SourceMatrixRow& row = source_matrix[i];
		if(file_names[row.file-1].directory_index != 0)
			printf("%s/_/", include_directories[file_names[row.file-1].directory_index-1]);
		printf("%s	%d	%x %d %d %d %d\n", file_names[row.file-1].filename, row.line, row.address, row.column, (int)row.is_stmt, (int)row.basic_block, (int)row.end_sequence);
	}
	puts("Slut");*/
	
	return ret;
	
	//return end_pos - debug_line_entry;
}

struct DebugInfoEntry
{
	const char *name, *comp_dir;
	uint language;
	DebugLineEntry debug_line_entry;
};

DebugInfoEntry DecodeDebugInfoEntry(const char *debug_file_entry, const char *abbrev_section, const char *line_section, const char *debug_str, uint& pos_out)
{
	DebugInfoEntry ret;
	
	ret.name = NULL;
	ret.comp_dir = NULL;
	ret.language = 0;
	
	struct EntryHeader
	{
		uint length;
		ushort dwarf_version;
		uint abbrev_offset;
		ubyte target_size;
	} PACKED;
	
	const EntryHeader *header = (const EntryHeader*)debug_file_entry;
	
	const char *pos = debug_file_entry + sizeof(EntryHeader);
	
	uint abbreviation_code = get_uleb128(pos);
	if(abbreviation_code == 0)
	{
		// Null entry
		pos_out += pos - debug_file_entry;
		return ret;
	}
	
	const char *abbrev_pos = abbrev_section + header->abbrev_offset;
	
	get_uleb128(abbrev_pos); // Should be 1
	uint tag = get_uleb128(abbrev_pos); // Should DW_TAG_compilation_unit
	bool has_children = *abbrev_pos++;
	
	enum {
		DW_AT_name = 0x03,
		DW_AT_stmt_list = 0x10,
		DW_AT_language = 0x13,
		DW_AT_comp_dir = 0x1b
	};
	
	enum {
		DW_FORM_addr = 1,
		DW_FORM_block2 = 3,
		DW_FORM_block4,
		DW_FORM_data2,
		DW_FORM_data4,
		DW_FORM_data8,
		DW_FORM_string,
		DW_FORM_block,
		DW_FORM_block1,
		DW_FORM_data1,
		DW_FORM_flag,
		DW_FORM_sdata,
		DW_FORM_strp,
		DW_FORM_udata,
		DW_FORM_ref_addr,
		DW_FORM_ref1,
		DW_FORM_ref2,
		DW_FORM_ref4,
		DW_FORM_ref8,
		DW_FORM_ref_udata,
		DW_FORM_indirect
	};
	
	uint attribute_name, attribute_form;
	
	for(;;)
	{
		attribute_name = get_uleb128(abbrev_pos);
		attribute_form = get_uleb128(abbrev_pos);
		
		if((attribute_name | attribute_form) == 0)
			break;
		
		uint address;
		const char *block;
		unsigned long long block_len;
		long long int sdata;
		unsigned long long data;
		const char *str;
		bool flag;
		unsigned long long ref;
		
		switch(attribute_form)
		{
			case DW_FORM_ref_addr:
			case DW_FORM_addr:
				address = ref = *((uint*)pos);
				pos += 4;
				break;
			case DW_FORM_block2:
				block = pos+2;
				pos += (block_len = *(ushort*)pos);
				break;
			case DW_FORM_block4:
				block = pos+4;
				pos += (block_len = *(ushort*)pos);
				break;
			case DW_FORM_ref2:
			case DW_FORM_data2:
				ref = data = *((ushort*)pos);
				pos += 2;
				break;
			case DW_FORM_ref4:
			case DW_FORM_data4:
				ref = data = *((uint*)pos);
				pos += 4;
				break;
			case DW_FORM_ref8:
			case DW_FORM_data8:
				ref = data = *((unsigned long long int*)pos);
				pos += 8;
				break;
			case DW_FORM_string:
				str = pos;
				pos += strlen(str) + 1;
				break;
			case DW_FORM_block:
				block_len = get_uleb128(pos);
				block = pos;
				pos += block_len;
				break;
			case DW_FORM_block1:
				block = pos+1;
				pos += (block_len = *(ubyte*)pos);
				break;
			case DW_FORM_ref1:
			case DW_FORM_data1:
				ref = data = *((ubyte*)pos);
				pos++;
				break;
			case DW_FORM_flag:
				flag = *pos++;
				break;
			case DW_FORM_sdata:
				sdata = get_sleb128(pos);
				break;
			case DW_FORM_strp:
				str = debug_str + *((uint*)pos);
				pos += 4;
				break;
			case DW_FORM_ref_udata:
			case DW_FORM_udata:
				ref = data = get_uleb128(pos);
				break;
			case DW_FORM_indirect:
				get_uleb128(pos);
				break;
		}
		
		switch(attribute_name)
		{
			case DW_AT_stmt_list:
			{
				DebugLineEntry entry = DecodeDebugLineEntry(line_section + data);
				ret.debug_line_entry.swap(entry);
				break;
			}
			case DW_AT_name:
				ret.name = str;
				break;
			case DW_AT_comp_dir:
				ret.comp_dir = str;
				break;
			case DW_AT_language:
				ret.language = data;
				break;
		}
	}
	
	//puts(name); puts(comp_dir);
	
	pos_out += 4 + header->length;
	return ret;
}

/*
Extracts each debug info entry from an ELF file, decodes them and put the result in a vector.
*/
vector<DebugInfoEntry> ELFReadDebug(const char *filedata)
{
	const Elf32_Ehdr *elf_header = (const Elf32_Ehdr*)filedata;
	const Elf32_Shdr *section_header = (const Elf32_Shdr*)(filedata + elf_header->e_shoff);
	const Elf32_Shdr *section_header_header_names = section_header + elf_header->e_shstrndx;
	const char *header_names = filedata + section_header_header_names->sh_offset;
	
	const Elf32_Shdr *debug_line = NULL;
	const Elf32_Shdr *debug_info = NULL;
	const Elf32_Shdr *debug_abbrev = NULL;
	const Elf32_Shdr *debug_str = NULL;
	
	for(uint i=0; i<elf_header->e_shnum; i++, section_header++){
		const char *header_name = header_names + section_header->sh_name;
		
		if(!strcmp(header_name, ".debug_line"))
			debug_line = section_header;
		else if(!strcmp(header_name, ".debug_info"))
			debug_info = section_header;
		else if(!strcmp(header_name, ".debug_abbrev"))
			debug_abbrev = section_header;
		else if(!strcmp(header_name, ".debug_str"))
			debug_str = section_header;
	}
	
	/*if(debug_line != NULL)
	{
		uint size = debug_line->sh_size;
		uint pos = 0;
		while(pos < size)
		{
			pos += DecodeDebugLineEntry(filedata + debug_line->sh_offset + pos);
		}
	}*/
	
	vector<DebugInfoEntry> debug_info_entries;
	
	if(debug_info != NULL)
	{
		uint size = debug_info->sh_size;
		uint pos = 0;
		while(pos < size)
		{
			debug_info_entries.push_back(
				DecodeDebugInfoEntry(
					filedata + debug_info->sh_offset + pos,
					filedata + debug_abbrev->sh_offset,
					filedata + debug_line->sh_offset,
					filedata + debug_str->sh_offset,
					pos
				)
			);
		}
	}
	
	return debug_info_entries;
}

pair<pair<uint*, size_t>, uint> ELFReadSection(const char *filedata, const char *section_name)
{
	const Elf32_Ehdr *elf_header = (const Elf32_Ehdr*)filedata;
	const Elf32_Shdr *section_header = (const Elf32_Shdr*)(filedata + elf_header->e_shoff);
	const Elf32_Shdr *section_header_header_names = section_header + elf_header->e_shstrndx;
	const char *header_names = filedata + section_header_header_names->sh_offset;
	
	for(uint i=0; i<elf_header->e_shnum; i++, section_header++){
		const char *header_name = header_names + section_header->sh_name;
		
		if(!strcmp(header_name, section_name))
		{
			return make_pair(make_pair((uint*)(filedata + section_header->sh_offset), section_header->sh_size/sizeof(uint)), section_header->sh_addr);
		}
	}
	return make_pair(make_pair((uint*)NULL, 0), 0);
}

static string concat_path(const char *base_path, const char *include_path, const char *filename)
{
	string ret;
	
	if(include_path[0] == '/')
	{
		ret = include_path;
		if(include_path[1] != '\0')
			ret += '/';
		ret += filename;
	} else {
		string bp = base_path;
		if(bp != "/")
			bp += '/';
		size_t bp_end = bp.size()-1;
		string ip = include_path;
		if(ip != "")
			ip += '/';
		
		size_t pos = 0;
		
		while(ip.c_str()[pos] == '.' && ip.c_str()[pos+1] == '.')
		{
			pos += 3;
			bp_end--; // Remove the last / ...
			// ... and the last directory, but leave the preceding /
			while(base_path[bp_end] != '/' && base_path[bp_end] != '\\')
				bp_end--;
		}
		
		ret.assign(bp.c_str(), bp_end+1);
		ret.append(ip.c_str() + pos, ip.size() - pos);
		ret += filename;
	}
	
	// Sometimes an entry in the directory table can look like this: "../project_bsp//HAL/inc" so we fix this here:
	size_t pos_in = 0;
	size_t pos_out = 0;
	while(pos_in < ret.size())
	{
		ret[pos_out] = ret.c_str()[pos_in];
		if(ret.c_str()[pos_in] == '/' && ret.c_str()[pos_in+1] == '/')
			pos_in++;
		pos_in++;
		pos_out++;
	}
	if(pos_in != pos_out)
	{
		ret.resize(pos_out);
	}
	
	return ret;
}

/*

Uses the DWARF2 debugging information extracted by ELFReadDebug to create more useful data structures.

*/
void BuildDebugInfo(DebugInfo& debug_info, const char *filedata)
{
	// Clear old info
	debug_info = DebugInfo();
	
	set<string> source_files_set;
	vector<string>& source_files = debug_info.source_files;
	
	set<uint> addresses_set;
	vector<uint>& addresses = debug_info.addresses;
	
	vector<vector<vector<uint> > >& source_to_addr = debug_info.source_to_addr;
	vector<vector<pair<int, int> > >& addr_to_source = debug_info.addr_to_source;
	
	vector<uint>& end_of_sequence_addresses = debug_info.end_of_sequence_addresses;
	
	vector<DebugInfoEntry> die = ELFReadDebug(filedata);
	
	// First pass, finds all source file names and used addresses.
	for(size_t i=0, e=die.size(); i!=e; i++){
		DebugInfoEntry& info_entry = die[i];
		
		for(size_t i=0, e=info_entry.debug_line_entry.source_matrix.size(); i!=e; i++)
		{
			SourceMatrixRow& matrix_row = info_entry.debug_line_entry.source_matrix[i];
			
			if(matrix_row.end_sequence)
			{
				end_of_sequence_addresses.push_back(matrix_row.address);
				continue;
			}
			
			FileName& fn = info_entry.debug_line_entry.file_names[matrix_row.file-1];
			const char *include_dir = fn.directory_index != 0 ? info_entry.debug_line_entry.include_directories[fn.directory_index-1] : "";
			
			string full_source_name = concat_path(info_entry.comp_dir, include_dir, fn.filename);
			source_files_set.insert(full_source_name);
			
			addresses_set.insert(matrix_row.address);
		}
	}
	
	source_files.reserve(source_files_set.size());
	for(set<string>::iterator it = source_files_set.begin(); it != source_files_set.end(); ++it)
		source_files.push_back(*it);
	
	addresses.reserve(addresses_set.size());
	for(set<uint>::iterator it = addresses_set.begin(); it != addresses_set.end(); ++it)
		addresses.push_back(*it);
	
	source_to_addr.resize(source_files.size());
	addr_to_source.resize(addresses.size());
	
	// Second pass, puts all information in corresponding data structures.
	for(size_t i=0, e=die.size(); i!=e; i++){
		DebugInfoEntry& info_entry = die[i];
		
		for(size_t i=0, e=info_entry.debug_line_entry.source_matrix.size(); i!=e; i++)
		{
			SourceMatrixRow& matrix_row = info_entry.debug_line_entry.source_matrix[i];
			
			if(matrix_row.end_sequence)
				continue;
			
			FileName& fn = info_entry.debug_line_entry.file_names[matrix_row.file-1];
			const char *include_dir = fn.directory_index != 0 ? info_entry.debug_line_entry.include_directories[fn.directory_index-1] : "";
			
			string full_source_name = concat_path(info_entry.comp_dir, include_dir, fn.filename);
			size_t source_id = lower_bound(source_files.begin(), source_files.end(), full_source_name) - source_files.begin();
			
			// Make sure there is space
			if(source_to_addr[source_id].size() <= matrix_row.line-1)
				source_to_addr[source_id].resize(matrix_row.line);
			
			vector<uint>& v = source_to_addr[source_id][matrix_row.line-1];
			if(!count(v.begin(), v.end(), matrix_row.address))
				v.push_back(matrix_row.address);
			
			size_t address_id = lower_bound(addresses.begin(), addresses.end(), matrix_row.address) - addresses.begin();
			vector<pair<int, int> >& v2 = addr_to_source[address_id];
			if(!count(v2.begin(), v2.end(), pair<int, int>(source_id, matrix_row.line)))
				v2.push_back(pair<int, int>(source_id, matrix_row.line));
		}
	}
	
	/*for(int i=0; i<source_files.size(); i++)
	{
		puts(source_files[i].c_str());
		for(int j=1; j<=source_to_addr[i].size(); j++)
		{
			printf("%d:", j);
			for(int k=0; k<source_to_addr[i][j-1].size(); k++)
				printf(" 0x%08x", source_to_addr[i][j-1][k]);
			puts("");
		}
	}*/
	
	/*for(int i=0; i<addr_to_source.size(); i++)
	{
		printf("0x%08x", addresses[i]);
		for(int j=0; j<addr_to_source[i].size(); j++)
			printf(" %s:%d", source_files[addr_to_source[i][j].first].c_str(), addr_to_source[i][j].second);
		puts("");
	}*/
}

vector<pair<pair<int, int>, uint> > DebugInfo::GetMatchingBreakpoints(int file_id, int line1)
{
	vector<pair<pair<int, int>, uint> > ret;
	
	for(size_t i=line1-1, e=source_to_addr[file_id].size(); i<e; i++)
	{
		if(!source_to_addr[file_id][i].empty())
		{
			// First an original one
			ret.push_back(make_pair(make_pair(file_id, i+1), source_to_addr[file_id][i].front()));
			
			// Then find other lines that will also be breakpoints implicitly
			vector<uint>& found_addresses = source_to_addr[file_id][i];
			for(size_t j=0, e=found_addresses.size(); j!=e; j++)
			{
				const vector<pair<int, int> >& lines = AddrToSource(found_addresses[j]);
				for(size_t k=0, e=lines.size(); k!=e; k++)
				{
					pair<pair<int, int>, uint> p;
					p.first = lines[k];
					p.second = found_addresses[j];
					
					if(!count(ret.begin(), ret.end(), p))
						ret.push_back(p);
				}
				
			}
			break;
		}
	}
	
	return ret;
}

static const vector<pair<int, int> > empty_vector_pii;
const vector<pair<int, int> >& DebugInfo::AddrToSource(uint addr)
{
	vector<uint>::iterator it = lower_bound(addresses.begin(), addresses.end(), addr);
	if(it == addresses.end() || *it != addr)
		return empty_vector_pii;
	return addr_to_source[it - addresses.begin()];
}

uint DebugInfo::GetNearestPrecedingAddrWithSourceInformation(uint addr){
	vector<uint>::iterator it = upper_bound(addresses.begin(), addresses.end(), addr);
	if(it == addresses.begin())
		return ~0;
	--it;
	uint addr_with_source = *it;
	it = upper_bound(end_of_sequence_addresses.begin(), end_of_sequence_addresses.end(), addr_with_source);
	if(it == end_of_sequence_addresses.end())
	{
		// This should never happen, but just in case
		return addr_with_source;
	}
	if(addr < *it)
		return addr_with_source;
	else
		return ~0;
}

