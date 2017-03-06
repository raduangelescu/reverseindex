#include "coocurancematrix.h"
#include "resumefile.h"

CoocuranceMatrix::CoocuranceMatrix()
{
	m_numEntries = 0;
	write_CoocuranceMatrix = NULL;
}

void CoocuranceMatrix::Init(SResumeFile *dict)
{
	m_numEntries = 0;
	//Prepare a word vector
	int max_threads_num = omp_get_max_threads();

	write_CoocuranceMatrix = new std::vector<std::pair<unsigned long long, double>>[max_threads_num];
	for (unsigned int i = 0; i < max_threads_num; i++)
	{
		write_CoocuranceMatrix[i].reserve(m_words.size());
	}

#ifdef USE_JUDY_ARRAY
	Pcvoid_t iterValue;
	uint8_t   index[JUDY_MAX_INDEX_LEN] = { 0 };

	m_words.resize(dict->m_judyArrayWordCount);
	JSLF(iterValue, dict->words, index)
	for (unsigned i = 0; i< m_words.size() && iterValue != NULL; i++ )
	{
		m_words[i] = std::string((char*)index);
		JSLN(iterValue, dict->words, index);
	}
#endif
	std::sort(m_words.begin(), m_words.end());
	// iterate and compute bigram counts
	omp_lock_t readLock;
	omp_init_lock(&readLock);
	size_t done = 0;
	size_t wsize = m_words.size();
#pragma omp parallel for
	for (int i = 0; i < wsize; i++)
	{
#pragma omp atomic
		done++;
		printf(" Doing word %I64u/%I64u \r", done, wsize);

		int tid = omp_get_thread_num();
		SWordInfo *winf1 = 0;
		Pvoid_t getWord1 = NULL;
		
		omp_set_lock(&readLock);
		JSLG(getWord1, dict->words, (const uint8_t*)m_words[i].c_str());
		omp_unset_lock(&readLock);
		
		winf1 = ((SWordInfo**)getWord1)[0];

		for (unsigned int j = i+1; j < wsize; j++)
		{
			SWordInfo *winf2 = 0;
			Pvoid_t getWord2 = NULL;
			
			omp_set_lock(&readLock);
			JSLG(getWord2, dict->words, (const uint8_t*)m_words[j].c_str());
			omp_unset_lock(&readLock);
			
			size_t word_size = strlen(m_words[j].c_str());
			winf2 = ((SWordInfo**)getWord2)[0];

			//Calculate distance between word1 and word2
			float dist = winf2->countBigram(winf2, c_max_word_distance, word_size);

			if (dist <= c_min_word_distance)
				continue;

			//hash map index
			Word_t  index = GetId(i, j);
			Pvoid_t  ptr_count = NULL;
			write_CoocuranceMatrix[tid].push_back(std::pair<unsigned long long, double>(index, dist));
	
		}
	}

	printf("NUM is %I64u ", m_numEntries);
	
	printf("\nDone\n");
}


void CoocuranceMatrix::writeBinary(std::ofstream &outf)
{
	int max_threads_num = omp_get_max_threads();
	size_t size = 0;
	for (unsigned int i = 0; i < max_threads_num; i++)
	{
		size += write_CoocuranceMatrix[i].size();
	
	}
	outf.write((char*)&size, sizeof(size_t));

	for (unsigned int i = 0; i < max_threads_num; i++)
	{
		std::vector<std::pair<unsigned long long, double>> &ref = write_CoocuranceMatrix[i];
		for (unsigned int j = 0; j < ref.size(); j++)
		{
			outf.write((char*)&ref[j].first, sizeof(Word_t));
			outf.write((char*)&ref[j].second, sizeof(double));
		}
	}

}

CoocuranceMatrix::~CoocuranceMatrix()
{
	delete [] write_CoocuranceMatrix;
}

void CoocuranceMatrix::readBinary(std::ifstream &inf)
{
#ifdef USE_JUDY_ARRAY
	Pvoid_t ptr_count = NULL;
	Word_t indexW = 0;

	inf.read((char*)&m_numEntries, sizeof(unsigned int));

	for (int i =0 ; i < m_numEntries; i++ )
	{
		double count;
		unsigned long indexW;

		inf.read((char*)&indexW, sizeof(Word_t));
		inf.read((char*)&count, sizeof(double));

		Pvoid_t ptr_count;
		JLI(ptr_count, m_coocuranceMatrix, indexW);
		(*(double *)ptr_count) = count;
	}
#endif
}