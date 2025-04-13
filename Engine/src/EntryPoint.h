#pragma once
#include "Application.h"

extern Application* CreateApplication(int argc, char** argv);
bool g_ApplicationRunning = true;

int main(int argc, char** argv)
{
	//  while loop is "paused" while the run() is being called.
	//  When the run() function is completed, then the app is deleted.
	while (g_ApplicationRunning)
	{
		try
		{
			Application* app = CreateApplication(argc, argv);
			app->Run();
			delete app;
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what() << '\n';
			return EXIT_FAILURE;
		}
	}

	return 0;
}
