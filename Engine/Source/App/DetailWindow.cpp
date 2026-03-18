#include "Framework.h"
#include "DetailWindow.h"
#include "Scene/Scene.h"
#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Camera.h"
#include "Entity/Components/Collider/BaseCollider.h"
#include "Graphics/Model/ModelAnimator.h"
#include "Graphics/Model/Model.h"
#include "Entity/Components/MeshRenderer.h"

DetailWindow::DetailWindow() {}
DetailWindow::~DetailWindow() {}

// ── 생성 ────────────────────────────────────────────────────────────────

bool DetailWindow::Create(HINSTANCE hInstance, HWND hMainWnd)
{
	if (_created) return true;
	_hInstance = hInstance;

	RegisterWindowClass(hInstance);

	RECT mainRect = {};
	if (hMainWnd) ::GetWindowRect(hMainWnd, &mainRect);
	int x = hMainWnd ? mainRect.right + 8 : CW_USEDEFAULT;
	int y = hMainWnd ? mainRect.top       : CW_USEDEFAULT;

	RECT wr = { 0, 0, 360, 860 };
	::AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	_hWnd = ::CreateWindowW(CLASS_NAME, L"Jellyto Studio - 디테일 패널",
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
	RefreshEntityList();
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

// ── UI 빌드 ──────────────────────────────────────────────────────────────

void DetailWindow::BuildUI()
{
	const int W  = 330;  // 전체 너비
	const int LX = 10;
	const int VX = 155;
	const int LW = 140;
	const int VW = 175;
	const int RH = 18;
	const int RS = 22;

	int y = 8;

	auto MkStatic = [&](const wchar_t* txt, int x, int yy, int w, int h, DWORD extra = 0) -> HWND {
		return ::CreateWindowW(L"STATIC", txt, WS_CHILD | WS_VISIBLE | SS_LEFT | extra,
			x, yy, w, h, _hWnd, nullptr, _hInstance, nullptr);
	};
	auto MkVal = [&](HWND& out, const wchar_t* def, int yy) {
		out = ::CreateWindowW(L"STATIC", def, WS_CHILD | WS_VISIBLE | SS_LEFT | SS_SUNKEN,
			VX, yy, VW, RH, _hWnd, nullptr, _hInstance, nullptr);
	};
	auto MkSep = [&](const wchar_t* title, int yy) {
		::CreateWindowW(L"STATIC", title, WS_CHILD | WS_VISIBLE | SS_LEFT,
			LX, yy, W, RH, _hWnd, nullptr, _hInstance, nullptr);
		::CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
			LX, yy + RH + 1, W, 2, _hWnd, nullptr, _hInstance, nullptr);
	};

	// ══════════════════════════════════════════════════
	// [1] 씬 정보
	// ══════════════════════════════════════════════════
	MkSep(L"▶  씬 (Scene)", y);  y += RH + 6;
	MkStatic(L"씬 이름", LX, y, LW, RH);
	MkVal(_hSceneName, L"(씬 없음)", y);
	y += RS + 8;

	// ══════════════════════════════════════════════════
	// [2] 씬 오브젝트 목록
	// ══════════════════════════════════════════════════
	MkSep(L"▶  씬 오브젝트 목록", y);  y += RH + 6;

	_hEntityCount = MkStatic(L"오브젝트 [0개]", LX, y, W, RH);
	y += RS;

	_hEntityList = ::CreateWindowW(L"LISTBOX", nullptr,
		WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
		LX, y, W, 160, _hWnd, (HMENU)(INT_PTR)ID_LIST_ENTITY, _hInstance, nullptr);
	y += 168;

	// ══════════════════════════════════════════════════
	// [3] 피킹된 오브젝트 정보
	// ══════════════════════════════════════════════════
	MkSep(L"▶  선택된 오브젝트", y);  y += RH + 6;

	_hPickedLabel = MkStatic(L"(선택 없음 — 마우스 좌클릭으로 선택)", LX, y, W, RH);
	y += RS + 4;

	// Model
	MkStatic(L"[ Model ]", LX, y, W, RH, SS_LEFT);  y += RS;
	MkStatic(L"이름",          LX, y, LW, RH); MkVal(_hModelName,  L"—", y); y += RS;
	MkStatic(L"본(Bone) 수",   LX, y, LW, RH); MkVal(_hBoneCount,  L"—", y); y += RS;
	MkStatic(L"메시(Mesh) 수", LX, y, LW, RH); MkVal(_hMeshCount,  L"—", y); y += RS + 4;

	// Animation
	MkStatic(L"[ Animation ]", LX, y, W, RH, SS_LEFT);  y += RS;
	MkStatic(L"클립 이름",    LX, y, LW, RH); MkVal(_hAnimName,   L"—",     y); y += RS;
	MkStatic(L"프레임 수",    LX, y, LW, RH); MkVal(_hFrameCount, L"—",     y); y += RS;
	MkStatic(L"Frame Rate",   LX, y, LW, RH); MkVal(_hFrameRate,  L"—",     y); y += RS;
	MkStatic(L"Duration (s)", LX, y, LW, RH); MkVal(_hDuration,   L"—",     y); y += RS + 4;

	// Transform
	MkStatic(L"[ Transform ]", LX, y, W, RH, SS_LEFT);  y += RS;
	MkStatic(L"Position  X",  LX, y, LW, RH); MkVal(_hPosX, L"0.000", y); y += RS;
	MkStatic(L"Position  Y",  LX, y, LW, RH); MkVal(_hPosY, L"0.000", y); y += RS;
	MkStatic(L"Position  Z",  LX, y, LW, RH); MkVal(_hPosZ, L"0.000", y); y += RS;
	MkStatic(L"Rotation  X°", LX, y, LW, RH); MkVal(_hRotX, L"0.000", y); y += RS;
	MkStatic(L"Rotation  Y°", LX, y, LW, RH); MkVal(_hRotY, L"0.000", y); y += RS;
	MkStatic(L"Rotation  Z°", LX, y, LW, RH); MkVal(_hRotZ, L"0.000", y); y += RS;
	MkStatic(L"Scale     X",  LX, y, LW, RH); MkVal(_hSclX, L"1.000", y); y += RS;
	MkStatic(L"Scale     Y",  LX, y, LW, RH); MkVal(_hSclY, L"1.000", y); y += RS;
	MkStatic(L"Scale     Z",  LX, y, LW, RH); MkVal(_hSclZ, L"1.000", y);
}

// ── 씬 주입 / 목록 갱신 ──────────────────────────────────────────────────

void DetailWindow::SetScene(std::shared_ptr<Scene> scene)
{
	_scene = scene;
	if (!_hWnd) return;

	// [1] 씬 이름 갱신
	if (_hSceneName)
	{
		const wchar_t* name = scene ? scene->GetName().c_str() : L"(씬 없음)";
		::SetWindowTextW(_hSceneName, name);
	}

	RefreshEntityList();
}

void DetailWindow::RefreshEntityList()
{
	if (!_hEntityList) return;

	::SendMessage(_hEntityList, LB_RESETCONTENT, 0, 0);

	auto scene = _scene.lock();
	if (!scene)
	{
		::SetWindowTextW(_hEntityCount, L"오브젝트 [씬 없음]");
		if (_hSceneName) ::SetWindowTextW(_hSceneName, L"(씬 없음)");
		return;
	}

	// [1] 씬 이름 최신화
	if (_hSceneName) ::SetWindowTextW(_hSceneName, scene->GetName().c_str());

	auto& entities = scene->GetEntities();
	int idx = 0;

	for (auto& entity : entities)
	{
		// 오브젝트 유형 판별해서 표시 이름 결정
		std::wstring label;
		bool hasCollider = (entity->GetComponent<BaseCollider>() != nullptr);

		auto animator = entity->GetComponent<ModelAnimator>();
		if (animator && animator->GetModel())
		{
			auto model = animator->GetModel();
			label = L"[Model] ";
			label += (model->GetMeshCount() > 0) ? model->GetMeshByIndex(0)->name : L"Unknown";
		}
		else if (entity->GetComponent<MeshRenderer>() != nullptr)
		{
			label = L"[Mesh] Entity_";
			label += std::to_wstring(idx);
		}
		else if (entity->GetComponent<Camera>() != nullptr)
		{
			label = L"[Camera]";
		}
		else
		{
			label = L"[Entity_" + std::to_wstring(idx) + L"]";
		}

		// Collider 유무 표시 (피킹 가능 여부)
		label += hasCollider ? L"  ✔" : L"  (콜라이더 없음)";

		::SendMessage(_hEntityList, LB_ADDSTRING, 0, (LPARAM)label.c_str());
		idx++;
	}

	wchar_t buf[64];
	swprintf_s(buf, L"오브젝트 [%d개]", (int)entities.size());
	::SetWindowTextW(_hEntityCount, buf);
}

// ── 피킹 결과 갱신 ───────────────────────────────────────────────────────

void DetailWindow::UpdateDetail(const DetailInfo& info)
{
	if (!_hWnd) return;

	// 선택 레이블
	std::wstring pickedLbl = L"선택됨: " + info.entityLabel;
	if (_hPickedLabel) ::SetWindowTextW(_hPickedLabel, pickedLbl.c_str());

	auto Set = [](HWND h, const wchar_t* txt) { if (h) ::SetWindowTextW(h, txt); };
	auto SetF = [](HWND h, float v) {
		if (!h) return;
		wchar_t b[32]; swprintf_s(b, L"%.3f", v); ::SetWindowTextW(h, b);
	};
	auto SetI = [](HWND h, int v) {
		if (!h) return;
		wchar_t b[16]; swprintf_s(b, L"%d", v); ::SetWindowTextW(h, b);
	};

	Set (_hModelName,  info.modelName.empty() ? L"—" : info.modelName.c_str());
	SetI(_hBoneCount,  info.boneCount);
	SetI(_hMeshCount,  info.meshCount);

	Set (_hAnimName,   info.animName.empty() ? L"—" : info.animName.c_str());
	SetI(_hFrameCount, info.frameCount);
	SetF(_hFrameRate,  info.frameRate);
	SetF(_hDuration,   info.duration);

	SetF(_hPosX, info.tx); SetF(_hPosY, info.ty); SetF(_hPosZ, info.tz);
	SetF(_hRotX, info.rx); SetF(_hRotY, info.ry); SetF(_hRotZ, info.rz);
	SetF(_hSclX, info.sx); SetF(_hSclY, info.sy); SetF(_hSclZ, info.sz);
}

void DetailWindow::ClearDetail()
{
	if (!_hWnd) return;

	if (_hPickedLabel)
		::SetWindowTextW(_hPickedLabel, L"(선택 없음 — 마우스 좌클릭으로 선택)");

	auto Set = [](HWND h, const wchar_t* t) { if (h) ::SetWindowTextW(h, t); };
	Set(_hModelName, L"—"); Set(_hBoneCount, L"—"); Set(_hMeshCount, L"—");
	Set(_hAnimName,  L"—"); Set(_hFrameCount, L"—");
	Set(_hFrameRate, L"—"); Set(_hDuration,   L"—");
	Set(_hPosX, L"0.000"); Set(_hPosY, L"0.000"); Set(_hPosZ, L"0.000");
	Set(_hRotX, L"0.000"); Set(_hRotY, L"0.000"); Set(_hRotZ, L"0.000");
	Set(_hSclX, L"1.000"); Set(_hSclY, L"1.000"); Set(_hSclZ, L"1.000");
}

// ── WndProc ──────────────────────────────────────────────────────────────

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