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

#ifndef _FILEPARSER_H_
#define _FILEPARSER_H_

#include <vector>
#include <string>
#include <utility>

using namespace std;

// Constants for string parsing
#define TOKEN_UNKNOWN	0
#define TOKEN_NUMBER	2
#define TOKEN_STRING	3

// <Keyword, arguments <token type, content>>
typedef vector<pair<string, vector<pair<int, string> > > > ParsedFile;
typedef vector<pair<int, string> > ParsedRowArguments;

ParsedFile ParseFile(const char *filename);
bool ArgsMatches(const ParsedRowArguments& args, const char *pattern);

#endif
