#pragma once
#include <algorithm>
#include <iterator>

#include "resumefile.h"
#include "stemmer.h"
#include "utils.h"

#include "config.h"



SResumeFile::SResumeFile()
#ifdef USE_JUDY_ARRAY
	:m_judyArrayWordCount(0)
#endif 
{
}

void SResumeFile::updateWord(std::string &wordstr, unsigned int DocumentID, unsigned int offset)
{
	if (max_document_id < DocumentID)
		max_document_id = DocumentID;

	if (wordstr.length() <= c_min_word_length)
	{
		return;
	}

	std::transform(wordstr.begin(), wordstr.end(), wordstr.begin(), ::tolower);
	bool contains_non_alpha = wordstr.find_first_not_of("abcdefghijklmnopqrstuvwxyz'") != std::string::npos;
	if (contains_non_alpha)
		return;

	Porter2Stemmer::stem(wordstr);

	if (wordstr.length() <= c_min_word_length)
	{
		return;
	}
	bool wordExistsInDb = false;


#ifdef USE_JUDY_ARRAY
	Pvoid_t getWord = NULL;
	JSLG(getWord, words, (const uint8_t*)wordstr.c_str());
	
	wordExistsInDb = (getWord != NULL);
#else
	std::map<std::string, SWordInfo*>::iterator lo = words.find(wordstr);
	wordExistsInDb = lo != words.end();
#endif



	if (!wordExistsInDb)
	{
#ifdef USE_JUDY_ARRAY
		Pvoid_t insertNewWord;

		JSLI(insertNewWord, words, (const uint8_t *)wordstr.c_str());
		SWordInfo** getWordSW = (SWordInfo**)insertNewWord;
		*getWordSW = new SWordInfo();
		(*getWordSW)->updateWordInfo(DocumentID, offset);

		m_judyArrayWordCount++;
#else
		std::pair<std::string, SWordInfo*> element;
		element.second = new SWordInfo();
		element.first = wordstr;

		element.second->updateWordInfo(DocumentID, offset);
		words.insert(element);
#endif
	}
	else
	{
		
#ifdef USE_JUDY_ARRAY
		SWordInfo** getWordInfo = (SWordInfo**)getWord;
		(*getWordInfo)->updateWordInfo(DocumentID, offset);
		
#else
		lo->second->updateWordInfo(DocumentID, offset);
#endif
	}

}
void SResumeFile::QueryRelationWords(std::vector<std::string> &inWords, std::vector<SDocumentInfo> &outWords, E_QUERYRELATION relation)
{
	std::vector<SWordInfo*> tempwords;
	//Get the word Infos
	for (unsigned int i = 0; i < inWords.size(); i++)
	{
#ifdef USE_JUDY_ARRAY
		Pvoid_t getWord = NULL;
		JSLG(getWord, words, (const uint8_t*)inWords[i].c_str());
		if (getWord)
			tempwords.push_back(*((SWordInfo**)getWord));
#else
		std::map<std::string, SWordInfo*>::iterator it = words.find(inWords[i]);
		if (it != words.end())
			tempwords.push_back(it->second);
#endif
	}
	std::vector<SDocumentInfo> tempres(tempwords[0]->documentIDS);
	std::vector<SDocumentInfo> res(tempwords[0]->documentIDS);

	auto lambdaCompareAndRank = [](SDocumentInfo& a, SDocumentInfo& b)
	{
		if (a.documentID == b.documentID)
		{
			//calculate the ranked distance
			a.rank = -1;
			b.rank = -1;
			for (unsigned int i = 0; i < a.fileOffset.size(); i++)
			{
				for (unsigned int j = 0; j < b.fileOffset.size(); j++)
				{
					unsigned int dist = abs(a.fileOffset[i] - b.fileOffset[j]);
					if (a.rank == -1 || a.rank > (int)dist)
					{
						a.rank = dist;
						b.rank = dist;
					}
				}
			}
		}

		return a.documentID < b.documentID;
	};

	for (unsigned int j = 1; j < tempwords.size(); j++)
	{
		res.clear();
		switch (relation)
		{
		case INTERSECTION:
		{
			set_intersection(tempres.begin(), tempres.end(),
				tempwords[j]->documentIDS.begin(), tempwords[j]->documentIDS.end(),
				std::back_inserter(res), lambdaCompareAndRank);
			tempres = res;
			break;
		}
		case UNION:
		{
			set_union(tempres.begin(), tempres.end(),
				tempwords[j]->documentIDS.begin(), tempwords[j]->documentIDS.end(),
				std::back_inserter(res), lambdaCompareAndRank);

			break;
		}
		case DIFFERENCE:
		{
			set_difference(tempres.begin(), tempres.end(),
				tempwords[j]->documentIDS.begin(), tempwords[j]->documentIDS.end(),
				std::back_inserter(res), lambdaCompareAndRank);
			break;
		}

		}
	}
	//sort the document ids for easy set operations
	std::sort(res.begin(), res.end(), [](const SDocumentInfo& a, const SDocumentInfo& b)
	{
		return a.rank < b.rank;
	});
	outWords = res;
}


void SResumeFile::writeBinary(std::ofstream & outf)
{
	size_t sz = 0;
#ifdef USE_JUDY_ARRAY
	sz = m_judyArrayWordCount;
#else
	sz = words.size();
#endif
	outf.write((char*)&sz, sizeof(size_t));
	//write max document id
	outf.write((char*)&max_document_id, sizeof(unsigned int));
#ifdef USE_JUDY_ARRAY
	Pcvoid_t iterValue;
	uint8_t   index[JUDY_MAX_INDEX_LEN] = { 0 };
	
	JSLF(iterValue, words, index)
	for(; iterValue != NULL; )
	{
		size_t strsize = strlen((const char*)index);
		outf.write((char*)&strsize, sizeof(size_t));
		outf.write((char*)index, strsize);
		SWordInfo** val = (SWordInfo**)iterValue;
		(*val)->writeBinary(outf);

		JSLN(iterValue, words, index);
	}
#else
	for (std::map<std::string, SWordInfo*>::iterator iterator = words.begin(); iterator != words.end(); iterator++)
	{
		size_t strsize = iterator->first.size();
		outf.write((char*)&strsize, sizeof(size_t));
		outf.write((char*)iterator->first.c_str(), strsize);
		iterator->second->writeBinary(outf);
	}
#endif
}

void SResumeFile::readBinary(std::ifstream & inf)
{
	inf.read((char*)&m_judyArrayWordCount, sizeof(size_t));
	//read max document id
	inf.read((char*)&max_document_id, sizeof(unsigned int));
	document_words.resize(max_document_id + 1);

	for (unsigned int i = 0; i < m_judyArrayWordCount; i++)
	{
		size_t sizeword;

		char tempbuffer[2048];
		inf.read((char*)&sizeword, sizeof(size_t));
		inf.read(tempbuffer, sizeword);
		
		tempbuffer[sizeword] = 0;
		size_t documentSize = 0;
#ifdef USE_JUDY_ARRAY
		Pvoid_t insertNewWord;
		JSLI(insertNewWord, words, (const uint8_t*)tempbuffer);
		SWordInfo** newItem = (SWordInfo**)insertNewWord;
		*newItem = new SWordInfo();
		(*newItem)->readBinary(inf);

		documentSize = (*newItem)->documentIDS.size();

		for (unsigned int j = 0; j < documentSize; j++)
		{
			unsigned int docid = (*newItem)->documentIDS[j].documentID;
			document_words[docid].push_back(*newItem);
		}
#else
		std::pair<std::string, SWordInfo*> newItem;

		newItem.first = std::string(tempbuffer);
		newItem.second = new SWordInfo();
		newItem.second->readBinary(inf);

		words.insert(newItem);
		documentSize = newItem.second->documentIDS.size();

		for (unsigned int j = 0; j < documentSize; j++)
		{
			unsigned int docid = newItem.second->documentIDS[j].documentID;
			documents.insert(std::pair<unsigned int, SWordInfo*>(docid, newItem.second));
		}
#endif
	

	}

}

