#pragma once

#include <map>
#include <string>
#include <omp.h>


#include "helperstructs.h"
#include "config.h"

struct SResumeFile
{

#ifndef USE_JUDY_ARRAY
	std::map<std::string, SWordInfo*> words;
	std::map<unsigned int, SWordInfo*> documents;
#else 
	Pvoid_t words		= (Pvoid_t)NULL;  // initialize JudySL array
	
	
	unsigned int		m_judyArrayWordCount;
#endif
	std::vector<std::vector<SWordInfo*>>	document_words;
	unsigned int		max_document_id;

	SResumeFile();
	//update a word stats
	void updateWord(std::string &wordstr, unsigned int DocumentID, unsigned int offset);
	//and words
	void QueryRelationWords(std::vector<std::string> &inWords, std::vector<SDocumentInfo> &outWords, E_QUERYRELATION relation);
	

	//Serialize/Deserialize structure
	void writeBinary(std::ofstream & outf);
	void readBinary(std::ifstream & inf);
};

