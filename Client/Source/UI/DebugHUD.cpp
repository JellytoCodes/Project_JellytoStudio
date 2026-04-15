
#include "pch.h"
#include "DebugHUD.h"

#include "Entity/Components/Camera.h"
#include "Graphics/Managers/InstancingManager.h"
#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"
#include "UI/UIManager.h"

void DebugHUD::Init(Camera* camera)
{
    _pCamera = camera;
    _lines.resize(kLineCount);
}

void DebugHUD::Update()
{
    if (GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::F2))
        _visible = !_visible;
}

void DebugHUD::RebuildLines()
{
    const auto& rs = GET_SINGLE(InstancingManager)->GetStats();
    const auto& cs = _pCamera ? _pCamera->GetCullStats() : Camera::CullStats{};
    const uint32 fps = GET_SINGLE(TimeManager)->GetFps();

    const float cullPct = cs.totalEntities > 0 ? cs.culledEntities * 100.f / cs.totalEntities : 0.f;
    const float instEff = (rs.totalInstances > 0 && rs.modelDrawCalls > 0) ? (1.f - static_cast<float>(rs.modelDrawCalls) / rs.totalInstances) * 100.f : 0.f;

    const std::wstring newTexts[kLineCount] = {
        L"[ RENDER STATS ]",
        std::wstring(L"FPS: ") + std::to_wstring(fps)
            + L"   DrawCall: " + std::to_wstring(rs.totalDrawCalls),
        std::wstring(L"Visible: ") + std::to_wstring(cs.visibleEntities)
            + L"  / Total: " + std::to_wstring(cs.totalEntities),
        std::wstring(L"Culled: ") + std::to_wstring(cs.culledEntities)
            + L"  (" + std::to_wstring(static_cast<int>(cullPct)) + L"%)",
        std::wstring(L"Instances: ") + std::to_wstring(rs.totalInstances)
            + L"  Eff: " + std::to_wstring(static_cast<int>(instEff)) + L"%",
    };

    for (int i = 0; i < kLineCount; i++)
    {
        if (_lines[i].text == newTexts[i]) continue;
        _lines[i].text = newTexts[i];
        _lines[i].srv  = nullptr;
    }
}

void DebugHUD::Render()
{
    if (!_visible) return;

    RebuildLines();

    const float panelH = kPadY * 2 + kLineCount * kLineH;

    GET_SINGLE(UIManager)->AddRect(
        kPanelX, kPanelY, kPanelW, panelH,
        Color(0.f, 0.f, 0.f, 0.65f));

    for (int i = 0; i < kLineCount; i++)
    {
        const float tx = kPanelX + kPadX;
        const float ty = kPanelY + kPadY + i * kLineH;

        const Color col = (i == 0)
            ? Color(1.f, 0.95f, 0.3f, 1.f)
            : Color(1.f, 1.f,   1.f, 1.f);

        GET_SINGLE(UIManager)->AddText(
            _lines[i].text, tx, ty, kPanelW - kPadX * 2, kLineH,
            col, kFontSize);
    }
}