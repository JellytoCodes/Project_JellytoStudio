#include "Framework.h"
#include "ItemWindow.h"

#include "Scene/Scene.h"
#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Graphics/Model/ModelAnimator.h"
#include "Graphics/Model/Model.h"
#include "Resource/Managers/ResourceManager.h"
#include "Pipeline/Shader.h"

#include <filesystem>

ItemWindow::ItemWindow() {}
ItemWindow::~ItemWindow() {}

// ── 생성 ────────────────────────────────────────────────────────────────

bool ItemWindow::Create(HINSTANCE hInstance, HWND hMainWnd)
{
	if (_created) return true;
	_hInstance = hInstance;

	RegisterWindowClass(hInstance);

	RECT mainRect = {};
	if (hMainWnd) ::GetWindowRect(hMainWnd, &mainRect);
	int x = hMainWnd ? mainRect.right + 8 : CW_USEDEFAULT;
	int y = hMainWnd ? mainRect.top       : CW_USEDEFAULT;

	RECT wr = { 0, 0, 580, 620 };
	::AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	_hWnd = ::CreateWindowW(CLASS_NAME, L"Jellyto Studio - 아이템 배치",
		WS_OVERLAPPEDWINDOW,
		x, y, wr.right - wr.left, wr.bottom - wr.top,
		hMainWnd, nullptr, hInstance, this);

	if (!_hWnd) return false;

	BuildUI();
	ScanMeshFiles();
	_created = true;
	return true;
}

void ItemWindow::Show()
{
	if (!_hWnd) return;
	ScanMeshFiles(); // 열릴 때마다 최신 목록 갱신
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

// ── UI 빌드 ──────────────────────────────────────────────────────────────

void ItemWindow::BuildUI()
{
	auto C = [&](const wchar_t* cls, const wchar_t* txt, DWORD s,
		int x, int y, int w, int h, int id) -> HWND
	{
		return ::CreateWindowW(cls, txt, WS_CHILD | WS_VISIBLE | s,
			x, y, w, h, _hWnd, (HMENU)(INT_PTR)id, _hInstance, nullptr);
	};

	// ── 좌측: .mesh 파일 목록 ────────────────────────────────────
	_hLblMesh = C(L"STATIC", L"모델 목록 [0개]", SS_LEFT, 10, 10, 240, 18, 0);
	_hMeshList = C(L"LISTBOX", L"",
		WS_BORDER | WS_VSCROLL | LBS_NOTIFY, 10, 32, 250, 530, ID_LIST_MESH);

	// ── 중앙: 버튼 ───────────────────────────────────────────────
	C(L"BUTTON", L"▶ 배치",    BS_PUSHBUTTON, 272, 180, 90, 28, ID_BTN_PLACE);
	C(L"BUTTON", L"◀ 제거",    BS_PUSHBUTTON, 272, 220, 90, 28, ID_BTN_REMOVE);
	C(L"BUTTON", L"전체 제거", BS_PUSHBUTTON, 272, 260, 90, 28, ID_BTN_CLEAR);
	C(L"BUTTON", L"새로고침",  BS_PUSHBUTTON, 272, 310, 90, 28, ID_BTN_REFRESH);

	// ── 우측: 배치된 모델 목록 ───────────────────────────────────
	_hLblPlaced  = C(L"STATIC", L"배치된 모델 [0개]", SS_LEFT, 375, 10, 195, 18, 0);
	_hPlacedList = C(L"LISTBOX", L"",
		WS_BORDER | WS_VSCROLL | LBS_NOTIFY, 375, 32, 195, 530, ID_LIST_PLACED);
}

// ── .mesh 파일 스캔 ──────────────────────────────────────────────────────

void ItemWindow::ScanMeshFiles()
{
	if (!_hMeshList) return;
	::SendMessage(_hMeshList, LB_RESETCONTENT, 0, 0);

	std::filesystem::path base = L"../Resources";
	int cnt = 0;

	if (std::filesystem::exists(base))
	{
		std::error_code ec;
		for (auto& e : std::filesystem::recursive_directory_iterator(base, ec))
		{
			if (!e.is_regular_file(ec)) continue;
			if (e.path().extension() != L".mesh") continue;

			// Resources 기준 상대경로로 표시 (예: Models\Ch03.mesh)
			std::filesystem::path rel = std::filesystem::relative(e.path(), base, ec);
			std::wstring label = ec ? e.path().filename().wstring() : rel.wstring();
			::SendMessage(_hMeshList, LB_ADDSTRING, 0, (LPARAM)label.c_str());
			cnt++;
		}
	}

	wchar_t buf[64];
	swprintf_s(buf, L"모델 목록 [%d개]", cnt);
	if (_hLblMesh) ::SetWindowTextW(_hLblMesh, buf);
}

// ── 씬 배치 이벤트 ───────────────────────────────────────────────────────

void ItemWindow::OnPlace()
{
	int sel = (int)::SendMessage(_hMeshList, LB_GETCURSEL, 0, 0);
	if (sel == LB_ERR) return;

	auto scene = _scene.lock();
	if (!scene)
	{
		::MessageBoxW(_hWnd, L"씬이 준비되지 않았습니다.", L"오류", MB_OK | MB_ICONWARNING);
		return;
	}

	// 선택 항목: "Models\Ch03.mesh" 형태
	wchar_t buf[MAX_PATH] = {};
	::SendMessage(_hMeshList, LB_GETTEXT, sel, (LPARAM)buf);

	// 확장자 제거 → 모델 키: "Models\Ch03" → 역슬래시 정규화
	std::filesystem::path relPath(buf);
	std::wstring modelKey = (relPath.parent_path() / relPath.stem()).wstring();
	// 역슬래시를 슬래시로 (ReadModel 경로 규칙)
	for (auto& ch : modelKey) if (ch == L'\\') ch = L'/';

	// Entity 생성 → (0,0,0) 배치
	auto entity = std::make_shared<Entity>();
	entity->AddComponent(std::make_shared<Transform>());
	entity->GetTransform()->SetLocalPosition(Vec3(0.f, 0.f, 0.f));
	entity->GetTransform()->SetLocalScale(Vec3(0.01f));

	// 모델 로드 후 ModelAnimator로 렌더
	auto shader = std::make_shared<Shader>(L"../Engine/Shaders/ModelShader.hlsl");
	auto model  = std::make_shared<Model>();
	try
	{
		model->ReadModel(modelKey);
		model->ReadMaterial(modelKey);
	}
	catch (...)
	{
		::MessageBoxW(_hWnd,
			(L"모델 로드 실패:\n" + modelKey).c_str(),
			L"오류", MB_OK | MB_ICONERROR);
		return;
	}

	auto animator = std::make_shared<ModelAnimator>(shader);
	animator->SetModel(model);
	entity->AddComponent(animator);

	scene->Add(entity);
	_placedEntities.push_back(entity);

	// 배치 목록에 파일명만 표시
	std::wstring displayName = relPath.stem().wstring();
	::SendMessage(_hPlacedList, LB_ADDSTRING, 0, (LPARAM)displayName.c_str());

	wchar_t lbl[64];
	swprintf_s(lbl, L"배치된 모델 [%d개]", (int)_placedEntities.size());
	::SetWindowTextW(_hLblPlaced, lbl);
}

void ItemWindow::OnRemove()
{
	int sel = (int)::SendMessage(_hPlacedList, LB_GETCURSEL, 0, 0);
	if (sel == LB_ERR || sel >= (int)_placedEntities.size()) return;

	auto scene = _scene.lock();
	if (scene) scene->Remove(_placedEntities[sel]);

	_placedEntities.erase(_placedEntities.begin() + sel);
	::SendMessage(_hPlacedList, LB_DELETESTRING, sel, 0);

	wchar_t lbl[64];
	swprintf_s(lbl, L"배치된 모델 [%d개]", (int)_placedEntities.size());
	::SetWindowTextW(_hLblPlaced, lbl);
}

void ItemWindow::OnClear()
{
	auto scene = _scene.lock();
	if (scene)
		for (auto& e : _placedEntities)
			scene->Remove(e);

	_placedEntities.clear();
	::SendMessage(_hPlacedList, LB_RESETCONTENT, 0, 0);
	::SetWindowTextW(_hLblPlaced, L"배치된 모델 [0개]");
}

// ── 윈도우 등록 / WndProc ────────────────────────────────────────────────

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
			case ID_BTN_PLACE:   self->OnPlace();       return 0;
			case ID_BTN_REMOVE:  self->OnRemove();      return 0;
			case ID_BTN_CLEAR:   self->OnClear();       return 0;
			case ID_BTN_REFRESH: self->ScanMeshFiles(); return 0;
			}
			break;
		}
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}