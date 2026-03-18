#pragma once

class Scene;
class Entity;

struct DetailInfo
{
	std::wstring entityLabel;

	// Model
	std::wstring modelName;
	int          boneCount = 0;
	int          meshCount = 0;

	// Animation
	std::wstring animName;
	int          frameCount = 0;
	float        frameRate = 0.f;
	float        duration = 0.f;

	// Transform
	float tx = 0.f, ty = 0.f, tz = 0.f;
	float rx = 0.f, ry = 0.f, rz = 0.f;
	float sx = 1.f, sy = 1.f, sz = 1.f;
};

class DetailWindow
{
public:
	DetailWindow();
	~DetailWindow();

	bool Create(HINSTANCE hInstance, HWND hMainWnd);

	void Show();
	void Hide();
	void Toggle();

	bool IsVisible() const { return _visible; }
	HWND GetHWnd()   const { return _hWnd; }

	// 씬 주입 및 목록 갱신
	void SetScene(std::shared_ptr<Scene> scene);
	void RefreshEntityList();

	// 피킹 결과 갱신 (MainApp에서 Ray pick 성공 시 호출)
	void UpdateDetail(const DetailInfo& info);
	void ClearDetail();

	// 현재 선택된 Entity (MainApp이 피킹 결과와 동기화에 사용)
	std::shared_ptr<Entity> GetSelectedEntity() const { return _selectedEntity; }
	const std::vector<std::shared_ptr<Entity>>& GetEntitySnapshot() const { return _entitySnapshot; }
	void SelectEntity(std::shared_ptr<Entity> entity);

private:
	void BuildUI();
	void OnEntityListClicked(); // 목록 클릭 → 해당 Entity 정보 표시
	void FillDetailFromEntity(std::shared_ptr<Entity> entity);

	void RegisterWindowClass(HINSTANCE hInstance);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND      _hWnd = nullptr;
	HINSTANCE _hInstance = nullptr;
	bool      _visible = false;
	bool      _created = false;

	std::weak_ptr<Scene>  _scene;
	std::shared_ptr<Entity> _selectedEntity;

	// Entity 목록 (ListBox 인덱스와 1:1)
	std::vector<std::shared_ptr<Entity>> _entitySnapshot;

	// ── [1] 씬 이름 ─────────────────────────────────────────────
	HWND _hSceneName = nullptr;

	// ── [2] 씬 Entity 목록 ──────────────────────────────────────
	HWND _hEntityCount = nullptr;
	HWND _hEntityList = nullptr;

	// ── [3] 선택된 오브젝트 정보 ────────────────────────────────
	HWND _hPickedLabel = nullptr;
	HWND _hModelName = nullptr;
	HWND _hBoneCount = nullptr;
	HWND _hMeshCount = nullptr;
	HWND _hAnimName = nullptr;
	HWND _hFrameCount = nullptr;
	HWND _hFrameRate = nullptr;
	HWND _hDuration = nullptr;
	HWND _hPosX = nullptr, _hPosY = nullptr, _hPosZ = nullptr;
	HWND _hRotX = nullptr, _hRotY = nullptr, _hRotZ = nullptr;
	HWND _hSclX = nullptr, _hSclY = nullptr, _hSclZ = nullptr;

	static constexpr wchar_t CLASS_NAME[] = L"JellytoDetailPanel";
	static constexpr int ID_LIST_ENTITY = 501;
};