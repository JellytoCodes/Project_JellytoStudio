#include "Framework.h"
#include "ItemWindow.h"
#include "Scene/Scene.h"
#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"

bool ItemWindow::Create(HINSTANCE hInstance, HWND hMainWnd)
{
	if (_created) return true;
	_hInstance = hInstance;

	RegisterWindowClass(hInstance);
	RegisterGridClass(hInstance);

	RECT mainRect = {};
	if (hMainWnd) ::GetWindowRect(hMainWnd, &mainRect);
	int x = hMainWnd ? mainRect.right + 8 : CW_USEDEFAULT;
	int y = hMainWnd ? mainRect.top : CW_USEDEFAULT;

	int winW = COLS * CELL_W + GRID_MARGIN * 2 + ::GetSystemMetrics(SM_CXVSCROLL) + 8;
	int winH = 860;

	RECT wr = { 0, 0, winW, winH };
	::AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	_hWnd = ::CreateWindowW(CLASS_NAME, L"Jellyto Studio - 아이템 배치",
		WS_OVERLAPPEDWINDOW,
		x, y, wr.right - wr.left, wr.bottom - wr.top,
		hMainWnd, nullptr, hInstance, this);

	if (!_hWnd) return false;

	RECT cr = {};
	::GetClientRect(_hWnd, &cr);
	int cW = cr.right, cH = cr.bottom;

	int gridPanelH = cH - PANEL_H;
	_hGridPanel = ::CreateWindowW(GRID_CLASS_NAME, nullptr,
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPCHILDREN,
		0, 0, cW, gridPanelH,
		_hWnd, nullptr, hInstance, this);

	BuildBottomPanel(cW, cH);

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

void ItemWindow::RegisterActor(const std::wstring& name, ActorFactory factory)
{
	_catalog.push_back({ name, std::move(factory) });
	if (_hGridPanel)
	{
		UpdateScrollRange();
		::InvalidateRect(_hGridPanel, nullptr, TRUE);
	}
}

void ItemWindow::BuildBottomPanel(int clientW, int clientH)
{
	const int panelY = clientH - PANEL_H;
	const int M = GRID_MARGIN;
	const int LW = 58;
	const int EW = 60;
	const int EH = 24;

	::CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
		0, panelY, clientW, 2, _hWnd, nullptr, _hInstance, nullptr);

	int y = panelY + 8;

	_hSelLabel = ::CreateWindowW(L"STATIC", L"선택된 Actor: 없음",
		WS_CHILD | WS_VISIBLE | SS_LEFT,
		M, y, clientW - M * 2, EH, _hWnd, nullptr, _hInstance, nullptr);
	y += 30;

	auto Lbl = [&](const wchar_t* t, int x, int yy) {
		::CreateWindowW(L"STATIC", t, WS_CHILD | WS_VISIBLE | SS_RIGHT,
			x, yy, LW, EH, _hWnd, nullptr, _hInstance, nullptr);
		};
	auto Edt = [&](HWND& out, const wchar_t* def, int x, int yy, int id) {
		out = ::CreateWindowW(L"EDIT", def,
			WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
			x, yy, EW, EH, _hWnd, (HMENU)(INT_PTR)id, _hInstance, nullptr);
		};

	const int c0 = M;
	const int c1 = c0 + LW + 2;
	const int c2 = c1 + EW + 10;
	const int c3 = c2 + LW + 2;
	const int c4 = c3 + EW + 10;
	const int c5 = c4 + LW + 2;

	// Position
	Lbl(L"Pos X", c0, y); Edt(_hPX, L"0.0", c1, y, ID_EDIT_PX);
	Lbl(L"Pos Y", c2, y); Edt(_hPY, L"0.0", c3, y, ID_EDIT_PY);
	Lbl(L"Pos Z", c4, y); Edt(_hPZ, L"0.0", c5, y, ID_EDIT_PZ);
	y += 30;

	// Rotation
	Lbl(L"Rot X°", c0, y); Edt(_hRX, L"0.0", c1, y, ID_EDIT_RX);
	Lbl(L"Rot Y°", c2, y); Edt(_hRY, L"0.0", c3, y, ID_EDIT_RY);
	Lbl(L"Rot Z°", c4, y); Edt(_hRZ, L"0.0", c5, y, ID_EDIT_RZ);
	y += 30;

	// Scale
	Lbl(L"Scale X", c0, y); Edt(_hSX, L"1.0", c1, y, ID_EDIT_SX);
	Lbl(L"Scale Y", c2, y); Edt(_hSY, L"1.0", c3, y, ID_EDIT_SY);
	Lbl(L"Scale Z", c4, y); Edt(_hSZ, L"1.0", c5, y, ID_EDIT_SZ);
	y += 34;

	const int btnW = 140, btnH = 32;
	int btnX = (clientW - btnW) / 2;
	_hBtnPlace = ::CreateWindowW(L"BUTTON", L"씬에 배치",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		btnX, y, btnW, btnH,
		_hWnd, (HMENU)(INT_PTR)ID_BTN_PLACE, _hInstance, nullptr);
}

int ItemWindow::GetGridContentHeight() const
{
	int rows = ((int)_catalog.size() + COLS - 1) / COLS;
	return GRID_LBL_H + rows * CELL_H + GRID_MARGIN;
}

void ItemWindow::UpdateScrollRange()
{
	if (!_hGridPanel) return;

	RECT cr = {};
	::GetClientRect(_hGridPanel, &cr);
	int viewH = cr.bottom;
	int contentH = GetGridContentHeight();

	SCROLLINFO si = {};
	si.cbSize = sizeof(si);
	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	si.nMin = 0;
	si.nMax = max(0, contentH - 1);
	si.nPage = viewH;
	si.nPos = _scrollY;
	::SetScrollInfo(_hGridPanel, SB_VERT, &si, TRUE);
}

void ItemWindow::OnGridPaint(HDC hdc)
{
	const int M = GRID_MARGIN;

	RECT lblRect = { M, M - _scrollY, M + COLS * CELL_W, M + GRID_LBL_H - _scrollY };
	wchar_t lbl[64];
	swprintf_s(lbl, L"Actor 카탈로그 [%d개]  (클릭하여 선택)", (int)_catalog.size());
	HFONT lblFont = ::CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH, L"Malgun Gothic");
	HFONT oldFont = (HFONT)::SelectObject(hdc, lblFont);
	::SetBkMode(hdc, TRANSPARENT);
	::SetTextColor(hdc, RGB(30, 30, 30));
	::DrawTextW(hdc, lbl, -1, &lblRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
	::SelectObject(hdc, oldFont);
	::DeleteObject(lblFont);

	for (int i = 0; i < (int)_catalog.size(); i++)
	{
		int col = i % COLS;
		int row = i / COLS;
		int cx = M + col * CELL_W;
		int cy = GRID_LBL_H + GRID_MARGIN + row * CELL_H - _scrollY;
		DrawActorCell(hdc, i, cx, cy, i == _selectedIdx);
	}
}

void ItemWindow::DrawActorCell(HDC hdc, int idx, int x, int y, bool selected)
{
	RECT iconRect = { x, y, x + ICON_SIZE, y + ICON_SIZE };

	COLORREF bgColor = selected ? RGB(210, 230, 255) : RGB(245, 245, 245);
	COLORREF penColor = selected ? RGB(0, 100, 210) : RGB(190, 190, 190);
	int      penW = selected ? 3 : 1;

	HBRUSH bg = ::CreateSolidBrush(bgColor);
	::FillRect(hdc, &iconRect, bg);
	::DeleteObject(bg);

	HPEN pen = ::CreatePen(PS_SOLID, penW, penColor);
	HPEN old = (HPEN)::SelectObject(hdc, pen);
	::Rectangle(hdc, iconRect.left, iconRect.top, iconRect.right, iconRect.bottom);
	::SelectObject(hdc, old);
	::DeleteObject(pen);

	const std::wstring& name = _catalog[idx].first;
	wchar_t initial[2] = { name.empty() ? L'?' : name[0], L'\0' };

	HFONT iFont = ::CreateFontW(80, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH, L"Arial");
	HFONT oldF = (HFONT)::SelectObject(hdc, iFont);
	::SetBkMode(hdc, TRANSPARENT);
	::SetTextColor(hdc, selected ? RGB(0, 80, 180) : RGB(100, 100, 100));
	::DrawTextW(hdc, initial, -1, &iconRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	::SelectObject(hdc, oldF);
	::DeleteObject(iFont);

	RECT nameRect = { x, y + ICON_SIZE + 4, x + ICON_SIZE, y + ICON_SIZE + NAME_HEIGHT + 4 };

	HFONT nFont = ::CreateFontW(18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH, L"Malgun Gothic");
	HFONT oldN = (HFONT)::SelectObject(hdc, nFont);
	::SetTextColor(hdc, selected ? RGB(0, 80, 180) : RGB(40, 40, 40));
	::DrawTextW(hdc, name.c_str(), -1, &nameRect,
		DT_CENTER | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
	::SelectObject(hdc, oldN);
	::DeleteObject(nFont);
}

int ItemWindow::HitTestGrid(int mouseX, int mouseY)
{
	const int M = GRID_MARGIN;
	const int gridTop = GRID_LBL_H + GRID_MARGIN;
	int relY = mouseY + _scrollY - gridTop;
	int relX = mouseX - M;

	if (relX < 0 || relY < 0) return -1;

	int col = relX / CELL_W;
	int row = relY / CELL_H;

	if (col >= COLS) return -1;

	int idx = row * COLS + col;

	int localX = relX - col * CELL_W;
	int localY = relY - row * CELL_H;
	if (localX > ICON_SIZE || localY > ICON_SIZE) return -1;

	if (idx < 0 || idx >= (int)_catalog.size()) return -1;
	return idx;
}

void ItemWindow::OnGridLButtonDown(int mouseX, int mouseY)
{
	int idx = HitTestGrid(mouseX, mouseY);
	if (idx == _selectedIdx) return;

	_selectedIdx = idx;

	if (_hSelLabel)
	{
		std::wstring txt = (idx >= 0)
			? L"선택된 Actor: " + _catalog[idx].first
			: L"선택된 Actor: 없음";
		::SetWindowTextW(_hSelLabel, txt.c_str());
	}

	::InvalidateRect(_hGridPanel, nullptr, FALSE);
}

void ItemWindow::OnPlace()
{
	if (_selectedIdx < 0 || _selectedIdx >= (int)_catalog.size())
	{
		::MessageBoxW(_hWnd, L"배치할 Actor를 먼저 선택해주세요.",
			L"알림", MB_OK | MB_ICONINFORMATION);
		return;
	}

	auto scene = _scene;
	if (!scene)
	{
		::MessageBoxW(_hWnd,
			L"씬이 준비되지 않았습니다.\n메인 윈도우에서 씬을 먼저 초기화해주세요.",
			L"오류", MB_OK | MB_ICONWARNING);
		return;
	}

	auto actor = _catalog[_selectedIdx].second();
	if (!actor->Spawn(scene))
	{
		::MessageBoxW(_hWnd, L"Actor 스폰 실패", L"오류", MB_OK | MB_ICONERROR);
		return;
	}

	auto entity = actor->GetEntity();
	if (entity && entity->GetComponent<Transform>())
	{
		auto tf = entity->GetComponent<Transform>();
		Vec3 pos(ReadFloat(_hPX), ReadFloat(_hPY), ReadFloat(_hPZ));
		Vec3 rot(
			XMConvertToRadians(ReadFloat(_hRX)),
			XMConvertToRadians(ReadFloat(_hRY)),
			XMConvertToRadians(ReadFloat(_hRZ))
		);
		Vec3 scl(ReadFloat(_hSX, 1.f), ReadFloat(_hSY, 1.f), ReadFloat(_hSZ, 1.f));
		tf->SetLocalPosition(pos);
		tf->SetLocalRotation(rot);
		tf->SetLocalScale(scl);
	}

	_placedActors.push_back(std::move(actor));
}

float ItemWindow::ReadFloat(HWND hEdit, float fallback)
{
	if (!hEdit) return fallback;
	wchar_t buf[32] = {};
	::GetWindowTextW(hEdit, buf, 32);
	try { return std::stof(buf); }
	catch (...) { return fallback; }
}

void ItemWindow::RegisterWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = ItemWindow::WndProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = CLASS_NAME;
	::RegisterClassExW(&wcex);
}

void ItemWindow::RegisterGridClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = ItemWindow::GridWndProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = GRID_CLASS_NAME;
	::RegisterClassExW(&wcex);
}

LRESULT CALLBACK ItemWindow::GridWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = ::BeginPaint(hWnd, &ps);
			self->OnGridPaint(hdc);
			::EndPaint(hWnd, &ps);
			return 0;
		}

		case WM_LBUTTONDOWN:
			self->OnGridLButtonDown(LOWORD(lParam), HIWORD(lParam));
			return 0;

		case WM_VSCROLL:
		{
			SCROLLINFO si = {};
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			::GetScrollInfo(hWnd, SB_VERT, &si);

			int newPos = si.nPos;
			switch (LOWORD(wParam))
			{
			case SB_LINEUP:        newPos -= 30;        break;
			case SB_LINEDOWN:      newPos += 30;        break;
			case SB_PAGEUP:        newPos -= si.nPage;  break;
			case SB_PAGEDOWN:      newPos += si.nPage;  break;
			case SB_THUMBTRACK:    newPos = si.nTrackPos; break;
			case SB_THUMBPOSITION: newPos = si.nTrackPos; break;
			}
			newPos = max(0, std::min(newPos, si.nMax - (int)si.nPage + 1));

			if (newPos != self->_scrollY)
			{
				::ScrollWindow(hWnd, 0, self->_scrollY - newPos, nullptr, nullptr);
				self->_scrollY = newPos;
				si.nPos = newPos;
				si.fMask = SIF_POS;
				::SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
			}
			return 0;
		}

		case WM_MOUSEWHEEL:
		{
			int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA * (-30);
			::SendMessage(hWnd, WM_VSCROLL,
				delta > 0 ? MAKEWPARAM(SB_LINEDOWN, 0) : MAKEWPARAM(SB_LINEUP, 0),
				0);

			int absDelta = abs(GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
			for (int i = 1; i < absDelta; i++)
				::SendMessage(hWnd, WM_VSCROLL,
					delta > 0 ? MAKEWPARAM(SB_LINEDOWN, 0) : MAKEWPARAM(SB_LINEUP, 0), 0);
			return 0;
		}

		case WM_SIZE:
			self->UpdateScrollRange();
			return 0;
		}
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
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

		case WM_SIZE:
		{
			int cW = LOWORD(lParam), cH = HIWORD(lParam);
			if (self->_hGridPanel)
				::MoveWindow(self->_hGridPanel, 0, 0, cW, cH - PANEL_H, TRUE);
			return 0;
		}

		case WM_COMMAND:
			if (LOWORD(wParam) == ID_BTN_PLACE)
			{
				self->OnPlace();
				return 0;
			}
			break;
		}
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}