#pragma once

class Scene;
class Entity;

// ── 피킹된 오브젝트 정보 구조체 ───────────────────────────────────────────
struct DetailInfo
{
	std::wstring entityLabel; // 표시용 이름 (메시명 or "Entity")

	// Model
	std::wstring modelName;
	int          boneCount  = 0;
	int          meshCount  = 0;

	// Animation
	std::wstring animName;
	int          frameCount = 0;
	float        frameRate  = 0.f;
	float        duration   = 0.f;

	// Transform
	float tx = 0.f, ty = 0.f, tz = 0.f;
	float rx = 0.f, ry = 0.f, rz = 0.f;
	float sx = 1.f, sy = 1.f, sz = 1.f;
};

// ── DetailWindow (디테일 패널) ────────────────────────────────────────────
// 3구간 구성:
//  [1] 현재 씬 이름
//  [2] 씬에 등록된 오브젝트 목록 (실시간)
//  [3] 피킹된 오브젝트 상세 정보
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
	HWND GetHWnd()   const { return _hWnd;    }

	// ── 외부에서 호출 ────────────────────────────────────────────
	// 씬 주입 (씬 변경 시마다 갱신)
	void SetScene(std::shared_ptr<Scene> scene);

	// 씬 오브젝트 목록 갱신 (Update 루프에서 주기적으로 호출)
	void RefreshEntityList();

	// 피킹 결과 반영
	void UpdateDetail(const DetailInfo& info);
	void ClearDetail();

private:
	void BuildUI();

	void RegisterWindowClass(HINSTANCE hInstance);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND      _hWnd      = nullptr;
	HINSTANCE _hInstance = nullptr;
	bool      _visible   = false;
	bool      _created   = false;

	std::weak_ptr<Scene> _scene;

	// ── [1] 씬 이름 섹션 ─────────────────────────────────────────
	HWND _hSceneName     = nullptr;

	// ── [2] 씬 오브젝트 목록 섹션 ───────────────────────────────
	HWND _hEntityCount   = nullptr;  // "오브젝트 [N개]"
	HWND _hEntityList    = nullptr;  // ListBox

	// ── [3] 피킹 정보 섹션 ──────────────────────────────────────
	HWND _hPickedLabel   = nullptr;  // "선택된 오브젝트: 없음"
	HWND _hModelName     = nullptr;
	HWND _hBoneCount     = nullptr;
	HWND _hMeshCount     = nullptr;
	HWND _hAnimName      = nullptr;
	HWND _hFrameCount    = nullptr;
	HWND _hFrameRate     = nullptr;
	HWND _hDuration      = nullptr;
	HWND _hPosX = nullptr, _hPosY = nullptr, _hPosZ = nullptr;
	HWND _hRotX = nullptr, _hRotY = nullptr, _hRotZ = nullptr;
	HWND _hSclX = nullptr, _hSclY = nullptr, _hSclZ = nullptr;

	static constexpr wchar_t CLASS_NAME[] = L"JellytoDetailPanel";
	static constexpr int ID_LIST_ENTITY   = 501;
};