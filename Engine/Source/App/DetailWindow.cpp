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

// ── 생성 ─────────────────────────────────────────────────────────────────

bool DetailWindow::Create(HINSTANCE hInstance, HWND hMainWnd)
{
	if (_created) return true;
	_hInstance = hInstance;

	RegisterWindowClass(hInstance);

	RECT mainRect = {};
	if (hMainWnd) ::GetWindowRect(hMainWnd, &mainRect);
	int x = hMainWnd ? mainRect.right + 8 : CW_USEDEFAULT;
	int y = hMainWnd ? mainRect.top : CW_USEDEFAULT;

	RECT wr = { 0, 0, 360, 960 };
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

// ── UI 빌드 ───────────────────────────────────────────────────────────────

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

	auto MkS = [&](const wchar_t* txt, int x, int yy, int w, int h) -> HWND
		{
			return ::CreateWindowW(L"STATIC", txt, WS_CHILD | WS_VISIBLE | SS_LEFT,
				x, yy, w, h, _hWnd, nullptr, _hInstance, nullptr);
		};
	auto MkVal = [&](HWND& out, const wchar_t* def, int yy)
		{
			out = ::CreateWindowW(L"STATIC", def, WS_CHILD | WS_VISIBLE | SS_LEFT | SS_SUNKEN,
				VX, yy, VW, RH, _hWnd, nullptr, _hInstance, nullptr);
		};
	auto MkEdit = [&](HWND& out, const wchar_t* def, int yy)
		{
			out = ::CreateWindowW(L"EDIT", def,
				WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
				VX, yy, VW, RH, _hWnd, nullptr, _hInstance, nullptr);
		};
	auto MkSep = [&](const wchar_t* title, int yy)
		{
			::CreateWindowW(L"STATIC", title, WS_CHILD | WS_VISIBLE | SS_LEFT,
				LX, yy, W, RH, _hWnd, nullptr, _hInstance, nullptr);
			::CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
				LX, yy + RH + 1, W, 2, _hWnd, nullptr, _hInstance, nullptr);
		};

	// [1] 씬
	MkSep(L"▶  씬 (Scene)", y); y += RH + 6;
	MkS(L"씬 이름", LX, y, LW, RH);
	MkVal(_hSceneName, L"(씬 없음)", y);
	y += RS + 10;

	// [2] 오브젝트 목록
	MkSep(L"▶  씬 오브젝트 목록  (클릭하여 선택)", y); y += RH + 6;
	_hEntityCount = MkS(L"오브젝트 [0개]", LX, y, W, RH);
	y += RS;
	_hEntityList = ::CreateWindowW(L"LISTBOX", nullptr,
		WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
		LX, y, W, 150, _hWnd, (HMENU)(INT_PTR)ID_LIST_ENTITY, _hInstance, nullptr);
	y += 158;

	// [3] 오브젝트 정보 (읽기전용)
	MkSep(L"▶  선택된 오브젝트", y); y += RH + 6;
	_hPickedLabel = MkS(L"(없음 — 목록 클릭 또는 마우스 좌클릭)", LX, y, W, RH);
	y += RS + 6;

	MkS(L"[ Model ]", LX, y, W, RH); y += RS;
	MkS(L"이름", LX, y, LW, RH); MkVal(_hModelName, L"—", y); y += RS;
	MkS(L"Bone 수", LX, y, LW, RH); MkVal(_hBoneCount, L"—", y); y += RS;
	MkS(L"Mesh 수", LX, y, LW, RH); MkVal(_hMeshCount, L"—", y); y += RS + 4;

	MkS(L"[ Animation ]", LX, y, W, RH); y += RS;
	MkS(L"클립 이름", LX, y, LW, RH); MkVal(_hAnimName, L"—", y); y += RS;
	MkS(L"프레임 수", LX, y, LW, RH); MkVal(_hFrameCount, L"—", y); y += RS;
	MkS(L"Frame Rate", LX, y, LW, RH); MkVal(_hFrameRate, L"—", y); y += RS;
	MkS(L"Duration (s)", LX, y, LW, RH); MkVal(_hDuration, L"—", y); y += RS + 6;

	// [4] Transform 편집
	MkSep(L"▶  Transform  [편집 후 Apply 또는 Enter]", y); y += RH + 6;

	MkS(L"Position  X", LX, y, LW, RH); MkEdit(_hPosX, L"0.000", y); y += RS;
	MkS(L"Position  Y", LX, y, LW, RH); MkEdit(_hPosY, L"0.000", y); y += RS;
	MkS(L"Position  Z", LX, y, LW, RH); MkEdit(_hPosZ, L"0.000", y); y += RS + 2;
	MkS(L"Rotation  X°", LX, y, LW, RH); MkEdit(_hRotX, L"0.000", y); y += RS;
	MkS(L"Rotation  Y°", LX, y, LW, RH); MkEdit(_hRotY, L"0.000", y); y += RS;
	MkS(L"Rotation  Z°", LX, y, LW, RH); MkEdit(_hRotZ, L"0.000", y); y += RS + 2;
	MkS(L"Scale     X", LX, y, LW, RH); MkEdit(_hSclX, L"1.000", y); y += RS;
	MkS(L"Scale     Y", LX, y, LW, RH); MkEdit(_hSclY, L"1.000", y); y += RS;
	MkS(L"Scale     Z", LX, y, LW, RH); MkEdit(_hSclZ, L"1.000", y); y += RS + 8;

	_hApplyBtn = ::CreateWindowW(L"BUTTON", L"Apply Transform",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		LX, y, W, 28, _hWnd, (HMENU)(INT_PTR)ID_BTN_APPLY, _hInstance, nullptr);
}

// ── 씬 / Dirty 관리 ───────────────────────────────────────────────────────

void DetailWindow::SetScene(std::shared_ptr<Scene> scene)
{
	_scene = scene;
	if (_hSceneName)
		::SetWindowTextW(_hSceneName, scene ? scene->GetName().c_str() : L"(씬 없음)");
	_listDirty = true;
	RefreshEntityList();
}

void DetailWindow::MarkDirty()
{
	_listDirty = true;
}

// ── RefreshEntityList ─────────────────────────────────────────────────────
// Dirty일 때만 목록 재구성 + 스크롤/선택 위치 보존
void DetailWindow::RefreshEntityList()
{
	if (!_hEntityList) return;
	// Dirty가 아니면 재구성하지 않음 (스크롤 유지, 편집 중 덮어쓰기 방지)
	if (!_listDirty) return;
	_listDirty = false;

	// 스크롤 top 위치 + 선택 index 기억
	int prevTop = (int)::SendMessage(_hEntityList, LB_GETTOPINDEX, 0, 0);
	int prevSel = (int)::SendMessage(_hEntityList, LB_GETCURSEL, 0, 0);
	std::shared_ptr<Entity> prevSelected = _selectedEntity;

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

	int newSelIdx = -1;
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
		else if (entity->GetComponent<Camera>())
			label = entity->GetEntityName();
		else if (entity->GetComponent<MeshRenderer>())
			label = entity->GetEntityName();
		else
			label = L"[Entity_" + std::to_wstring(idx) + L"]";

		label += hasCollider ? L"  ✔" : L"  ✕";
		::SendMessage(_hEntityList, LB_ADDSTRING, 0, (LPARAM)label.c_str());

		// 이전 선택 Entity를 새 snapshot에서 찾아둠
		if (entity == prevSelected)
			newSelIdx = idx;

		_entitySnapshot.push_back(entity);
		idx++;
	}

	wchar_t buf[64];
	swprintf_s(buf, L"오브젝트 [%d개]  (✔=피킹가능)", (int)_entitySnapshot.size());
	::SetWindowTextW(_hEntityCount, buf);

	// 스크롤 위치 복원
	if (prevTop >= 0)
		::SendMessage(_hEntityList, LB_SETTOPINDEX, prevTop, 0);

	// 선택 복원 (Entity 포인터 기준으로 새 인덱스 찾아서 복원)
	if (newSelIdx >= 0)
		::SendMessage(_hEntityList, LB_SETCURSEL, newSelIdx, 0);
	else if (prevSel >= 0 && prevSel < (int)_entitySnapshot.size())
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
		info.entityLabel = entity->GetEntityName();
	}

	if (auto tf = entity->GetTransform())
	{
		Vec3 pos = tf->GetLocalPosition();
		Vec3 rot = tf->GetLocalRotation();
		Vec3 scl = tf->GetLocalScale();
		info.tx = pos.x; info.ty = pos.y; info.tz = pos.z;
		info.rx = XMConvertToDegrees(rot.x);
		info.ry = XMConvertToDegrees(rot.y);
		info.rz = XMConvertToDegrees(rot.z);
		info.sx = scl.x; info.sy = scl.y; info.sz = scl.z;
	}

	UpdateDetail(info);
}

// ── UpdateDetail / ClearDetail ────────────────────────────────────────────

void DetailWindow::UpdateDetail(const DetailInfo& info)
{
	if (!_hWnd) return;

	if (_hPickedLabel)
		::SetWindowTextW(_hPickedLabel, (L"선택됨: " + info.entityLabel).c_str());

	auto Set = [](HWND h, const wchar_t* t) { if (h) ::SetWindowTextW(h, t); };
	auto SetF = [](HWND h, float v)
		{
			if (!h) return;
			wchar_t b[32]; swprintf_s(b, L"%.3f", v);
			::SetWindowTextW(h, b);
		};
	auto SetI = [](HWND h, int v)
		{
			if (!h) return;
			wchar_t b[16]; swprintf_s(b, L"%d", v);
			::SetWindowTextW(h, b);
		};

	Set(_hModelName, info.modelName.empty() ? L"—" : info.modelName.c_str());
	SetI(_hBoneCount, info.boneCount);
	SetI(_hMeshCount, info.meshCount);
	Set(_hAnimName, info.animName.empty() ? L"—" : info.animName.c_str());
	SetI(_hFrameCount, info.frameCount);
	SetF(_hFrameRate, info.frameRate);
	SetF(_hDuration, info.duration);

	// Transform Edit: 편집 중(포커스 있음)이면 덮어쓰지 않음
	if (!IsTransformEditFocused())
	{
		SetF(_hPosX, info.tx); SetF(_hPosY, info.ty); SetF(_hPosZ, info.tz);
		SetF(_hRotX, info.rx); SetF(_hRotY, info.ry); SetF(_hRotZ, info.rz);
		SetF(_hSclX, info.sx); SetF(_hSclY, info.sy); SetF(_hSclZ, info.sz);
	}
}

void DetailWindow::ClearDetail()
{
	if (!_hWnd) return;
	if (_hPickedLabel)
		::SetWindowTextW(_hPickedLabel, L"(없음 — 목록 클릭 또는 마우스 좌클릭)");

	auto Set = [](HWND h, const wchar_t* t) { if (h) ::SetWindowTextW(h, t); };
	Set(_hModelName, L"—"); Set(_hBoneCount, L"—"); Set(_hMeshCount, L"—");
	Set(_hAnimName, L"—");  Set(_hFrameCount, L"—");
	Set(_hFrameRate, L"—"); Set(_hDuration, L"—");
	Set(_hPosX, L"0.000"); Set(_hPosY, L"0.000"); Set(_hPosZ, L"0.000");
	Set(_hRotX, L"0.000"); Set(_hRotY, L"0.000"); Set(_hRotZ, L"0.000");
	Set(_hSclX, L"1.000"); Set(_hSclY, L"1.000"); Set(_hSclZ, L"1.000");
}

// ── Transform 편집 ────────────────────────────────────────────────────────

bool DetailWindow::IsTransformEditFocused() const
{
	HWND focused = ::GetFocus();
	return focused == _hPosX || focused == _hPosY || focused == _hPosZ
		|| focused == _hRotX || focused == _hRotY || focused == _hRotZ
		|| focused == _hSclX || focused == _hSclY || focused == _hSclZ;
}

float DetailWindow::GetEditFloat(HWND hEdit, float fallback)
{
	if (!hEdit) return fallback;
	wchar_t buf[64] = {};
	::GetWindowTextW(hEdit, buf, 64);
	try { return std::stof(buf); }
	catch (...) { return fallback; }
}

void DetailWindow::ApplyTransform()
{
	if (!_selectedEntity) return;
	auto tf = _selectedEntity->GetTransform();
	if (!tf) return;

	Vec3 pos(GetEditFloat(_hPosX), GetEditFloat(_hPosY), GetEditFloat(_hPosZ));
	Vec3 rot(
		XMConvertToRadians(GetEditFloat(_hRotX)),
		XMConvertToRadians(GetEditFloat(_hRotY)),
		XMConvertToRadians(GetEditFloat(_hRotZ)));
	Vec3 scl(
		GetEditFloat(_hSclX, 1.f),
		GetEditFloat(_hSclY, 1.f),
		GetEditFloat(_hSclZ, 1.f));

	tf->SetLocalPosition(pos);
	tf->SetLocalRotation(rot);
	tf->SetLocalScale(scl);
}

// ── SelectEntity ──────────────────────────────────────────────────────────

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

// ── WndProc ───────────────────────────────────────────────────────────────

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
			// Apply 버튼
			if (LOWORD(wParam) == ID_BTN_APPLY && HIWORD(wParam) == BN_CLICKED)
			{
				self->ApplyTransform();
				return 0;
			}
			// Entity 목록 선택
			if (LOWORD(wParam) == ID_LIST_ENTITY && HIWORD(wParam) == LBN_SELCHANGE)
			{
				self->OnEntityListClicked();
				return 0;
			}
			break;

			// Edit 컨트롤에서 Enter → Apply
		case WM_KEYDOWN:
			if (wParam == VK_RETURN && self->IsTransformEditFocused())
			{
				self->ApplyTransform();
				return 0;
			}
			break;
		}
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}