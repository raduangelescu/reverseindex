#include "coocurancematrix.h"
#include "resumefile.h"

CoocuranceMatrix::CoocuranceMatrix()
{
	m_numEntries = 0;
}

void CoocuranceMatrix::Init(SResumeFile *dict)
{
	m_numEntries = 0;
	//Prepare a word vector
	int max_threads_num = omp_get_max_threads();
#ifdef USE_JUDY_ARRAY
	Pvoid_t *tmp_CoocuranceMatrix = new Pvoid_t[max_threads_num];
	for (unsigned int i = 0; i < max_threads_num; i++)
	{
		tmp_CoocuranceMatrix[i] = (Pvoid_t)NULL;
	}

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
		int tid = omp_get_thread_num();
		Pvoid_t &tmp_cmtx = tmp_CoocuranceMatrix[tid];
		SWordInfo *winf1 = 0;
		Pvoid_t getWord1 = NULL;
		omp_set_lock(&readLock);
#pragma omp atomic
		done++;
		printf(" Doing word %I64u/%I64u \r", done, wsize);
		
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
			
			JLG(ptr_count, tmp_cmtx, index);
			if (ptr_count)
			{
				double count = (*(double *)ptr_count) + dist;
				(*(double *)ptr_count) = count;
			}
			else
			{
#pragma omp atomic
				m_numEntries++;
				JLI(ptr_count, tmp_cmtx, index);
				(*(double *)ptr_count) = 1.0;
		
			}
			
		}
	}

	printf("NUM is %I64u ", m_numEntries);

	printf("\nMerging\n");
	//Merge Judy Arrays
	for (unsigned int i = 0; i < max_threads_num; i++)
	{
		Pvoid_t ptr_count = NULL;
		Word_t indexW = 0;

		JLF(ptr_count, tmp_CoocuranceMatrix[i], indexW)
		for (; ptr_count != NULL; )
		{
			double value = *(double*)ptr_count;
			Pvoid_t entry = NULL;
			Word_t newIndex = indexW;
			JLI(entry, m_coocuranceMatrix, newIndex);
			(*(double*)entry) = value;
			JLN(ptr_count, tmp_CoocuranceMatrix[i], indexW);
		}
	}
	delete []tmp_CoocuranceMatrix;
	printf("\nDone\n");
}


void CoocuranceMatrix::writeBinary(std::ofstream &outf)
{
#ifdef USE_JUDY_ARRAY
	Pvoid_t ptr_count = NULL;
	Word_t indexW = 0;

	outf.write((char*)&m_numEntries, sizeof(unsigned int));
	JLF(ptr_count, m_coocuranceMatrix, indexW)
		for (; ptr_count != NULL; )
		{
			double* float_ = (double*)ptr_count;
			outf.write((char*)&indexW, sizeof(Word_t));
			outf.write((char*)&float_[0], sizeof(double));

			JLN(ptr_count, m_coocuranceMatrix, indexW);
		}
#endif
}

void CoocuranceMatrix::readBinary(std::ifstream &inf)
{
#ifdef USE_JUDY_ARRAY
	Pvoid_t ptr_count = NULL;
	Word_t indexW = 0;

	inf.read((char*)&m_numEntries, sizeof(size_t));

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