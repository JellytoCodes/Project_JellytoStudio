#pragma once

// DetailWindow — Pick한 오브젝트의 상세 정보 표시
// - Model 정보 (메시 이름, 본 수 등)
// - Animation 정보 (클립 이름, 프레임 수, 재생 속도 등)
// - Transform 정보 (Position, Rotation, Scale)
// UpdateDetail()을 외부에서 호출해서 내용을 갱신

struct DetailInfo
{
	// Model
	std::wstring modelName;
	int          boneCount    = 0;
	int          meshCount    = 0;

	// Animation
	std::wstring animName;
	int          frameCount   = 0;
	float        frameRate    = 0.f;
	float        duration     = 0.f;

	// Transform
	float tx = 0.f, ty = 0.f, tz = 0.f;   // Position
	float rx = 0.f, ry = 0.f, rz = 0.f;   // Rotation (degree)
	float sx = 1.f, sy = 1.f, sz = 1.f;   // Scale
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

	// 외부에서 Pick 결과를 전달해서 갱신
	void UpdateDetail(const DetailInfo& info);
	void ClearDetail();

	bool IsVisible() const { return _visible; }
	HWND GetHWnd()   const { return _hWnd;    }

private:
	void BuildUI();

	void RegisterWindowClass(HINSTANCE hInstance);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND      _hWnd      = nullptr;
	HINSTANCE _hInstance = nullptr;
	bool      _visible   = false;
	bool      _created   = false;

	// ── Model 섹션 ───────────────────────────────────────────────
	HWND _hModelName    = nullptr;
	HWND _hBoneCount    = nullptr;
	HWND _hMeshCount    = nullptr;

	// ── Animation 섹션 ───────────────────────────────────────────
	HWND _hAnimName     = nullptr;
	HWND _hFrameCount   = nullptr;
	HWND _hFrameRate    = nullptr;
	HWND _hDuration     = nullptr;

	// ── Transform 섹션 ───────────────────────────────────────────
	HWND _hPosX = nullptr, _hPosY = nullptr, _hPosZ = nullptr;
	HWND _hRotX = nullptr, _hRotY = nullptr, _hRotZ = nullptr;
	HWND _hSclX = nullptr, _hSclY = nullptr, _hSclZ = nullptr;

	static constexpr wchar_t CLASS_NAME[] = L"JellytoDetailWindow";
};