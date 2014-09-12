/*
NIISim - Nios II Simulator, A simulator that is capable of simulating various systems containing Nios II cpus.
Copyright (C) 2010,2011 Emil Bäckström
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
#include <cstring>
#include "fileparser.h"
#include "CFile.h"

/*
 *	HexToInt()
 *
 *  Converts a hex string to an integer.
 *  The hex string must not contains more than 8 digits.
 *
 *  Paramters:	hex - The string which contains the hex number
 *				len - The length of the string which contains the hex number
 *				number - A pointer to an integer which will hold the result.
 *
 *	Returns:	True if the conversion was successful. False if it failed.
 */
static bool HexToInt(const char *hex, int len, int *number)
{
	int shift = 0;

	*number = 0;

	// Return false if the hex string had more than 8 characters in it
	if(len > 8)
		return false;

	// Loop through all digits in the hex string, starting with the least significant one
	for(int i=len-1; i>=0; i--)
	{
		// Check for numbers 0-9
		if(hex[i] >= '0' && hex[i] <= '9')
		{
			*number += ((int)(hex[i] - '0')) << shift;
		}
		// Check for lower case A-F
		else if(hex[i] >= 'a' && hex[i] <= 'f')
		{
			*number += ((int)(hex[i] - 'a' + 10)) << shift;
		}
		// Check for upper case A-F
		else if(hex[i] >= 'A' && hex[i] <= 'F')
		{
			*number += ((int)(hex[i] - 'A' + 10)) << shift;
		}
		else
		{
			// Invalid character in the hex string. Return false
			*number = 0;
			return false;
		}

		// Increment shift by 4 since each hex character represents a 4 bit number
		shift +=4;
	}

	// Conversion successful. Return true
	return true;
}

struct ParsingSyntaxError
{
	int line_number;
	string msg;
	ParsingSyntaxError(int line_number, string msg) : line_number(line_number), msg(msg) {}
};

/*
 *  ParseFile()
 *
 *  Parses a description file. Each line is translated to a pair<Keyword, arguments <token type, content>>.
 *  If an integer is found, its string representation in decimal format is stored.
 *  Comments (lines starting with //) are ignored.
 *
 *  Throws a ParsingSyntaxError on syntax error.
 */
vector<pair<string, vector<pair<int, string> > > > ParseFile(const char *filename)
{
	string in_line;
	const char *line;
	vector<pair<string, vector<pair<int, string> > > > out;
	pair<string, vector<pair<int, string> > > *cur_out;
	int line_number, i, j;
	int number;
	char buf[32];
	
	// Free the string on next line input, on throwing an error, or returning from the function
	struct FreeString
	{
		char *str;
		~FreeString(){ g_free(str); }
		char *operator=(char *s){ g_free(str); return str = s; }
	} _; _.str = NULL;
	
	// Open the file
	CFile ifs(filename);
	
	// Go through each line
	while((line = _ = ifs.read_line()) != NULL)
	{
		i = 0;
		
		// Skip empty lines, line breaks and comments
		if(!line[0] || (line[0] == '/' && line[1] == '/') || line[0] == '\r' || line[0] == '\n')
			continue;
		
		out.push_back(pair<string, vector<pair<int, string> > >());
		cur_out = &out.back();
		
		// Find keyword
		while((line[i] >= 'a' && line[i] <= 'z') ||
			  (line[i] >= 'A' && line[i] <= 'Z'))
		{
			i++;
		}
		
		if(line[i] == ' ')
		{
			cur_out->first.assign(line, i);
		}
		else
		{
			throw ParsingSyntaxError(line_number, "Expected keyword");
		}
		
		line = &line[i+1];
		i = 0;
		
		for(;;)
		{
			while(line[i] == ' ' || line[i] == '\t')
			{
				i++;
			}
			line = &line[i];
			i = 0;
			
			while(line[i] != ',' && line[i] != '\r' && line[i] != '\0')
			{
				i++;
			}
			
			if(i == 0)
				break;
			
			if(line[0] == '\"' && line[i-1] == '\"')
			{
				cur_out->second.push_back(pair<int, string>(TOKEN_STRING, string(&line[1], i-2)));
			}
			else if(line[0] >= '0' && line[0] <= '9')
			{
				// Check for 0x which indicates a hex number
				if(line[0] == '0' && line[1] == 'x')
				{
					if(HexToInt(&line[2], i-2, &number) == false)
					{
						throw ParsingSyntaxError(line_number, "Invalid hexadecimal integer");
					}
					sprintf(buf, "%d", number);
					cur_out->second.push_back(pair<int, string>(TOKEN_NUMBER, string(buf)));
				}
				else
				{
					for(j=1; j<i; j++)
						if(line[j] < '0' || line[j] > '9')
							throw ParsingSyntaxError(line_number, "Invalid integer");
					cur_out->second.push_back(pair<int, string>(TOKEN_NUMBER, string(line, i)));
				}
			}
			
			if(line[i] == ',')
			{
				i++;
			}
		}
		
		line_number++;
	}
	return out;
}

/*
 *  ArgsMatches()
 *
 *  Matches the argument types according to a pattern.
 * 
 *  Each character in the pattern must consist of one of the following chars: 'u', 'n', 's',
 *  corresponding to TOKEN_UNKNOWN, TOKEN_NUMBER and TOKEN_STRING. A '?' can be added to the last arguments indicating optional.
 */
bool ArgsMatches(const ParsedRowArguments& args, const char *pattern)
{
	char apattern[256];
	int pattern_len = strlen(pattern);
	int num_varargs = 0;
	int num_required, max_args;
	
	for(int i=0, j=0; i<pattern_len; i++)
	{
		if(pattern[i] == '?')
		{
			num_varargs++;
		}
		else
		{
			apattern[j++] = pattern[i] == 's' ? TOKEN_STRING : (pattern[i] == 'n' ? TOKEN_NUMBER : TOKEN_UNKNOWN);
		}
	}
	num_required = pattern_len - 2*num_varargs;
	max_args = num_required + num_varargs;
	
	if(args.size() < num_required || max_args < args.size())
		return false;
	
	for(size_t i=0; i<args.size(); i++)
	{
		if(args[i].first != apattern[i])
			return false;
	}
	
	return true;
}

/*

//For testing:

#include <iostream>

int main(int argc, char *argv[]){
	vector<pair<string, vector<pair<int, string> > > > input = ParseFile(argv[1]);
	
	for(int i=0; i<input.size(); i++){
		cout << input[i].first << ": ";
		for(int j=0; j<input[i].second.size(); j++){
			cout << input[i].second[j].first << '.' << input[i].second[j].second << "; ";
		}
		cout << endl;
	}
}
*/
