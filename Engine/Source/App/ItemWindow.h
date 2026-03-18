#pragma once

class Scene;
class Entity;

// ItemWindow — .mesh 파일 목록 표시 + 씬 배치
// SetScene()으로 Scene을 주입받아야 배치 기능이 동작함
class ItemWindow
{
public:
	ItemWindow();
	~ItemWindow();

	bool Create(HINSTANCE hInstance, HWND hMainWnd);

	void Show();
	void Hide();
	void Toggle();

	bool IsVisible() const { return _visible; }
	HWND GetHWnd()   const { return _hWnd;    }

	// 씬 주입 (MainApp::Init() 이후 호출)
	void SetScene(std::shared_ptr<Scene> scene) { _scene = scene; }

private:
	void BuildUI();
	void ScanMeshFiles();  // ../Resources 재귀 탐색 → .mesh 목록
	void OnPlace();        // 선택 .mesh → 씬 (0,0,0) 배치
	void OnRemove();       // 배치된 선택 항목 씬에서 제거
	void OnClear();        // 전체 제거

	void RegisterWindowClass(HINSTANCE hInstance);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND      _hWnd      = nullptr;
	HINSTANCE _hInstance = nullptr;
	bool      _visible   = false;
	bool      _created   = false;

	std::weak_ptr<Scene> _scene;

	// 배치된 Entity 목록 (PlacedList 인덱스와 1:1 대응)
	std::vector<std::shared_ptr<Entity>> _placedEntities;

	HWND _hLblMesh    = nullptr;  // "모델 목록 [N개]"
	HWND _hMeshList   = nullptr;  // .mesh 파일 목록
	HWND _hLblPlaced  = nullptr;  // "배치된 모델 [N개]"
	HWND _hPlacedList = nullptr;  // 배치된 모델 이름 목록

	static constexpr wchar_t CLASS_NAME[] = L"JellytoItemWindow";

	static constexpr int ID_LIST_MESH    = 401;
	static constexpr int ID_LIST_PLACED  = 402;
	static constexpr int ID_BTN_PLACE    = 403;
	static constexpr int ID_BTN_REMOVE   = 404;
	static constexpr int ID_BTN_CLEAR    = 405;
	static constexpr int ID_BTN_REFRESH  = 406;
};