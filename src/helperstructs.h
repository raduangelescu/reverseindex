#pragma once
#include <vector>
#include <fstream>


struct SSentence
{
	unsigned long start;
	unsigned long end;
};

struct SDocumentInfo
{
	int     rank;
	unsigned short documentID;
	std::vector<unsigned short> fileOffset;
};


struct SWordInfo
{
	std::vector<SDocumentInfo> documentIDS;

	SWordInfo();

	SDocumentInfo* getDocumentInfo(unsigned int did);
	//Update word info
	void updateWordInfo(unsigned int did, unsigned int wordoffset);
	//Serialize/Deserialize structure
	void writeBinary(std::ofstream & outf);
	void readBinary(std::ifstream & inf);
};

struct ResultRanked
{
	std::vector<SDocumentInfo> result;
	std::vector<double>        ranks;
};

enum E_QUERYRELATION
{
	INTERSECTION,
	UNION,
	DIFFERENCE
};
