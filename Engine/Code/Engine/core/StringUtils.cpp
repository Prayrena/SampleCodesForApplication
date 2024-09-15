#include "Engine/Core/StringUtils.hpp"
#include <stdarg.h>


//-----------------------------------------------------------------------------------------------
constexpr int STRINGF_STACK_LOCAL_TEMP_LENGTH = 2048;


//-----------------------------------------------------------------------------------------------
const std::string Stringf( char const* format, ... )
{
	char textLiteral[ STRINGF_STACK_LOCAL_TEMP_LENGTH ];
	va_list variableArgumentList;
	va_start( variableArgumentList, format );
	vsnprintf_s( textLiteral, STRINGF_STACK_LOCAL_TEMP_LENGTH, _TRUNCATE, format, variableArgumentList );	
	va_end( variableArgumentList );
	textLiteral[ STRINGF_STACK_LOCAL_TEMP_LENGTH - 1 ] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

	return std::string( textLiteral );
}


//-----------------------------------------------------------------------------------------------
const std::string Stringf( int maxLength, char const* format, ... )
{
	char textLiteralSmall[ STRINGF_STACK_LOCAL_TEMP_LENGTH ];
	char* textLiteral = textLiteralSmall;
	if( maxLength > STRINGF_STACK_LOCAL_TEMP_LENGTH )
		textLiteral = new char[ maxLength ];

	va_list variableArgumentList;
	va_start( variableArgumentList, format );
	vsnprintf_s( textLiteral, maxLength, _TRUNCATE, format, variableArgumentList );	
	va_end( variableArgumentList );
	textLiteral[ maxLength - 1 ] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

	std::string returnValue( textLiteral );
	if( maxLength > STRINGF_STACK_LOCAL_TEMP_LENGTH )
		delete[] textLiteral;

	return returnValue;
}

Strings SplitStringOnDelimiter(std::string const& originalString, char delimiterToSplitOn)
{
	Strings newStrings; // list of string for output
	std::string singleString; // temp store for single string

	for (int charIndex = 0; charIndex < originalString.size(); ++charIndex)
	{
		if (originalString[charIndex] != delimiterToSplitOn)// if it is not the same as the split char, 
		{
			singleString.push_back(originalString[charIndex]);// push the char into the temp char array
		}
		else
		{
			newStrings.push_back(singleString);// put the temp string into the list of string
			singleString.clear();// clear up the temp string for the next loop
		}
	}
	newStrings.push_back(singleString);// put the rest of the temp string into the strings
	return newStrings;
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// std::size_t found = originalString.find_first_of(delimiterToSplitOn);
	// int subStringStart = 0;
	// int startOfSubstr = 0;
	// int delimziterIndex = 0;
	// while (found != std::string::npos)
	// {
	// 	singleString = originalString.substr(subStringStart, found);
	// 	newStrings.push_back(singleString);
	// 	restOfString = originalString.substr((found + 1));
	// 	subStringStart = found + 1;
	// 	found = found + 1 + restOfString.find_first_of(delimiterToSplitOn);		
	// }
	// newStrings.push_back(restOfString);
	// return newStrings;
}




