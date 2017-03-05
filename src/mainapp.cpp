#include "mainapp.h"
#include <fstream>
#include <string>
#include <time.h>
#include <iostream>
#include <algorithm>
#include "mainapp.h"

#include "utils.h"
#include "stemmer.h"
#include "helperstructs.h"
#include "resumefile.h"
#include "coocurancematrix.h"

#include "config.h"

void MainApp::Query()
{
	srand((unsigned int)time(NULL));


	std::vector<std::string> vec;
	std::cout<<"Insert query: ";
	std::string input;
	std::getline(std::cin, input);

	char *strinput = (char*)malloc(sizeof(char) * 1024);
	memcpy(strinput, (char*)input.c_str(), input.size());
	strinput[input.size()] = 0;
	char * word = strtok(strinput, " ,\n.");
	while (word)
	{
		std::string wordstr(word);
		Porter2Stemmer::stem(wordstr);
		std::transform(wordstr.begin(), wordstr.end(), wordstr.begin(), ::tolower);
		wordstr.erase(remove_if(wordstr.begin(), wordstr.end(), [](char c) { return !isalpha((unsigned char)c); }), wordstr.end());

		vec.push_back(wordstr);
		word = strtok(NULL, " ,\n.");
	}

	std::vector<SDocumentInfo> result;
	m_dictFileRead.QueryRelationWords(vec, result, INTERSECTION);

	FILE *outfile = fopen("results.html", "w");
	fputs("<!DOCTYPE html><html><body>", outfile);

	for (unsigned int i = 0; i < result.size() && i < 3; i++)
	{
		char filename[128];


		char formatout[c_block_size];
		char *writeout = formatout;
		sprintf(filename, "file_blocks/file%d.txt", result[i].documentID);
		RawData see;
		readFileInMemory(filename, see);
		see.data[see.size - 1] = 0;

		char * word = strtok(see.data, " ");
		while (word)
		{
			std::string wordstr(word);
			Porter2Stemmer::stem(wordstr);
			std::transform(wordstr.begin(), wordstr.end(), wordstr.begin(), ::tolower);
			wordstr.erase(remove_if(wordstr.begin(), wordstr.end(), [](char c) { return !isalpha((unsigned char)c); }), wordstr.end());

			bool found = false;
			for (unsigned int j = 0; j < vec.size(); j++)
			{
				if (vec[j].compare(wordstr) == 0)
				{
					sprintf(writeout, "<b>%s</b> ", word);
					writeout += strlen(word) + 8;
					found = true;
					break;
				}

			}
			if (!found)
			{
				sprintf(writeout, "%s ", word);
				writeout += strlen(word) + 1;
			}

			word = strtok(NULL, " ,\n.");
		}
		std::string formatoutr(formatout);

		replaceAll(formatoutr, std::string("\r"));
		replaceAll(formatoutr, std::string("\n"));

		fputs(formatoutr.c_str(), outfile);
		fputs("<br>", outfile);
		fputs("<hr>", outfile);
		fputs("<br>", outfile);
	}
	fputs(" </body></html>", outfile);
	fclose(outfile);
}

void MainApp::InitDB()
{
	//Read file names from header
	std::vector<std::string> fileNames;
	std::ifstream header("data_raw/header.txt");
	std::string line;
	std::getline(header, line);
	int size = atoi(line.c_str());
	fileNames.resize(size);
	for (int i = 0; i < size; i++)
	{
		std::getline(header, fileNames[i]);	
	}
	header.close();

	//process files
	char block[c_block_size];

	for ( int bk = 0; bk < fileNames.size(); bk++)
	{
		char fname[1024];
		std::vector<SSentence> sentences;
		RawData outData;

		unsigned int putSentenceID = 0;
		unsigned int sentenceIdx = 0;
		unsigned int sentencesNum = 0;
		sprintf(fname, "data_raw/%s", fileNames[bk].c_str());
		readFileInMemory(fname, outData);

		for (unsigned int i = 0; i < outData.size; i++)
		{
			if (outData.data[i] == '.')
			{
				sentencesNum++;
			}
		}
		sentences.resize(sentencesNum + 1);

		sentences[sentenceIdx].start = 0;
		for (unsigned int i = 0; i < outData.size; i++)
		{
			if (outData.data[i] == '.')
			{
				sentences[sentenceIdx].end = i;
				sentenceIdx++;
				sentences[sentenceIdx].start = i + 1;
			}
		}

		while (putSentenceID < sentenceIdx)
		{
			char filename[256];
			sprintf(filename, "file_blocks/file%d.txt", bk);
			FILE *fileBlock = fopen(filename, "w");

			unsigned int k = 0;
			unsigned int c_sentence_size;
			int block_size = c_block_size;

			memset(block, 0, sizeof(char) *c_block_size);

			do
			{
				c_sentence_size = sentences[putSentenceID].end - sentences[putSentenceID].start;
				block_size -= c_sentence_size;
				if (block_size - 2 < 0)
				{
					putSentenceID++;
					break;
				}

				for (unsigned int j = sentences[putSentenceID].start; j < sentences[putSentenceID].end; j++)
				{
					block[k] = outData.data[j];
					k++;
				}
				block[k] = 0;
				putSentenceID++;

			} while (putSentenceID < sentenceIdx);

			fputs(block, fileBlock);
			fclose(fileBlock);


			char * word = strtok(block, " ,\n.");

			while (word)
			{
				int current_offset = (int)(word - block);

				std::string wordstr = std::string(word);
				m_dictFileRead.updateWord(wordstr, bk, current_offset);

				word = strtok(NULL, " ,\n.\r");
			}

		}
		free(outData.data);
	}

	std::ofstream fout("dictFile.bin", std::fstream::binary);
	m_dictFileRead.writeBinary(fout);
	fout.close();
}


void MainApp::Run(EMainApp type)
{
	switch (type)
	{
		case EMainApp::INIT:
		{
			InitDB();
			break;
		}
		case EMainApp::BUILD_COOCURANCE:
		{
			//Load the dict in memory
			std::ifstream fin("dictFile.bin", std::fstream::binary);
			m_dictFileRead.readBinary(fin);
			fin.close();
			
			std::ofstream fout("coomtx.bin", std::fstream::binary);

			CoocuranceMatrix mtx;
			mtx.Init(&m_dictFileRead);
			mtx.writeBinary(fout);
			fin.close();

			bool isDone = false;
			while (!isDone)
			{
			}
			break;
		}

		case EMainApp::TRAIN_GLOVE:
		{
			std::ifstream fin("coomtx.bin", std::fstream::binary);
			CoocuranceMatrix mtx;
			mtx.readBinary(fin);

			fin.close();

			break;
		}

		case EMainApp::QUERY_SERVER:
		{
			std::ifstream fin("dictFile.bin", std::fstream::binary);
			m_dictFileRead.readBinary(fin);
			fin.close();


			bool isDone = false;
			while (!isDone)
			{
				Query();
			}

			break;
		}

	}
}