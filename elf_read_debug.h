#ifndef _ELF_READ_DEBUG_H_
#define _ELF_READ_DEBUG_H_

#include <vector>
#include <utility>
#include <string>

typedef unsigned int uint;

using namespace std;

struct DebugInfo {
	vector<string> source_files;
	vector<vector<vector<uint> > > source_to_addr; // source_to_addr[file_id][line0] contains list of addresses
	
	vector<uint> addresses;
	vector<vector<pair<int, int> > > addr_to_source; // addr_to_source[addr index in addresses] contains source points, pair(file id, line1)
	
	vector<uint> end_of_sequence_addresses; // contains end of sequence addresses, extracted from the DWARF2 debug info
	
	// <Line1, vector<Addr>>
	pair<int, vector<uint> > GetFirstValidBreakpointLineFromLine(const string& file, int line1);
	
	// <file id, line1>, same as addr_to_source but uses the direct address instead of index in the addresses vector as argument.
	// returns an empty vector if nothing was found
	const vector<pair<int, int> >& AddrToSource(uint addr);
	
	// <<file id, line1>, addr>
	vector<pair<pair<int, int>, uint> > GetMatchingBreakpoints(int file_id, int line1);
	
	// Returns the nearest addr preceding addr (or itself) that is mapped to a source code line.
	// If there is no such addr, returns ~0
	uint GetNearestPrecedingAddrWithSourceInformation(uint addr);
};

// <<instructions, num_instructions>, base_addr>
pair<pair<uint*, size_t>, uint> ELFReadSection(const char *filedata, const char *section_name);
void BuildDebugInfo(DebugInfo& debug_info, const char *filedata);

#endif
