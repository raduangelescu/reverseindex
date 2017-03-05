#pragma once
#include "resumefile.h"

enum EMainApp
{
	INIT,
	BUILD_COOCURANCE,
	TRAIN_GLOVE,
	QUERY_SERVER
};

class MainApp
{
	SResumeFile m_dictFileRead;
	
public:
	void Run(EMainApp type);
	void Query();
	void InitDB();
};