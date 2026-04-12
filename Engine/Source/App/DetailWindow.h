#pragma once
#include "Interfaces/IWindow.h"

class Scene;
class Entity;

struct DetailInfo
{
	std::wstring entityLabel;

	std::wstring modelName;
	int          boneCount = 0;
	int          meshCount = 0;

	std::wstring animName;
	int          frameCount = 0;
	float        frameRate = 0.f;
	float        duration = 0.f;

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

	void SetScene(Scene* scene);

	void MarkDirty();
	void RefreshEntityList();

	void UpdateDetail(const DetailInfo& info);
	void ClearDetail();

	Entity* GetSelectedEntity() const { return _selectedEntity; }
	const std::vector<Entity*>& GetEntitySnapshot() const { return _entitySnapshot; }

	void SelectEntity(Entity* entity);

private:
	void BuildUI();
	void OnEntityListClicked();
	void FillDetailFromEntity(Entity* entity);

	void    ApplyTransform();
	float   GetEditFloat(HWND hEdit, float fallback = 0.f);
	bool    IsTransformEditFocused() const;

	void RegisterWindowClass(HINSTANCE hInstance);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND      _hWnd = nullptr;
	HINSTANCE _hInstance = nullptr;
	bool      _visible = false;
	bool      _created = false;

	bool      _listDirty = false;

	Scene*  _scene          = nullptr;
	Entity* _selectedEntity = nullptr;

	std::vector<Entity*> _entitySnapshot;

	HWND _hSceneName = nullptr;

	HWND _hEntityCount = nullptr;
	HWND _hEntityList = nullptr;

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

	HWND _hApplyBtn = nullptr;

	static constexpr wchar_t CLASS_NAME[] = L"JellytoDetailPanel";
	static constexpr int     ID_LIST_ENTITY = 501;
	static constexpr int     ID_BTN_APPLY = 502;
};