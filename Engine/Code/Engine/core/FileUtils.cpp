#include "Engine/core/FileUtils.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/core/EngineCommon.hpp"
#include <stdio.h>
#include <Windows.h>
#include <fstream>

// if this chunk have a different version save file, skip
bool IfThisFileCouldBeRead(std::string const& filePath)
{
	// Open for read
	errno_t err;
	FILE* pFile; // like a cursor in the word document
	char const* filePathName = filePath.c_str();

	// open the file
	err = fopen_s(&pFile, filePathName, "r");
	if (err != 0)
	{
		return false;
	}
	else
	{
		err = fclose(pFile);
		return true;
	}
}

// return the number of bytes read and resize the input buffer
int FileReadToBuffer(std::vector<uint8_t>& outBuffer, std::string const& filePath)
{
	// Open for read
	errno_t err;
	FILE* pFile; // like a cursor in the word document
	char const* filePathName = filePath.c_str();

	// open the file
	err = fopen_s(&pFile, filePathName, "rb"); // b: _O_BINARY
	if (err != 0)
	{
		ERROR_AND_DIE(Stringf("The file %s could not be opened", filePath.c_str()));
	}

	// set the pointer to the end of the file
	int errorInfo;
	errorInfo = fseek(pFile, 0, SEEK_END);
	if (err != 0)
	{
		ERROR_AND_DIE("Could not read to the end of the file");
	}

	// get the file size
	long fileByteSize;
	fileByteSize = ftell(pFile);

	// resize, and copy over to the buffer
	outBuffer.resize(fileByteSize);
	fseek(pFile, 0, SEEK_SET); // put the pointer back to the beginning of the file
	fread(outBuffer.data(), sizeof(char), fileByteSize, pFile);

	// close the file
	err = fclose(pFile);
	if (err != 0)
	{
		ERROR_AND_DIE("The file just opened could not be closed");
	}

	return (int)fileByteSize;
}

// First read the file as a buffer of bytes, 
// then append a null terminator to make a c string, then make a std::string from that. 
int FileReadToString(std::string& outString, std::string const& filePath)
{
	std::vector<uint8_t> tempBuffer;	
	int fileByteSize = FileReadToBuffer(tempBuffer, filePath);
	outString.resize(fileByteSize);
	for (int i = 0; i < fileByteSize; ++i)
	{
		outString[i] = tempBuffer[i];
	}
	outString.push_back('\0'); // adding a null terminator
	return (int)outString.size();
}

bool CreateFolder(std::string filePathName)
{
	errno_t err;

	// todo:??? what is the difference between CreateDirectoryA and CreateDirectory
	err = CreateDirectoryA(LPCSTR(filePathName.c_str()), NULL);

	if (err == 0)
	{
		if (GetLastError() == ERROR_PATH_NOT_FOUND)
		{
			ERROR_AND_DIE(Stringf("Input file path %s does not exist", filePathName.c_str()));
		}	
		else if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			printf("The specified directory %s already exists", filePathName.c_str());
		}
		return false;
	}
	else
	{
		return true;
	}
}

bool FileWriteFromBuffer(std::vector<uint8_t> inBuffer, std::string const& filePathName)
{
	// Open for read
	errno_t err;
	FILE* pFile; // like a cursor in the word document
	char const* filePath = filePathName.c_str();

	// open the file
	err = fopen_s(&pFile, filePath, "wb");

	// close the file
	if (pFile == nullptr)
	{
		return false;
	}
	else
	{
		fwrite(inBuffer.data(), 1, inBuffer.size(), pFile); 
		err = fclose(pFile);
		return true;
	}
}
