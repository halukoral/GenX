#pragma once
#include "Application.h"
#include "Log.h"

extern Application* CreateApplication(int argc, char** argv);
bool g_ApplicationRunning = true;

int main(int argc, char** argv)
{
	while (g_ApplicationRunning)
	{
		Application* app = CreateApplication(argc, argv);
		app->Run();
		delete app;
	}
	return 0;
}
