#include "Framework.h"
#include "ToolWindow.h"
#include "Utils/Converter.h"
#include <filesystem>
#include <commdlg.h>
#include <shlobj.h>

#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")

ToolWindow::ToolWindow() {}
ToolWindow::~ToolWindow() {}

// ── 생성 ────────────────────────────────────────────────────────────────

bool ToolWindow::Create(HINSTANCE hInstance, HWND hMainWnd,
	const std::wstring& title, int width, int height)
{
	if (_created) return true;
	_hInstance = hInstance;

	RegisterWindowClass(hInstance);

	RECT mainRect = {};
	if (hMainWnd) ::GetWindowRect(hMainWnd, &mainRect);
	int x = hMainWnd ? mainRect.right + 8 : CW_USEDEFAULT;
	int y = hMainWnd ? mainRect.top       : CW_USEDEFAULT;

	RECT wr = { 0, 0, width, height };
	::AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, TRUE);

	_hWnd = ::CreateWindowW(CLASS_NAME, title.c_str(),
		WS_OVERLAPPEDWINDOW,
		x, y, wr.right - wr.left, wr.bottom - wr.top,
		hMainWnd, nullptr, hInstance, this);

	if (!_hWnd) return false;

	CreateSubMenu();
	_created = true;
	return true;
}

void ToolWindow::Show()
{
	if (!_hWnd) return;
	::ShowWindow(_hWnd, SW_SHOW);
	::SetForegroundWindow(_hWnd);
	_visible = true;
}

void ToolWindow::Hide()
{
	if (!_hWnd) return;
	::ShowWindow(_hWnd, SW_HIDE);
	_visible = false;
}

void ToolWindow::Toggle()
{
	_visible ? Hide() : Show();
}

// ── 서브 윈도우 메뉴바 ───────────────────────────────────────────────────

void ToolWindow::CreateSubMenu()
{
	HMENU hBar = ::CreateMenu();

	HMENU hFbx = ::CreatePopupMenu();
	::AppendMenuW(hFbx, MF_STRING, (UINT_PTR)SubMenuCmd::ShowFbxConverter, L"FBX 컨버터 열기");
	::AppendMenuW(hBar, MF_POPUP, (UINT_PTR)hFbx, L"FBX 컨버터(&F)");

	HMENU hModel = ::CreatePopupMenu();
	::AppendMenuW(hModel, MF_STRING, (UINT_PTR)SubMenuCmd::ShowModelBrowser, L"모델 브라우저 열기");
	::AppendMenuW(hBar, MF_POPUP, (UINT_PTR)hModel, L"모델(&M)");


	::SetMenu(_hWnd, hBar);
}

// ── 패널 전환 ────────────────────────────────────────────────────────────

void ToolWindow::SwitchPanel(ActivePanel panel)
{
	if (_activePanel == panel) return;
	ClearPanel();
	_activePanel = panel;

	switch (panel)
	{
	case ActivePanel::FbxConverter: BuildFbxConverterPanel(); break;
	case ActivePanel::ModelBrowser: BuildModelBrowserPanel(); break;
	default: break;
	}
}

void ToolWindow::ClearPanel()
{
	for (HWND h : _panelControls)
		if (::IsWindow(h)) ::DestroyWindow(h);
	_panelControls.clear();

	_hModelFbxPath   = nullptr;
	_hAnimFbxPath    = nullptr;
	_hEditOutputPath = nullptr;
	_hStatusLabel    = nullptr;
	_hListMesh       = nullptr;
	_hListClip       = nullptr;
	_hMeshCount      = nullptr;
	_hClipCount      = nullptr;
}

// ── 헬퍼 ─────────────────────────────────────────────────────────────────

namespace
{
HWND MakeCtrl(HWND parent, HINSTANCE hi,
	const wchar_t* cls, const wchar_t* text,
	DWORD style, int x, int y, int w, int h, int id,
	std::vector<HWND>& reg)
{
	HWND hw = ::CreateWindowW(cls, text, WS_CHILD | WS_VISIBLE | style,
		x, y, w, h, parent, (HMENU)(INT_PTR)id, hi, nullptr);
	reg.push_back(hw);
	return hw;
}
}

// ── FBX 컨버터 패널 ─────────────────────────────────────────────────────

void ToolWindow::BuildFbxConverterPanel()
{
	auto C = [&](const wchar_t* cls, const wchar_t* txt, DWORD s,
		int x, int y, int w, int h, int id) -> HWND
	{
		return MakeCtrl(_hWnd, _hInstance, cls, txt, s, x, y, w, h, id, _panelControls);
	};

	const int M  = 16;
	const int BW = 160;
	const int TW = 500;
	const int BH = 28;

	// Import Model FBX
	C(L"BUTTON", L"Import Model FBX", BS_PUSHBUTTON, M, 20, BW, BH, ID_BTN_MODEL_FBX);
	_hModelFbxPath = C(L"STATIC", L"선택된 파일 없음",
		SS_LEFT | SS_SUNKEN, M, 54, TW + BW + 4, 20, ID_STATIC_MODEL);

	// Import Animation FBX
	C(L"BUTTON", L"Import Animation FBX", BS_PUSHBUTTON, M, 90, BW, BH, ID_BTN_ANIM_FBX);
	_hAnimFbxPath = C(L"STATIC", L"선택된 파일 없음",
		SS_LEFT | SS_SUNKEN, M, 124, TW + BW + 4, 20, ID_STATIC_ANIM);

	// Export 경로
	C(L"STATIC", L"Export 경로", SS_LEFT, M, 164, 80, 18, 0);
	_hEditOutputPath = C(L"EDIT", L"../Resources/",
		WS_BORDER | ES_AUTOHSCROLL, M, 184, TW, BH, ID_EDIT_OUTPUT);
	C(L"BUTTON", L"폴더 선택", BS_PUSHBUTTON, M + TW + 6, 184, 100, BH, ID_BTN_OUTPUT);
	C(L"STATIC", L"※ 기본값: ../Resources/  (하위 경로 직접 입력 또는 폴더 선택 가능)",
		SS_LEFT, M, 218, TW + BW + 4, 18, 0);

	// 변환 실행
	C(L"BUTTON", L"변환 실행", BS_PUSHBUTTON,
		(TW + BW) / 2 - 50 + M, 250, 120, 32, ID_BTN_EXPORT);

	// 상태 메시지
	_hStatusLabel = C(L"STATIC", L"", SS_CENTER,
		M, 296, TW + BW + 4, 22, ID_LABEL_STATUS);
}

// ── FBX 컨버터 이벤트 ────────────────────────────────────────────────────

void ToolWindow::OnBrowseModelFbx()
{
	wchar_t buf[MAX_PATH] = {};
	OPENFILENAMEW ofn = {};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner   = _hWnd;
	ofn.lpstrFilter = L"FBX 파일 (*.fbx)\0*.fbx\0모든 파일\0*.*\0";
	ofn.lpstrFile   = buf;
	ofn.nMaxFile    = MAX_PATH;
	ofn.lpstrTitle  = L"모델 FBX 파일 선택";
	ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	if (::GetOpenFileNameW(&ofn))
		::SetWindowTextW(_hModelFbxPath, buf);
}

void ToolWindow::OnBrowseAnimFbx()
{
	wchar_t buf[MAX_PATH] = {};
	OPENFILENAMEW ofn = {};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner   = _hWnd;
	ofn.lpstrFilter = L"FBX 파일 (*.fbx)\0*.fbx\0모든 파일\0*.*\0";
	ofn.lpstrFile   = buf;
	ofn.nMaxFile    = MAX_PATH;
	ofn.lpstrTitle  = L"애니메이션 FBX 파일 선택";
	ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	if (::GetOpenFileNameW(&ofn))
		::SetWindowTextW(_hAnimFbxPath, buf);
}

void ToolWindow::OnBrowseOutput()
{
	wchar_t buf[MAX_PATH] = {};
	BROWSEINFOW bi    = {};
	bi.hwndOwner      = _hWnd;
	bi.pszDisplayName = buf;
	bi.lpszTitle      = L"출력 폴더 선택 (Resources 하위 폴더 권장)";
	bi.ulFlags        = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	LPITEMIDLIST pidl = ::SHBrowseForFolderW(&bi);
	if (pidl)
	{
		::SHGetPathFromIDListW(pidl, buf);
		::CoTaskMemFree(pidl);
		::SetWindowTextW(_hEditOutputPath, buf);
	}
}

void ToolWindow::OnExport()
{
	wchar_t modelBuf[MAX_PATH] = {};
	wchar_t animBuf [MAX_PATH] = {};
	wchar_t outBuf  [MAX_PATH] = {};

	::GetWindowTextW(_hModelFbxPath,   modelBuf, MAX_PATH);
	::GetWindowTextW(_hAnimFbxPath,    animBuf,  MAX_PATH);
	::GetWindowTextW(_hEditOutputPath, outBuf,   MAX_PATH);

	bool hasModel = wcslen(modelBuf) > 0 && wcscmp(modelBuf, L"선택된 파일 없음") != 0;
	bool hasAnim  = wcslen(animBuf)  > 0 && wcscmp(animBuf,  L"선택된 파일 없음") != 0;

	if (!hasModel && !hasAnim)
	{
		SetStatus(L"[오류] 변환할 FBX 파일을 먼저 선택해주세요.", false);
		return;
	}
	if (wcslen(outBuf) == 0)
	{
		SetStatus(L"[오류] Export 경로를 입력해주세요.", false);
		return;
	}

	::CreateDirectoryW(outBuf, nullptr);

	bool ok = true;
	std::wstring results;

	if (hasModel)
	{
		std::wstring name = std::filesystem::path(modelBuf).stem().wstring();
		bool r = RunConvertModel(modelBuf, outBuf, name);
		results += r ? (L"✔ " + name + L"  ") : (L"✘ " + name + L" (실패)  ");
		ok &= r;
	}
	if (hasAnim)
	{
		std::wstring name = std::filesystem::path(animBuf).stem().wstring();
		bool r = RunConvertAnim(animBuf, outBuf, name);
		results += r ? (L"✔ " + name + L"  ") : (L"✘ " + name + L" (실패)  ");
		ok &= r;
	}

	SetStatus(results, ok);
}

bool ToolWindow::RunConvertModel(const std::wstring& fbxPath,
	const std::wstring& outputDir, const std::wstring& baseName)
{
	try
	{
		std::wstring dir = outputDir;
		if (!dir.empty() && dir.back() != L'\\' && dir.back() != L'/') dir += L'\\';

		auto cv = std::make_shared<Converter>();
		cv->SetAssetPath(L"");
		cv->SetModelPath(dir);
		cv->SetTexturePath(dir);
		cv->ReadAssetFileAbsolute(fbxPath);
		cv->ExportMaterialData(baseName);
		cv->ExportModelData(baseName);
		return true;
	}
	catch (...) { return false; }
}

bool ToolWindow::RunConvertAnim(const std::wstring& fbxPath,
	const std::wstring& outputDir, const std::wstring& baseName)
{
	try
	{
		std::wstring dir = outputDir;
		if (!dir.empty() && dir.back() != L'\\' && dir.back() != L'/') dir += L'\\';

		auto cv = std::make_shared<Converter>();
		cv->SetAssetPath(L"");
		cv->SetModelPath(dir);
		cv->ReadAssetFileAbsolute(fbxPath);
		cv->ExportAnimationData(baseName);
		return true;
	}
	catch (...) { return false; }
}

void ToolWindow::SetStatus(const std::wstring& msg, bool success)
{
	if (!_hStatusLabel) return;
	::SetWindowTextW(_hStatusLabel, msg.c_str());
	::SetWindowLongPtr(_hStatusLabel, GWLP_USERDATA, success ? 1 : 0);
	::InvalidateRect(_hStatusLabel, nullptr, TRUE);
}

// ── 모델 브라우저 패널 ───────────────────────────────────────────────────

void ToolWindow::BuildModelBrowserPanel()
{
	auto C = [&](const wchar_t* cls, const wchar_t* txt, DWORD s,
		int x, int y, int w, int h, int id) -> HWND
	{
		return MakeCtrl(_hWnd, _hInstance, cls, txt, s, x, y, w, h, id, _panelControls);
	};

	_hMeshCount = C(L"STATIC", L"모델 (.mesh)  [0개]",       SS_LEFT, 10,  10, 300, 20, 0);
	_hClipCount = C(L"STATIC", L"애니메이션 (.clip)  [0개]", SS_LEFT, 360, 10, 300, 20, 0);

	_hListMesh = C(L"LISTBOX", L"",
		WS_BORDER | WS_VSCROLL | LBS_NOTIFY, 10,  34, 330, 580, ID_LIST_MESH);
	_hListClip = C(L"LISTBOX", L"",
		WS_BORDER | WS_VSCROLL | LBS_NOTIFY, 360, 34, 330, 580, ID_LIST_CLIP);

	C(L"BUTTON", L"새로고침", BS_PUSHBUTTON, 290, 628, 120, 30, ID_BTN_REFRESH);

	RefreshModelList();
}


// ── 모델 브라우저 이벤트 ─────────────────────────────────────────────────

void ToolWindow::RefreshModelList()
{
	if (!_hListMesh || !_hListClip) return;
	::SendMessage(_hListMesh, LB_RESETCONTENT, 0, 0);
	::SendMessage(_hListClip, LB_RESETCONTENT, 0, 0);

	// ../Resources/ 전체를 재귀 탐색 (.mesh / .clip 모두 수집)
	// 파일명만 표시하면 어느 경로인지 모르므로 Resources 기준 상대경로 표시
	std::filesystem::path baseDir = L"../Resources";
	int meshCnt = 0, clipCnt = 0;

	if (std::filesystem::exists(baseDir))
	{
		std::error_code ec;
		for (auto& e : std::filesystem::recursive_directory_iterator(baseDir, ec))
		{
			if (!e.is_regular_file(ec)) continue;
			std::wstring ext = e.path().extension().wstring();

			if (ext == L".mesh" || ext == L".clip")
			{
				// Resources 기준 상대경로 표시 (예: Models\Ch03.mesh)
				std::filesystem::path rel = std::filesystem::relative(e.path(), baseDir, ec);
				std::wstring label = ec ? e.path().filename().wstring() : rel.wstring();

				if (ext == L".mesh")
				{
					::SendMessage(_hListMesh, LB_ADDSTRING, 0, (LPARAM)label.c_str());
					meshCnt++;
				}
				else
				{
					::SendMessage(_hListClip, LB_ADDSTRING, 0, (LPARAM)label.c_str());
					clipCnt++;
				}
			}
		}
	}

	wchar_t buf[64];
	swprintf_s(buf, L"모델 (.mesh)  [%d개]", meshCnt);
	if (_hMeshCount) ::SetWindowTextW(_hMeshCount, buf);
	swprintf_s(buf, L"애니메이션 (.clip)  [%d개]", clipCnt);
	if (_hClipCount) ::SetWindowTextW(_hClipCount, buf);
}

// ── 윈도우 등록 / WndProc ────────────────────────────────────────────────

void ToolWindow::RegisterWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex   = {};
	wcex.cbSize        = sizeof(WNDCLASSEX);
	wcex.style         = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc   = ToolWindow::WndProc;
	wcex.hInstance     = hInstance;
	wcex.hCursor       = ::LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = CLASS_NAME;
	::RegisterClassExW(&wcex);
}

LRESULT CALLBACK ToolWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NCCREATE)
	{
		auto cs = reinterpret_cast<LPCREATESTRUCTW>(lParam);
		::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
		return ::DefWindowProc(hWnd, msg, wParam, lParam);
	}

	ToolWindow* self = reinterpret_cast<ToolWindow*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
	if (self)
	{
		switch (msg)
		{
		case WM_CLOSE:
			self->Hide();
			return 0;

		case WM_COMMAND:
			// 메뉴 패널 전환
			switch ((SubMenuCmd)LOWORD(wParam))
			{
			case SubMenuCmd::ShowFbxConverter:
				self->SwitchPanel(ActivePanel::FbxConverter); return 0;
			case SubMenuCmd::ShowModelBrowser:
				self->SwitchPanel(ActivePanel::ModelBrowser); return 0;
			}
			// 패널 내부 버튼
			switch (LOWORD(wParam))
			{
			case ID_BTN_MODEL_FBX: self->OnBrowseModelFbx(); return 0;
			case ID_BTN_ANIM_FBX:  self->OnBrowseAnimFbx();  return 0;
			case ID_BTN_OUTPUT:    self->OnBrowseOutput();    return 0;
			case ID_BTN_EXPORT:    self->OnExport();          return 0;
			case ID_BTN_REFRESH:   self->RefreshModelList();  return 0;
			}
			break;

		case WM_CTLCOLORSTATIC:
		{
			HWND hCtrl = (HWND)lParam;
			if (hCtrl == self->_hStatusLabel && self->_hStatusLabel)
			{
				HDC hdc  = (HDC)wParam;
				bool ok  = (::GetWindowLongPtr(hCtrl, GWLP_USERDATA) == 1);
				::SetTextColor(hdc, ok ? RGB(0, 140, 0) : RGB(180, 0, 0));
				::SetBkColor(hdc, RGB(255, 255, 255));
				return (LRESULT)::GetStockObject(WHITE_BRUSH);
			}
			break;
		}
		}
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}