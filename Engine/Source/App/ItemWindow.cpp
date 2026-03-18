#include "Framework.h"
#include "ItemWindow.h"

ItemWindow::ItemWindow() {}
ItemWindow::~ItemWindow() {}

bool ItemWindow::Create(HINSTANCE hInstance, HWND hMainWnd)
{
	if (_created) return true;
	_hInstance = hInstance;

	RegisterWindowClass(hInstance);

	RECT mainRect = {};
	if (hMainWnd) ::GetWindowRect(hMainWnd, &mainRect);
	int x = hMainWnd ? mainRect.right + 8 : CW_USEDEFAULT;
	int y = hMainWnd ? mainRect.top       : CW_USEDEFAULT;

	RECT wr = { 0, 0, 560, 560 };
	::AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	_hWnd = ::CreateWindowW(CLASS_NAME, L"Jellyto Studio - 아이템 배치",
		WS_OVERLAPPEDWINDOW,
		x, y, wr.right - wr.left, wr.bottom - wr.top,
		hMainWnd, nullptr, hInstance, this);

	if (!_hWnd) return false;

	BuildUI();
	_created = true;
	return true;
}

void ItemWindow::Show()
{
	if (!_hWnd) return;
	::ShowWindow(_hWnd, SW_SHOW);
	::SetForegroundWindow(_hWnd);
	_visible = true;
}

void ItemWindow::Hide()
{
	if (!_hWnd) return;
	::ShowWindow(_hWnd, SW_HIDE);
	_visible = false;
}

void ItemWindow::Toggle()
{
	_visible ? Hide() : Show();
}

void ItemWindow::BuildUI()
{
	auto Ctrl = [&](const wchar_t* cls, const wchar_t* txt, DWORD s,
		int x, int y, int w, int h, int id) -> HWND
	{
		return ::CreateWindowW(cls, txt, WS_CHILD | WS_VISIBLE | s,
			x, y, w, h, _hWnd, (HMENU)(INT_PTR)id, _hInstance, nullptr);
	};

	// ── 아이템 카탈로그 (배치 가능 목록) ─────────────────────────
	_hLblCatalog = Ctrl(L"STATIC", L"아이템 카탈로그", SS_LEFT, 10, 10, 240, 18, 0);
	_hCatalogList = Ctrl(L"LISTBOX", L"",
		WS_BORDER | WS_VSCROLL | LBS_NOTIFY, 10, 32, 240, 440, ID_LIST_CATALOG);

	// 임시 카탈로그 항목 추가 (추후 실제 데이터로 교체)
	const wchar_t* items[] = {
		L"나무 (Tree_01)", L"바위 (Rock_01)", L"울타리 (Fence_01)",
		L"집 (House_01)",  L"가로등 (Lamp_01)", L"벤치 (Bench_01)"
	};
	for (auto& item : items)
		::SendMessage(_hCatalogList, LB_ADDSTRING, 0, (LPARAM)item);

	// ── 배치 버튼 ────────────────────────────────────────────────
	Ctrl(L"BUTTON", L"배치 →", BS_PUSHBUTTON, 265, 200, 80, 30, ID_BTN_PLACE);
	Ctrl(L"BUTTON", L"← 제거", BS_PUSHBUTTON, 265, 240, 80, 30, ID_BTN_REMOVE);
	Ctrl(L"BUTTON", L"전체 제거", BS_PUSHBUTTON, 265, 280, 80, 30, ID_BTN_CLEAR);

	// ── 배치된 아이템 목록 ───────────────────────────────────────
	_hLblPlaced = Ctrl(L"STATIC", L"배치된 아이템 [0개]", SS_LEFT, 360, 10, 180, 18, 0);
	_hPlacedList = Ctrl(L"LISTBOX", L"",
		WS_BORDER | WS_VSCROLL | LBS_NOTIFY, 360, 32, 180, 440, ID_LIST_PLACED);
}

void ItemWindow::OnPlace()
{
	int sel = (int)::SendMessage(_hCatalogList, LB_GETCURSEL, 0, 0);
	if (sel == LB_ERR) return;

	wchar_t buf[128] = {};
	::SendMessage(_hCatalogList, LB_GETTEXT, sel, (LPARAM)buf);
	::SendMessage(_hPlacedList, LB_ADDSTRING, 0, (LPARAM)buf);

	// 배치된 개수 레이블 갱신
	int cnt = (int)::SendMessage(_hPlacedList, LB_GETCOUNT, 0, 0);
	wchar_t lbl[64];
	swprintf_s(lbl, L"배치된 아이템 [%d개]", cnt);
	::SetWindowTextW(_hLblPlaced, lbl);
}

void ItemWindow::OnRemove()
{
	int sel = (int)::SendMessage(_hPlacedList, LB_GETCURSEL, 0, 0);
	if (sel == LB_ERR) return;

	::SendMessage(_hPlacedList, LB_DELETESTRING, sel, 0);

	int cnt = (int)::SendMessage(_hPlacedList, LB_GETCOUNT, 0, 0);
	wchar_t lbl[64];
	swprintf_s(lbl, L"배치된 아이템 [%d개]", cnt);
	::SetWindowTextW(_hLblPlaced, lbl);
}

void ItemWindow::OnClear()
{
	::SendMessage(_hPlacedList, LB_RESETCONTENT, 0, 0);
	::SetWindowTextW(_hLblPlaced, L"배치된 아이템 [0개]");
}

void ItemWindow::RegisterWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex   = {};
	wcex.cbSize        = sizeof(WNDCLASSEX);
	wcex.style         = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc   = ItemWindow::WndProc;
	wcex.hInstance     = hInstance;
	wcex.hCursor       = ::LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = CLASS_NAME;
	::RegisterClassExW(&wcex);
}

LRESULT CALLBACK ItemWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NCCREATE)
	{
		auto cs = reinterpret_cast<LPCREATESTRUCTW>(lParam);
		::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
		return ::DefWindowProc(hWnd, msg, wParam, lParam);
	}

	ItemWindow* self = reinterpret_cast<ItemWindow*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
	if (self)
	{
		switch (msg)
		{
		case WM_CLOSE:
			self->Hide();
			return 0;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case ID_BTN_PLACE:  self->OnPlace();  return 0;
			case ID_BTN_REMOVE: self->OnRemove(); return 0;
			case ID_BTN_CLEAR:  self->OnClear();  return 0;
			}
			break;
		}
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}