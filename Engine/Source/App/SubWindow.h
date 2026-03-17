#pragma once

enum class SubMenuCmd : UINT
{
	ShowFbxConverter = 2001,
	ShowModelBrowser = 2002,
	ShowItemPanel    = 2003,
};

enum class ActivePanel
{
	None,
	FbxConverter,
	ModelBrowser,
	ItemPanel,
};

class SubWindow
{
public:
	SubWindow();
	~SubWindow();

	bool Create(HINSTANCE hInstance, HWND hMainWnd,
		const std::wstring& title, int width, int height);

	void Show();
	void Hide();
	void Toggle();

	bool IsVisible() const { return _visible; }
	HWND GetHWnd()   const { return _hWnd;    }

private:
	void CreateSubMenu();
	void SwitchPanel(ActivePanel panel);

	void BuildFbxConverterPanel();
	void BuildModelBrowserPanel();
	void BuildItemPanel();
	void ClearPanel();

	// FBX 컨버터 이벤트
	void OnBrowseModelFbx();
	void OnBrowseAnimFbx();
	void OnBrowseOutput();
	void OnExport();
	bool RunConvertModel(const std::wstring& fbxPath, const std::wstring& outputDir, const std::wstring& baseName);
	bool RunConvertAnim (const std::wstring& fbxPath, const std::wstring& outputDir, const std::wstring& baseName);
	void SetStatus(const std::wstring& msg, bool success);

	// 모델 브라우저 이벤트
	void RefreshModelList();

	void RegisterWindowClass(HINSTANCE hInstance);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND      _hWnd      = nullptr;
	HINSTANCE _hInstance = nullptr;
	bool      _visible   = false;
	bool      _created   = false;

	ActivePanel _activePanel = ActivePanel::None;

	// FBX 컨버터 컨트롤 핸들
	HWND _hModelFbxPath   = nullptr;
	HWND _hAnimFbxPath    = nullptr;
	HWND _hEditOutputPath = nullptr;
	HWND _hStatusLabel    = nullptr;

	// 모델 브라우저 컨트롤 핸들
	HWND _hListMesh  = nullptr;
	HWND _hListClip  = nullptr;
	HWND _hMeshCount = nullptr;
	HWND _hClipCount = nullptr;

	std::vector<HWND> _panelControls;

	static constexpr wchar_t CLASS_NAME[] = L"JellytoSubWindow";

	// 컨트롤 ID
	static constexpr int ID_BTN_MODEL_FBX = 301;
	static constexpr int ID_BTN_ANIM_FBX  = 302;
	static constexpr int ID_BTN_OUTPUT    = 303;
	static constexpr int ID_BTN_EXPORT    = 304;
	static constexpr int ID_STATIC_MODEL  = 305;
	static constexpr int ID_STATIC_ANIM   = 306;
	static constexpr int ID_EDIT_OUTPUT   = 307;
	static constexpr int ID_LABEL_STATUS  = 308;
	static constexpr int ID_LIST_MESH     = 309;
	static constexpr int ID_LIST_CLIP     = 310;
	static constexpr int ID_BTN_REFRESH   = 311;
};