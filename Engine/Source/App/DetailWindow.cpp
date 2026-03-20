#include "Framework.h"
#include "DetailWindow.h"

#include "Scene/Scene.h"
#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Camera.h"
#include "Entity/Components/Collider/BaseCollider.h"
#include "Entity/Components/MeshRenderer.h"
#include "Graphics/Model/ModelAnimator.h"
#include "Graphics/Model/Model.h"
#include "Graphics/Model/ModelAnimation.h"

DetailWindow::DetailWindow()	{ }
DetailWindow::~DetailWindow()	{ }

// ── 생성 ────────────────────────────────────────────────────────────────

bool DetailWindow::Create(HINSTANCE hInstance, HWND hMainWnd)
{
	if (_created) return true;
	_hInstance = hInstance;

	RegisterWindowClass(hInstance);

	RECT mainRect = {};
	if (hMainWnd) ::GetWindowRect(hMainWnd, &mainRect);
	int x = hMainWnd ? mainRect.right + 8 : CW_USEDEFAULT;
	int y = hMainWnd ? mainRect.top : CW_USEDEFAULT;

	RECT wr = { 0, 0, 360, 900 };
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
	const int W = 330;
	const int LX = 10;
	const int VX = 160;
	const int LW = 145;
	const int VW = 170;
	const int RH = 18;
	const int RS = 22;

	int y = 8;

	auto MkS = [&](const wchar_t* txt, int x, int yy, int w, int h, DWORD ex = 0) -> HWND {
		return ::CreateWindowW(L"STATIC", txt, WS_CHILD | WS_VISIBLE | SS_LEFT | ex,
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

	// ══ [1] 씬 ══════════════════════════════════════════════════
	MkSep(L"▶  씬 (Scene)", y);  y += RH + 6;
	MkS(L"씬 이름", LX, y, LW, RH);
	MkVal(_hSceneName, L"(씬 없음)", y);
	y += RS + 10;

	// ══ [2] 씬 오브젝트 목록 ════════════════════════════════════
	MkSep(L"▶  씬 오브젝트 목록  (클릭하여 선택)", y);  y += RH + 6;
	_hEntityCount = MkS(L"오브젝트 [0개]", LX, y, W, RH);
	y += RS;
	_hEntityList = ::CreateWindowW(L"LISTBOX", nullptr,
		WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
		LX, y, W, 160, _hWnd, (HMENU)(INT_PTR)ID_LIST_ENTITY, _hInstance, nullptr);
	y += 168;

	// ══ [3] 선택된 오브젝트 정보 ════════════════════════════════
	MkSep(L"▶  선택된 오브젝트", y);  y += RH + 6;
	_hPickedLabel = MkS(L"(없음 — 목록 클릭 또는 마우스 좌클릭)", LX, y, W, RH);
	y += RS + 6;

	MkS(L"[ Model ]", LX, y, W, RH); y += RS;
	MkS(L"이름", LX, y, LW, RH); MkVal(_hModelName, L"—", y); y += RS;
	MkS(L"본(Bone) 수", LX, y, LW, RH); MkVal(_hBoneCount, L"—", y); y += RS;
	MkS(L"메시(Mesh) 수", LX, y, LW, RH); MkVal(_hMeshCount, L"—", y); y += RS + 4;

	MkS(L"[ Animation ]", LX, y, W, RH); y += RS;
	MkS(L"클립 이름", LX, y, LW, RH); MkVal(_hAnimName, L"—", y); y += RS;
	MkS(L"프레임 수", LX, y, LW, RH); MkVal(_hFrameCount, L"—", y); y += RS;
	MkS(L"Frame Rate", LX, y, LW, RH); MkVal(_hFrameRate, L"—", y); y += RS;
	MkS(L"Duration (s)", LX, y, LW, RH); MkVal(_hDuration, L"—", y); y += RS + 4;

	MkS(L"[ Transform ]", LX, y, W, RH); y += RS;
	MkS(L"Position  X", LX, y, LW, RH); MkVal(_hPosX, L"0.000", y); y += RS;
	MkS(L"Position  Y", LX, y, LW, RH); MkVal(_hPosY, L"0.000", y); y += RS;
	MkS(L"Position  Z", LX, y, LW, RH); MkVal(_hPosZ, L"0.000", y); y += RS;
	MkS(L"Rotation  X°", LX, y, LW, RH); MkVal(_hRotX, L"0.000", y); y += RS;
	MkS(L"Rotation  Y°", LX, y, LW, RH); MkVal(_hRotY, L"0.000", y); y += RS;
	MkS(L"Rotation  Z°", LX, y, LW, RH); MkVal(_hRotZ, L"0.000", y); y += RS;
	MkS(L"Scale     X", LX, y, LW, RH); MkVal(_hSclX, L"1.000", y); y += RS;
	MkS(L"Scale     Y", LX, y, LW, RH); MkVal(_hSclY, L"1.000", y); y += RS;
	MkS(L"Scale     Z", LX, y, LW, RH); MkVal(_hSclZ, L"1.000", y);
}

// ── 씬 주입 / 목록 갱신 ──────────────────────────────────────────────────

void DetailWindow::SetScene(std::shared_ptr<Scene> scene)
{
	_scene = scene;
	if (_hSceneName)
		::SetWindowTextW(_hSceneName, scene ? scene->GetName().c_str() : L"(씬 없음)");
	RefreshEntityList();
}

void DetailWindow::RefreshEntityList()
{
	if (!_hEntityList) return;

	// 현재 선택 인덱스 기억 (갱신 후 복원)
	int prevSel = (int)::SendMessage(_hEntityList, LB_GETCURSEL, 0, 0);

	::SendMessage(_hEntityList, LB_RESETCONTENT, 0, 0);
	_entitySnapshot.clear();

	auto scene = _scene.lock();
	if (!scene)
	{
		::SetWindowTextW(_hEntityCount, L"오브젝트 [씬 없음]");
		if (_hSceneName) ::SetWindowTextW(_hSceneName, L"(씬 없음)");
		return;
	}

	if (_hSceneName) ::SetWindowTextW(_hSceneName, scene->GetName().c_str());

	int idx = 0;
	for (auto& entity : scene->GetEntities())
	{
		std::wstring label;
		bool hasCollider = (entity->GetComponent<BaseCollider>() != nullptr);

		auto animator = entity->GetComponent<ModelAnimator>();
		if (animator && animator->GetModel())
		{
			auto mdl = animator->GetModel();
			label = entity->GetEntityName();
			label += (mdl->GetMeshCount() > 0) ? mdl->GetMeshByIndex(0)->name : L"Unknown";
		}
		else if (entity->GetComponent<Camera>() != nullptr)
		{
			label = entity->GetEntityName();
		}
		else if (entity->GetComponent<MeshRenderer>() != nullptr)
		{
			label = entity->GetEntityName();
		}
		else
		{
			label = L"[Entity_" + std::to_wstring(idx) + L"]";
		}

		// 피킹 가능 여부 표시 (콜라이더 있으면 ✔)
		label += hasCollider ? L"  ✔" : L"  ✕";

		::SendMessage(_hEntityList, LB_ADDSTRING, 0, (LPARAM)label.c_str());
		_entitySnapshot.push_back(entity);
		idx++;
	}

	wchar_t buf[64];
	swprintf_s(buf, L"오브젝트 [%d개]  (✔=피킹가능)", (int)_entitySnapshot.size());
	::SetWindowTextW(_hEntityCount, buf);

	// 이전 선택 복원 (범위 내일 때만)
	if (prevSel >= 0 && prevSel < (int)_entitySnapshot.size())
		::SendMessage(_hEntityList, LB_SETCURSEL, prevSel, 0);
}

// ── 목록 클릭 → Entity 정보 표시 ─────────────────────────────────────────

void DetailWindow::OnEntityListClicked()
{
	int sel = (int)::SendMessage(_hEntityList, LB_GETCURSEL, 0, 0);
	if (sel == LB_ERR || sel >= (int)_entitySnapshot.size())
	{
		_selectedEntity = nullptr;
		ClearDetail();
		return;
	}

	_selectedEntity = _entitySnapshot[sel];
	FillDetailFromEntity(_selectedEntity);
}

void DetailWindow::FillDetailFromEntity(std::shared_ptr<Entity> entity)
{
	if (!entity) { ClearDetail(); return; }

	DetailInfo info;

	// 표시 이름
	auto animator = entity->GetComponent<ModelAnimator>();
	if (animator && animator->GetModel())
	{
		auto mdl = animator->GetModel();
		info.entityLabel = (mdl->GetMeshCount() > 0) ? mdl->GetMeshByIndex(0)->name : L"Unknown";
		info.modelName = info.entityLabel;
		info.boneCount = (int)mdl->GetBoneCount();
		info.meshCount = (int)mdl->GetMeshCount();

		if (mdl->GetAnimationCount() > 0)
		{
			auto anim = mdl->GetAnimationByIndex(0);
			info.animName = anim->name;
			info.frameCount = (int)anim->frameCount;
			info.frameRate = anim->frameRate;
			info.duration = anim->duration;
		}
	}
	else
	{
		info.entityLabel = L"Entity";
	}

	// Transform
	auto tf = entity->GetTransform();
	if (tf)
	{
		Vec3 pos = tf->GetPosition();
		Vec3 rot = tf->GetRotation();
		Vec3 scl = tf->GetScale();
		info.tx = pos.x; info.ty = pos.y; info.tz = pos.z;
		info.rx = XMConvertToDegrees(rot.x);
		info.ry = XMConvertToDegrees(rot.y);
		info.rz = XMConvertToDegrees(rot.z);
		info.sx = scl.x; info.sy = scl.y; info.sz = scl.z;
	}

	UpdateDetail(info);
}

// ── 피킹 결과 갱신 (외부 MainApp에서 호출) ───────────────────────────────

void DetailWindow::UpdateDetail(const DetailInfo& info)
{
	if (!_hWnd) return;

	if (_hPickedLabel)
		::SetWindowTextW(_hPickedLabel, (L"선택됨: " + info.entityLabel).c_str());

	auto Set = [](HWND h, const wchar_t* t) { if (h) ::SetWindowTextW(h, t); };
	auto SetF = [](HWND h, float v) {
		if (!h) return; wchar_t b[32]; swprintf_s(b, L"%.3f", v); ::SetWindowTextW(h, b);
		};
	auto SetI = [](HWND h, int v) {
		if (!h) return; wchar_t b[16]; swprintf_s(b, L"%d", v); ::SetWindowTextW(h, b);
		};

	Set(_hModelName, info.modelName.empty() ? L"—" : info.modelName.c_str());
	SetI(_hBoneCount, info.boneCount);
	SetI(_hMeshCount, info.meshCount);
	Set(_hAnimName, info.animName.empty() ? L"—" : info.animName.c_str());
	SetI(_hFrameCount, info.frameCount);
	SetF(_hFrameRate, info.frameRate);
	SetF(_hDuration, info.duration);
	SetF(_hPosX, info.tx); SetF(_hPosY, info.ty); SetF(_hPosZ, info.tz);
	SetF(_hRotX, info.rx); SetF(_hRotY, info.ry); SetF(_hRotZ, info.rz);
	SetF(_hSclX, info.sx); SetF(_hSclY, info.sy); SetF(_hSclZ, info.sz);
}

void DetailWindow::ClearDetail()
{
	if (!_hWnd) return;
	if (_hPickedLabel)
		::SetWindowTextW(_hPickedLabel, L"(없음 — 목록 클릭 또는 마우스 좌클릭)");

	auto Set = [](HWND h, const wchar_t* t) { if (h) ::SetWindowTextW(h, t); };
	Set(_hModelName, L"—"); Set(_hBoneCount, L"—"); Set(_hMeshCount, L"—");
	Set(_hAnimName, L"—"); Set(_hFrameCount, L"—");
	Set(_hFrameRate, L"—"); Set(_hDuration, L"—");
	Set(_hPosX, L"0.000"); Set(_hPosY, L"0.000"); Set(_hPosZ, L"0.000");
	Set(_hRotX, L"0.000"); Set(_hRotY, L"0.000"); Set(_hRotZ, L"0.000");
	Set(_hSclX, L"1.000"); Set(_hSclY, L"1.000"); Set(_hSclZ, L"1.000");
}

// ── Ray 피킹 결과로 목록 강조 ───────────────────────────────────────────

void DetailWindow::SelectEntity(std::shared_ptr<Entity> entity)
{
	_selectedEntity = entity;
	if (!_hEntityList || !entity) return;

	for (int i = 0; i < (int)_entitySnapshot.size(); i++)
	{
		if (_entitySnapshot[i] == entity)
		{
			::SendMessage(_hEntityList, LB_SETCURSEL, i, 0);
			FillDetailFromEntity(entity);
			return;
		}
	}
}

// ── WndProc ──────────────────────────────────────────────────────────────

void DetailWindow::RegisterWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = DetailWindow::WndProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
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

		case WM_COMMAND:
			// ListBox 클릭 (LBN_SELCHANGE: 선택 변경)
			if (LOWORD(wParam) == ID_LIST_ENTITY &&
				HIWORD(wParam) == LBN_SELCHANGE)
			{
				self->OnEntityListClicked();
				return 0;
			}
			break;
		}
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}