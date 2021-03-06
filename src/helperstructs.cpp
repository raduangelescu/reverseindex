#include "helperstructs.h"
#include <algorithm>
#include <assert.h>

SWordInfo::SWordInfo() 
{
}

bool document_compare(const SDocumentInfo&i, const SDocumentInfo&j) { return (i.documentID<j.documentID); }
//documents ids should be in order
SDocumentInfo* SWordInfo::getDocumentInfo(unsigned int did)
{
	SDocumentInfo search;
	search.documentID = did;
	std::vector<SDocumentInfo>::iterator first = std::lower_bound(documentIDS.begin(), documentIDS.end(), search, document_compare);

	if(first == documentIDS.end() || (*first).documentID != did)
		return NULL;

	return &(*first);
}

void SWordInfo::updateWordInfo(unsigned int did, unsigned int wordoffset)
{
	SDocumentInfo *currentInfo = getDocumentInfo(did);
	if (!currentInfo)
	{
		documentIDS.push_back(SDocumentInfo());
		currentInfo = &documentIDS[documentIDS.size() - 1];
	}
	currentInfo->documentID = did;
	currentInfo->fileOffset.push_back(wordoffset);
}

float SWordInfo::countBigram(SWordInfo *other,int maxdst,int offset)
{
	float count = 0.0f;
	for (int i = 0; i < documentIDS.size(); i++)
	{
		SDocumentInfo search;
		search.documentID = i;
		std::vector<SDocumentInfo>::iterator first = std::lower_bound(other->documentIDS.begin(), other->documentIDS.end(), search, document_compare);

		if (first == other->documentIDS.end())
			continue;
		if (first->documentID != i)
			continue;
		
		SDocumentInfo &found = *first;
		SDocumentInfo &thisdoc = documentIDS[i];

		std::vector<unsigned short> &offsets = first->fileOffset;

		for (int j = 0; j < thisdoc.fileOffset.size(); j++)
		{
			short val = thisdoc.fileOffset[j];
			if (val > offsets[offsets.size() - 1])
				break;

			std::vector<unsigned short>::iterator f = std::upper_bound(offsets.begin(), offsets.end(), val);
			if (f != offsets.end())
			{
				unsigned int dst = abs( *f - val - offset);
				if (dst < maxdst)
				{
					count += 1.0 /(double(abs(int(dst)))); ;
				}
			}
		}

	}
	return count;
}

//Serialize/Deserialize structure
void SWordInfo::writeBinary(std::ofstream & outf)
{
	//document ids are sorted (id generation is monotonous)
	//now write to file
	size_t sz = documentIDS.size();
	outf.write((char*)&sz, sizeof(size_t));
	for (unsigned int i = 0; i < sz; i++)
	{
		SDocumentInfo &info = documentIDS[i];

		outf.write((char*)&info.documentID, sizeof(unsigned int));
		size_t szfo = info.fileOffset.size();

		outf.write((char*)&szfo, sizeof(size_t));

		for (unsigned int j = 0; j < szfo; j++)
		{
			outf.write((char*)&info.fileOffset[j], sizeof(unsigned short));
		}
	}
}

void SWordInfo::readBinary(std::ifstream & inf)
{
	size_t fsize = 0;
	inf.read((char*)&fsize, sizeof(size_t));
	documentIDS.resize(fsize);
	for (unsigned int i = 0; i < documentIDS.size(); i++)
	{
		SDocumentInfo &info = documentIDS[i];
		inf.read((char*)& info.documentID, sizeof(unsigned int));
		size_t szfo;
		inf.read((char*)& szfo, sizeof(size_t));
		info.fileOffset.resize(szfo);

		for (unsigned int j = 0; j < szfo; j++)
		{
			inf.read((char*)&info.fileOffset[j], sizeof(unsigned short));
		}

	}
}
