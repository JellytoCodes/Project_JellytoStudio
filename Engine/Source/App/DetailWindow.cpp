#include "Framework.h"
#include "DetailWindow.h"

DetailWindow::DetailWindow() {}
DetailWindow::~DetailWindow() {}

bool DetailWindow::Create(HINSTANCE hInstance, HWND hMainWnd)
{
	if (_created) return true;
	_hInstance = hInstance;

	RegisterWindowClass(hInstance);

	RECT mainRect = {};
	if (hMainWnd) ::GetWindowRect(hMainWnd, &mainRect);
	int x = hMainWnd ? mainRect.right + 8 : CW_USEDEFAULT;
	int y = hMainWnd ? mainRect.top       : CW_USEDEFAULT;

	RECT wr = { 0, 0, 340, 580 };
	::AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	_hWnd = ::CreateWindowW(CLASS_NAME, L"Jellyto Studio - 오브젝트 상세 정보",
		WS_OVERLAPPEDWINDOW,
		x, y, wr.right - wr.left, wr.bottom - wr.top,
		hMainWnd, nullptr, hInstance, this);

	if (!_hWnd) return false;

	BuildUI();
	_created = true;
	return true;
}

void DetailWindow::Show()
{
	if (!_hWnd) return;
	::ShowWindow(_hWnd, SW_SHOW);
	::SetForegroundWindow(_hWnd);
	_visible = true;
}

void DetailWindow::Hide()
{
	if (!_hWnd) return;
	::ShowWindow(_hWnd, SW_HIDE);
	_visible = false;
}

void DetailWindow::Toggle()
{
	_visible ? Hide() : Show();
}

void DetailWindow::BuildUI()
{
	const int LX = 10;   // 레이블 X
	const int VX = 160;  // 값 X
	const int LW = 145;  // 레이블 너비
	const int VW = 160;  // 값 너비
	const int RH = 18;   // 행 높이
	const int RS = 22;   // 행 간격

	auto Lbl = [&](const wchar_t* txt, int y) {
		::CreateWindowW(L"STATIC", txt, WS_CHILD | WS_VISIBLE | SS_LEFT,
			LX, y, LW, RH, _hWnd, nullptr, _hInstance, nullptr);
	};
	auto Val = [&](HWND& out, const wchar_t* def, int y) {
		out = ::CreateWindowW(L"STATIC", def, WS_CHILD | WS_VISIBLE | SS_LEFT | SS_SUNKEN,
			VX, y, VW, RH, _hWnd, nullptr, _hInstance, nullptr);
	};
	auto Sep = [&](const wchar_t* title, int y) {
		::CreateWindowW(L"STATIC", title, WS_CHILD | WS_VISIBLE | SS_LEFT,
			LX, y, 310, RH, _hWnd, nullptr, _hInstance, nullptr);
		// 구분선 (가로 선)
		::CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
			LX, y + RH + 2, 310, 2, _hWnd, nullptr, _hInstance, nullptr);
	};

	int y = 10;

	// ── Model ────────────────────────────────────────────────────
	Sep(L"▶  Model", y);          y += RH + 8;
	Lbl(L"이름",          y); Val(_hModelName,  L"—", y); y += RS;
	Lbl(L"본(Bone) 수",   y); Val(_hBoneCount,  L"—", y); y += RS;
	Lbl(L"메시(Mesh) 수", y); Val(_hMeshCount,  L"—", y); y += RS + 8;

	// ── Animation ────────────────────────────────────────────────
	Sep(L"▶  Animation", y);      y += RH + 8;
	Lbl(L"클립 이름",    y); Val(_hAnimName,   L"—", y); y += RS;
	Lbl(L"프레임 수",    y); Val(_hFrameCount, L"—", y); y += RS;
	Lbl(L"Frame Rate",   y); Val(_hFrameRate,  L"—", y); y += RS;
	Lbl(L"Duration (s)", y); Val(_hDuration,   L"—", y); y += RS + 8;

	// ── Transform ────────────────────────────────────────────────
	Sep(L"▶  Transform", y);      y += RH + 8;
	Lbl(L"Position  X",  y); Val(_hPosX, L"0.000", y); y += RS;
	Lbl(L"Position  Y",  y); Val(_hPosY, L"0.000", y); y += RS;
	Lbl(L"Position  Z",  y); Val(_hPosZ, L"0.000", y); y += RS;
	Lbl(L"Rotation  X°", y); Val(_hRotX, L"0.000", y); y += RS;
	Lbl(L"Rotation  Y°", y); Val(_hRotY, L"0.000", y); y += RS;
	Lbl(L"Rotation  Z°", y); Val(_hRotZ, L"0.000", y); y += RS;
	Lbl(L"Scale     X",  y); Val(_hSclX, L"1.000", y); y += RS;
	Lbl(L"Scale     Y",  y); Val(_hSclY, L"1.000", y); y += RS;
	Lbl(L"Scale     Z",  y); Val(_hSclZ, L"1.000", y);
}

void DetailWindow::UpdateDetail(const DetailInfo& info)
{
	if (!_hWnd) return;

	auto Set = [](HWND h, const wchar_t* txt) {
		if (h) ::SetWindowTextW(h, txt);
	};
	auto SetF = [](HWND h, float v) {
		if (!h) return;
		wchar_t buf[32]; swprintf_s(buf, L"%.3f", v);
		::SetWindowTextW(h, buf);
	};
	auto SetI = [](HWND h, int v) {
		if (!h) return;
		wchar_t buf[16]; swprintf_s(buf, L"%d", v);
		::SetWindowTextW(h, buf);
	};

	// Model
	Set (_hModelName,  info.modelName.empty() ? L"—" : info.modelName.c_str());
	SetI(_hBoneCount,  info.boneCount);
	SetI(_hMeshCount,  info.meshCount);

	// Animation
	Set (_hAnimName,   info.animName.empty() ? L"—" : info.animName.c_str());
	SetI(_hFrameCount, info.frameCount);
	SetF(_hFrameRate,  info.frameRate);
	SetF(_hDuration,   info.duration);

	// Transform
	SetF(_hPosX, info.tx); SetF(_hPosY, info.ty); SetF(_hPosZ, info.tz);
	SetF(_hRotX, info.rx); SetF(_hRotY, info.ry); SetF(_hRotZ, info.rz);
	SetF(_hSclX, info.sx); SetF(_hSclY, info.sy); SetF(_hSclZ, info.sz);
}

void DetailWindow::ClearDetail()
{
	DetailInfo empty;
	UpdateDetail(empty);
}

void DetailWindow::RegisterWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex   = {};
	wcex.cbSize        = sizeof(WNDCLASSEX);
	wcex.style         = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc   = DetailWindow::WndProc;
	wcex.hInstance     = hInstance;
	wcex.hCursor       = ::LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = CLASS_NAME;
	::RegisterClassExW(&wcex);
}

LRESULT CALLBACK DetailWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NCCREATE)
	{
		auto cs = reinterpret_cast<LPCREATESTRUCTW>(lParam);
		::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
		return ::DefWindowProc(hWnd, msg, wParam, lParam);
	}

	DetailWindow* self = reinterpret_cast<DetailWindow*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
	if (self)
	{
		switch (msg)
		{
		case WM_CLOSE:
			self->Hide();
			return 0;
		}
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}