
#include "Framework.h"

#include "Application.h"

int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	ApplicationDesc desc;
	desc.appName = L"Jellyto Studio v1.0";
	desc.hInstance = hInstance;
	desc.width = 1280;
	desc.height = 720;

	Application app;

	if (app.Initialize(desc))
	{
		app.Run();
	}

	app.Shutdown();

	return 0;
}
