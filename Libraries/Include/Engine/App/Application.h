#pragma once
#include "ToolWindow.h"
#include "ItemWindow.h"
#include "DetailWindow.h"

class IExecute;

enum class AppMenuCmd : UINT
{
	ToggleToolWindow   = 1001,  // 창 > 툴 윈도우
	ToggleItemWindow   = 1002,  // 창 > 아이템 배치
	ToggleDetailWindow = 1003,  // 창 > 오브젝트 상세
	Exit               = 1004,  // 파일 > 종료
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

	// Pick 결과를 DetailWindow에 전달 (외부에서 호출)
	DetailWindow& GetDetailWindow() { return _detailWindow; }

private:
	void Update();
	void UpdateWindowTitle();
	ATOM MyRegisterClass();
	BOOL InitInstance();
	void CreateMainMenu();

	void ToggleToolWindow();
	void ToggleItemWindow();
	void ToggleDetailWindow();

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	ApplicationDesc _desc;
	ToolWindow      _toolWindow;
	ItemWindow      _itemWindow;
	DetailWindow    _detailWindow;
};