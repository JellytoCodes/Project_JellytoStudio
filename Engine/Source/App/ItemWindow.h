#pragma once
#include "Entity/Actor.h"
#include "Interfaces/IWindow.h"

class Scene;

class ItemWindow : public IWindow
{
public:

	virtual bool Create(HINSTANCE hInstance, HWND hMainWnd) override;

	virtual void Show() override;
	virtual void Hide() override;
	virtual void Toggle() override;

	virtual bool IsVisible() const override		{ return _visible; }
	virtual HWND GetHWnd()   const override		{ return _hWnd;    }

	void SetScene(std::shared_ptr<Scene> scene) { _scene = scene; }

	using ActorFactory = std::function<std::shared_ptr<Actor>()>;
	void RegisterActor(const std::wstring& name, ActorFactory factory);

private:
	// ── 레이아웃 상수 ───────────────────────────────────────────
	static constexpr int COLS = 4;    // 한 줄 셀 수
	static constexpr int ICON_SIZE = 200;  // 아이콘 박스 크기
	static constexpr int NAME_HEIGHT = 28;   // 이름 텍스트 높이
	static constexpr int CELL_PAD = 10;   // 셀 간 패딩
	static constexpr int CELL_W = ICON_SIZE + CELL_PAD;
	static constexpr int CELL_H = ICON_SIZE + NAME_HEIGHT + CELL_PAD;
	static constexpr int GRID_MARGIN = 10;
	static constexpr int GRID_LBL_H = 28;  // 카탈로그 레이블 높이
	static constexpr int PANEL_H = 190; // 하단 고정 패널 높이

	// ── UI 구성 ─────────────────────────────────────────────────
	void BuildBottomPanel(int clientW, int clientH);

	// ── 그리드 패널 (자식 윈도우) ───────────────────────────────
	static LRESULT CALLBACK GridWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnGridPaint(HDC hdc);
	void DrawActorCell(HDC hdc, int idx, int x, int y, bool selected);
	void OnGridLButtonDown(int mouseX, int mouseY);
	int  HitTestGrid(int mouseX, int mouseY);
	void UpdateScrollRange();
	int  GetGridContentHeight() const;

	// ── 배치 ────────────────────────────────────────────────────
	void OnPlace();
	float ReadFloat(HWND hEdit, float fallback = 0.f);

	// ── 창 등록 ─────────────────────────────────────────────────
	void RegisterWindowClass(HINSTANCE hInstance);
	void RegisterGridClass(HINSTANCE hInstance);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND      _hWnd = nullptr;
	HINSTANCE _hInstance = nullptr;
	bool      _visible = false;
	bool      _created = false;

	HWND _hGridPanel = nullptr; // 스크롤 가능한 그리드 패널
	int  _scrollY = 0;       // 현재 스크롤 오프셋

	std::weak_ptr<Scene> _scene;

	std::vector<std::pair<std::wstring, ActorFactory>> _catalog;
	// 배치된 Actor 목록 (여러 개 허용)
	std::vector<std::shared_ptr<Actor>> _placedActors;
	int _selectedIdx = -1;

	// 하단 고정 패널 컨트롤
	HWND _hSelLabel = nullptr;
	HWND _hPX = nullptr, _hPY = nullptr, _hPZ = nullptr;
	HWND _hRX = nullptr, _hRY = nullptr, _hRZ = nullptr;
	HWND _hSX = nullptr, _hSY = nullptr, _hSZ = nullptr;
	HWND _hBtnPlace = nullptr;

	static constexpr wchar_t CLASS_NAME[] = L"JellytoItemWindow";
	static constexpr wchar_t GRID_CLASS_NAME[] = L"JellytoItemGrid";

	static constexpr int ID_BTN_PLACE = 403;
	static constexpr int ID_EDIT_PX = 410;
	static constexpr int ID_EDIT_PY = 411;
	static constexpr int ID_EDIT_PZ = 412;
	static constexpr int ID_EDIT_RX = 413;
	static constexpr int ID_EDIT_RY = 414;
	static constexpr int ID_EDIT_RZ = 415;
	static constexpr int ID_EDIT_SX = 416;
	static constexpr int ID_EDIT_SY = 417;
	static constexpr int ID_EDIT_SZ = 418;
};