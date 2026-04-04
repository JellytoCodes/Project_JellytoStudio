#pragma once
#include "Interfaces/IWindow.h"

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

class DetailWindow : public IWindow
{
public:
	virtual bool Create(HINSTANCE hInstance, HWND hMainWnd) override;

	virtual void Show()   override;
	virtual void Hide()   override;
	virtual void Toggle() override;

	virtual bool IsVisible() const override { return _visible; }
	virtual HWND GetHWnd()   const override { return _hWnd; }

	void SetScene(std::shared_ptr<Scene> scene);

	// Entity 추가/삭제 등 씬 구조가 변경됐을 때만 호출
	// (매 프레임 틱 호출 X)
	void MarkDirty();
	void RefreshEntityList();

	void UpdateDetail(const DetailInfo& info);
	void ClearDetail();

	std::shared_ptr<Entity> GetSelectedEntity() const { return _selectedEntity; }
	const std::vector<std::shared_ptr<Entity>>& GetEntitySnapshot() const { return _entitySnapshot; }
	void SelectEntity(std::shared_ptr<Entity> entity);

private:
	void BuildUI();
	void OnEntityListClicked();
	void FillDetailFromEntity(std::shared_ptr<Entity> entity);

	// Transform 편집
	void    ApplyTransform();
	float   GetEditFloat(HWND hEdit, float fallback = 0.f);
	bool    IsTransformEditFocused() const;   // 9개 Edit 컨트롤 중 하나가 포커스 중인지

	void RegisterWindowClass(HINSTANCE hInstance);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND      _hWnd = nullptr;
	HINSTANCE _hInstance = nullptr;
	bool      _visible = false;
	bool      _created = false;

	// Dirty 플래그 — 씬 구조 변경 시 true, Refresh 후 false
	bool      _listDirty = false;

	std::weak_ptr<Scene>    _scene;
	std::shared_ptr<Entity> _selectedEntity;
	std::vector<std::shared_ptr<Entity>> _entitySnapshot;

	// [1] 씬
	HWND _hSceneName = nullptr;

	// [2] Entity 목록
	HWND _hEntityCount = nullptr;
	HWND _hEntityList = nullptr;

	// [3] 오브젝트 정보 (읽기전용)
	HWND _hPickedLabel = nullptr;
	HWND _hModelName = nullptr;
	HWND _hBoneCount = nullptr;
	HWND _hMeshCount = nullptr;
	HWND _hAnimName = nullptr;
	HWND _hFrameCount = nullptr;
	HWND _hFrameRate = nullptr;
	HWND _hDuration = nullptr;

	// [4] Transform Edit 컨트롤 (읽기/쓰기)
	HWND _hPosX = nullptr, _hPosY = nullptr, _hPosZ = nullptr;
	HWND _hRotX = nullptr, _hRotY = nullptr, _hRotZ = nullptr;
	HWND _hSclX = nullptr, _hSclY = nullptr, _hSclZ = nullptr;

	// Apply 버튼
	HWND _hApplyBtn = nullptr;

	static constexpr wchar_t CLASS_NAME[] = L"JellytoDetailPanel";
	static constexpr int     ID_LIST_ENTITY = 501;
	static constexpr int     ID_BTN_APPLY = 502;
};