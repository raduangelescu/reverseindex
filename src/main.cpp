#include "mainapp.h"

int main()
{
	MainApp app;
	
	//app.Run(EMainApp::INIT);
	app.Run(EMainApp::BUILD_COOCURANCE);
	//app.Run(EMainApp::TRAIN_GLOVE);
	//app.Run(EMainApp::QUERY_SERVER);
    return 0;
}