#pragma once
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>


void  readFileInMemory(const char * filename, RawData &outData)
{
	FILE *f = fopen(filename, "r");
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *string = (char*)malloc(fsize + 1);
	fread(string, fsize, 1, f);
	fclose(f);

	string[fsize] = 0;

	outData.data = string;
	outData.size = fsize + 1;
}

void replaceAll(std::string &inputStr, std::string &replaceStr)
{
	size_t sz = inputStr.find(replaceStr);
	while (sz != -1 && sz < inputStr.length())
	{
		inputStr.replace(sz, 1, "<br>");
		sz = inputStr.find(replaceStr);
	}
}