#pragma once

// ItemWindow — 하우징 아이템 배치 전용 윈도우
// - 아이템 카탈로그 목록 표시
// - 선택한 아이템을 씬에 배치 / 제거

class ItemWindow
{
public:
	ItemWindow();
	~ItemWindow();

	bool Create(HINSTANCE hInstance, HWND hMainWnd);

	void Show();
	void Hide();
	void Toggle();

	bool IsVisible() const { return _visible; }
	HWND GetHWnd()   const { return _hWnd;    }

private:
	void BuildUI();

	// 이벤트
	void OnPlace();   // 선택 아이템 씬에 배치
	void OnRemove();  // 선택 아이템 제거
	void OnClear();   // 전체 제거

	void RegisterWindowClass(HINSTANCE hInstance);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND      _hWnd      = nullptr;
	HINSTANCE _hInstance = nullptr;
	bool      _visible   = false;
	bool      _created   = false;

	// 컨트롤 핸들
	HWND _hCatalogList  = nullptr;  // 아이템 카탈로그 (배치 가능 목록)
	HWND _hPlacedList   = nullptr;  // 현재 씬에 배치된 목록
	HWND _hLblCatalog   = nullptr;
	HWND _hLblPlaced    = nullptr;

	static constexpr wchar_t CLASS_NAME[] = L"JellytoItemWindow";

	static constexpr int ID_LIST_CATALOG = 401;
	static constexpr int ID_LIST_PLACED  = 402;
	static constexpr int ID_BTN_PLACE    = 403;
	static constexpr int ID_BTN_REMOVE   = 404;
	static constexpr int ID_BTN_CLEAR    = 405;
};