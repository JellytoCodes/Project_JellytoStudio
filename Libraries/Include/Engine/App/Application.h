#pragma once
#include "SubWindow.h"

class IExecute;

// 메인 윈도우 메뉴 커맨드
enum class AppMenuCmd : UINT
{
	ToggleSubWindow = 1001,   // 창 > 서브 윈도우 열기/닫기
	Exit            = 1002,   // 파일 > 종료
};

struct ApplicationDesc
{
	std::shared_ptr<IExecute> app;
	std::wstring appName;
	HINSTANCE hInstance;
	HWND hWnd;
	UINT width;
	UINT height;
	bool vsync    = false;
	bool windowed = true;
};

class Application
{
public:
	bool   Initialize(const ApplicationDesc& desc);
	WPARAM Run();
	void   Shutdown();

private:
	void Update();
	void UpdateWindowTitle();
	ATOM MyRegisterClass();
	BOOL InitInstance();
	void CreateMainMenu();       // 파일 / 창 메뉴만
	void ToggleSubWindow();

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	ApplicationDesc _desc;
	SubWindow       _subWindow;
};