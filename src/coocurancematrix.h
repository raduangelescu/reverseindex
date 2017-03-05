#pragma once
#include "config.h"
#include <Judy.h>
#include <vector>

#include <algorithm>   
struct SResumeFile;

struct CoocuranceMatrix
{
	size_t m_numEntries;

#ifdef USE_JUDY_ARRAY
	Pvoid_t m_coocuranceMatrix		= (Pvoid_t)NULL; 
#else
	//TODO
#endif

	std::vector<std::string> m_words;

	CoocuranceMatrix();
	void Init(SResumeFile *dict);

	//Cantor pairing function https://en.wikipedia.org/wiki/Pairing_function
	//(not ok because it generates indices bigger than 64 bits, 
	// for two 32 bit values
	/*unsigned long long GetId(long id1, long id2)
	{
	int k1 = std::max(id1, id2);
	int k2 = std::min(id1, id2);
	long sum = k1 + k2;
	return ceil(0.5 * sum * (sum + 1) + k2);
	}*/
	//Matthew Szudzik pairing function is better for this
	// http://szudzik.com/ElegantPairing.pdf
	unsigned long long GetId(unsigned long id1, unsigned long id2)
	{
		unsigned long k1 = std::max(id1, id2);
		unsigned long k2 = std::min(id1, id2);
		return k1*k1 + k1 + k2;

	}

	void writeBinary(std::ofstream &outf);
	void readBinary(std::ifstream &inf);
};