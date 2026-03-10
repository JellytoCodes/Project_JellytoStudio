
#include "FBXConverter.h"
#include "MainApp.h"
#include "Core/Framework.h"
#include "App/Application.h"

int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	ApplicationDesc desc;

	desc.appName		= L"Jellyto Studio v0.1";
	desc.hInstance		= hInstance;
	desc.width			= MAIN_WINDOW_WIDTH;
	desc.height			= MAIN_WINDOW_HEIGHT;
	desc.app			= std::make_shared<MainApp>();

	Application app;

	if (app.Initialize(desc))
		app.Run();

	app.Shutdown();

	return 0;
}