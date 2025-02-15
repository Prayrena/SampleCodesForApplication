#pragma once
#include <vector>
//-----------------------------------------------------------------------------------------------
#include <string>
typedef std::vector<std::string> Strings;

//-----------------------------------------------------------------------------------------------
const std::string Stringf( char const* format, ... );
const std::string Stringf( int maxLength, char const* format, ... );

Strings SplitStringOnDelimiter(std::string const& originalString, char delimiterToSplitOn);



