#pragma once
#include "Entity/Actor.h"
#include "Interfaces/IWindow.h"

class Scene;

class ItemWindow : public IWindow
{
public:
	virtual bool										Create(HINSTANCE hInstance, HWND hMainWnd) override;

	virtual void										Show()   override;
	virtual void										Hide()   override;
	virtual void										Toggle() override;
	virtual bool										IsVisible() const override { return _visible; }
	virtual HWND										GetHWnd()   const override { return _hWnd; }

	void												SetScene(Scene* scene) { _scene = scene; }

	using												ActorFactory = std::function<std::unique_ptr<Actor>()>;
	void												RegisterActor(const std::wstring& name, ActorFactory factory);

private:
	static constexpr int								COLS        = 4;
	static constexpr int								ICON_SIZE   = 200;
	static constexpr int								NAME_HEIGHT = 28;
	static constexpr int								CELL_PAD    = 10;
	static constexpr int								CELL_W      = ICON_SIZE + CELL_PAD;
	static constexpr int								CELL_H      = ICON_SIZE + NAME_HEIGHT + CELL_PAD;
	static constexpr int								GRID_MARGIN = 10;
	static constexpr int								GRID_LBL_H  = 28;
	static constexpr int								PANEL_H     = 190;

	static constexpr wchar_t							CLASS_NAME[] = L"JellytoItemWindow";
	static constexpr wchar_t							GRID_CLASS_NAME[] = L"JellytoItemGrid";

	static constexpr int								ID_BTN_PLACE = 403;
	static constexpr int								ID_EDIT_PX = 410;
	static constexpr int								ID_EDIT_PY = 411;
	static constexpr int								ID_EDIT_PZ = 412;
	static constexpr int								ID_EDIT_RX = 413;
	static constexpr int								ID_EDIT_RY = 414;
	static constexpr int								ID_EDIT_RZ = 415;
	static constexpr int								ID_EDIT_SX = 416;
	static constexpr int								ID_EDIT_SY = 417;
	static constexpr int								ID_EDIT_SZ = 418;

	void												BuildBottomPanel(int clientW, int clientH);

	static												LRESULT CALLBACK GridWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void												OnGridPaint(HDC hdc);
	void												DrawActorCell(HDC hdc, int idx, int x, int y, bool selected);
	void												OnGridLButtonDown(int mouseX, int mouseY);
	int													HitTestGrid(int mouseX, int mouseY);
	void												UpdateScrollRange();
	int													GetGridContentHeight() const;

	void												OnPlace();
	float												 ReadFloat(HWND hEdit, float fallback = 0.f);

	void												RegisterWindowClass(HINSTANCE hInstance);
	void												RegisterGridClass(HINSTANCE hInstance);
	static LRESULT CALLBACK								WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND												_hWnd      = nullptr;
	HINSTANCE											_hInstance = nullptr;
	bool												_visible   = false;
	bool												_created   = false;
	HWND												_hGridPanel = nullptr;
	int													_scrollY    = 0;

	Scene*												_scene = nullptr;

	std::vector<std::pair<std::wstring, ActorFactory>>	_catalog;
	std::vector<std::unique_ptr<Actor>>					_placedActors;

	int													_selectedIdx = -1;

	HWND												_hSelLabel = nullptr;
	HWND												_hPX = nullptr, _hPY = nullptr, _hPZ = nullptr;
	HWND												_hRX = nullptr, _hRY = nullptr, _hRZ = nullptr;
	HWND												_hSX = nullptr, _hSY = nullptr, _hSZ = nullptr;
	HWND												_hBtnPlace = nullptr;


};
