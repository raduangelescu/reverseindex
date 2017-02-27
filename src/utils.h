#pragma once
#include <string>

struct RawData
{
	char *data;
	unsigned int size;
};


void  readFileInMemory(const char * filename, RawData &outData);
void replaceAll(std::string &inputStr, std::string &replaceStr);