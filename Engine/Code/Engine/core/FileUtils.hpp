#pragma once
#include <vector>
#include <string>

bool IfThisFileCouldBeRead(std::string const& filePath);
int FileReadToBuffer(std::vector<uint8_t>& outBuffer, std::string const& filePath);
int FileReadToString(std::string& outString, std::string const& filePath);
bool FileWriteFromBuffer(std::vector<uint8_t> inBuffer, std::string const& filePathName);
bool CreateFolder(std::string filePathName);