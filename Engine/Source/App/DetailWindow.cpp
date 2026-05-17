#include "Framework.h"
#include "DetailWindow.h"

#include "Scene/Scene.h"
#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Camera.h"
#include "Resource/Material.h"
#include "Resource/Mesh.h"
#include "Entity/Components/MeshRenderer.h"
#include "Entity/Components/Collider/BaseCollider.h"
#include "Entity/Components/Collider/CollisionChannel.h"
#include "Graphics/Model/ModelAnimator.h"
#include "Graphics/Model/Model.h"
#include "Graphics/Model/ModelAnimation.h"

namespace
{
    std::wstring ColliderShapeStr(ColliderType t)
    {
        switch (t)
        {
        case ColliderType::AABB:    return L"AABB (Box)";
        case ColliderType::OBB:     return L"OBB (Box·회전)";
        case ColliderType::Sphere:  return L"Sphere";
        case ColliderType::Frustum: return L"Frustum";
        default:                    return L"알 수 없음";
        }
    }

    const wchar_t* ChannelName(CollisionChannel ch)
    {
        switch (ch)
        {
        case CollisionChannel::Default:   return L"Default";
        case CollisionChannel::Character: return L"Character";
        case CollisionChannel::Priming:   return L"Priming";
        case CollisionChannel::Mushroom:  return L"Mushroom";
        case CollisionChannel::Floor:     return L"Floor";
        case CollisionChannel::All:       return L"All";
        default:                          return L"None";
        }
    }

    std::wstring MaskToChannelStr(uint8 mask)
    {
        if (mask == 0)    return L"None";
        if (mask == 0xFF) return L"All";

        static const CollisionChannel kChannels[] = {
            CollisionChannel::Default,
            CollisionChannel::Character,
            CollisionChannel::Priming,
            CollisionChannel::Mushroom,
            CollisionChannel::Floor,
        };

        std::wstring result;
        for (CollisionChannel ch : kChannels)
        {
            if (ChannelInMask(ch, mask))
            {
                if (!result.empty()) result += L" | ";
                result += ChannelName(ch);
            }
        }
        return result.empty() ? L"None" : result;
    }

}

bool DetailWindow::Create(HINSTANCE hInstance, HWND hMainWnd)
{
    if (_created) return true;
    _hInstance = hInstance;

    RegisterWindowClass(hInstance);

    RECT mainRect = {};
    if (hMainWnd) ::GetWindowRect(hMainWnd, &mainRect);
    const int x = hMainWnd ? mainRect.right + 8 : CW_USEDEFAULT;
    const int y = hMainWnd ? mainRect.top        : CW_USEDEFAULT;

    RECT wr = { 0, 0, 360, 1060 };
    ::AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    _hWnd = ::CreateWindowW(CLASS_NAME, L"Jellyto Studio — 인스펙터",
        WS_OVERLAPPEDWINDOW,
        x, y, wr.right - wr.left, wr.bottom - wr.top,
        hMainWnd, nullptr, hInstance, this);

    if (!_hWnd) return false;

    BuildUI();
    _created = true;
    return true;
}

void DetailWindow::Show()
{
    if (!_hWnd) return;
    RefreshEntityList();
    ::ShowWindow(_hWnd, SW_SHOW);
    ::SetForegroundWindow(_hWnd);
    _visible = true;
}

void DetailWindow::Hide()
{
    if (!_hWnd) return;
    ::ShowWindow(_hWnd, SW_HIDE);
    _visible = false;
}

void DetailWindow::Toggle() { _visible ? Hide() : Show(); }

void DetailWindow::BuildUI()
{
    constexpr int W   = 330; 
    constexpr int LX  = 10;  
    constexpr int VX  = 160; 
    constexpr int LW  = 145; 
    constexpr int VW  = 170; 
    constexpr int RH  = 17;  
    constexpr int RS  = 21;  

    int y = 8;

    auto MkL = [&](const wchar_t* txt, int x, int yy, int w) -> HWND
    {
        return ::CreateWindowW(L"STATIC", txt,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            x, yy, w, RH, _hWnd, nullptr, _hInstance, nullptr);
    };

    auto MkV = [&](HWND& out, const wchar_t* def, int yy)
    {
        out = ::CreateWindowW(L"STATIC", def,
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_SUNKEN,
            VX, yy, VW, RH, _hWnd, nullptr, _hInstance, nullptr);
    };

    auto MkE = [&](HWND& out, const wchar_t* def, int yy)
    {
        out = ::CreateWindowW(L"EDIT", def,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            VX, yy, VW, RH, _hWnd, nullptr, _hInstance, nullptr);
    };

    auto MkSep = [&](const wchar_t* title, int yy)
    {
        ::CreateWindowW(L"STATIC", title,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            LX, yy, W, RH, _hWnd, nullptr, _hInstance, nullptr);
        ::CreateWindowW(L"STATIC", L"",
            WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
            LX, yy + RH + 1, W, 2, _hWnd, nullptr, _hInstance, nullptr);
    };

    MkSep(L"▶  씬 (Scene)", y); y += RH + 6;
    MkL(L"씬 이름", LX, y, LW);
    MkV(_hSceneName, L"(씬 없음)", y);
    y += RS + 10;

    MkSep(L"▶  씬 오브젝트 목록  (클릭하여 선택)", y); y += RH + 6;
    _hEntityCount = MkL(L"오브젝트 [0개]", LX, y, W);
    y += RS;
    _hEntityList = ::CreateWindowW(L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
        LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
        LX, y, W, 130, _hWnd,
        (HMENU)(INT_PTR)ID_LIST_ENTITY, _hInstance, nullptr);
    y += 138;

    MkSep(L"▶  선택된 오브젝트", y); y += RH + 6;
    _hPickedLabel = MkL(L"(없음 — 목록 클릭 또는 뷰포트 좌클릭)", LX, y, W);
    y += RS + 8;

    MkSep(L"▶  Renderer", y); y += RH + 6;

    MkL(L"렌더러 타입",   LX, y, LW); MkV(_hRendererType,  L"—", y); y += RS;
    MkL(L"Mesh 이름",     LX, y, LW); MkV(_hMeshName,      L"—", y); y += RS;
    MkL(L"Material",     LX, y, LW); MkV(_hMaterialName,  L"—", y); y += RS + 8;

    MkSep(L"▶  Collider", y); y += RH + 6;

    MkL(L"형상 타입",      LX, y, LW); MkV(_hColliderShape,     L"없음", y); y += RS;
    MkL(L"Own Channel",   LX, y, LW); MkV(_hOwnChannel,        L"—",   y); y += RS;
    MkL(L"Pickable",      LX, y, LW); MkV(_hPickableChannels,  L"—",   y); y += RS;
    MkL(L"Static",        LX, y, LW); MkV(_hIsStatic,          L"—",   y); y += RS;
    MkL(L"Extents (XYZ)", LX, y, LW); MkV(_hExtents,           L"—",   y); y += RS + 8;

    MkSep(L"▶  Model / Animation", y); y += RH + 6;

    MkL(L"모델 이름",   LX, y, LW); MkV(_hModelName,  L"—", y); y += RS;
    MkL(L"Bone 수",     LX, y, LW); MkV(_hBoneCount,  L"—", y); y += RS;
    MkL(L"Mesh 수",     LX, y, LW); MkV(_hMeshCount,  L"—", y); y += RS + 4;

    MkL(L"클립 이름",   LX, y, LW); MkV(_hAnimName,   L"—", y); y += RS;
    MkL(L"프레임 수",   LX, y, LW); MkV(_hFrameCount, L"—", y); y += RS;
    MkL(L"Frame Rate", LX, y, LW); MkV(_hFrameRate,  L"—", y); y += RS;
    MkL(L"Duration(s)",LX, y, LW); MkV(_hDuration,   L"—", y); y += RS + 8;

    MkSep(L"▶  Transform  [편집 후 Apply 또는 Enter]", y); y += RH + 6;

    MkL(L"Position  X",  LX, y, LW); MkE(_hPosX, L"0.000", y); y += RS;
    MkL(L"Position  Y",  LX, y, LW); MkE(_hPosY, L"0.000", y); y += RS;
    MkL(L"Position  Z",  LX, y, LW); MkE(_hPosZ, L"0.000", y); y += RS + 2;
    MkL(L"Rotation  X°", LX, y, LW); MkE(_hRotX, L"0.000", y); y += RS;
    MkL(L"Rotation  Y°", LX, y, LW); MkE(_hRotY, L"0.000", y); y += RS;
    MkL(L"Rotation  Z°", LX, y, LW); MkE(_hRotZ, L"0.000", y); y += RS + 2;
    MkL(L"Scale     X",  LX, y, LW); MkE(_hSclX, L"1.000", y); y += RS;
    MkL(L"Scale     Y",  LX, y, LW); MkE(_hSclY, L"1.000", y); y += RS;
    MkL(L"Scale     Z",  LX, y, LW); MkE(_hSclZ, L"1.000", y); y += RS + 8;

    _hApplyBtn = ::CreateWindowW(L"BUTTON", L"Apply Transform",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        LX, y, W, 28, _hWnd,
        (HMENU)(INT_PTR)ID_BTN_APPLY, _hInstance, nullptr);
}

void DetailWindow::SetScene(Scene* scene)
{
    _scene = scene;
    if (_hSceneName)
        ::SetWindowTextW(_hSceneName, scene ? scene->GetName().c_str() : L"(씬 없음)");
    _listDirty = true;
    RefreshEntityList();
}

void DetailWindow::MarkDirty() { _listDirty = true; }

void DetailWindow::RefreshEntityList()
{
    if (!_hEntityList || !_listDirty) return;
    _listDirty = false;

    const int prevTop = (int)::SendMessage(_hEntityList, LB_GETTOPINDEX, 0, 0);
    Entity* prevSelected = _selectedEntity;

    ::SendMessage(_hEntityList, LB_RESETCONTENT, 0, 0);
    _entitySnapshot.clear();

    if (!_scene)
    {
        ::SetWindowTextW(_hEntityCount, L"오브젝트 [씬 없음]");
        if (_hSceneName) ::SetWindowTextW(_hSceneName, L"(씬 없음)");
        return;
    }

    if (_hSceneName) ::SetWindowTextW(_hSceneName, _scene->GetName().c_str());

    int newSelIdx = -1;
    int idx       = 0;

    for (const auto& entityPtr : _scene->GetEntities())
    {
        if (!entityPtr) continue;

        std::wstring label = entityPtr->GetEntityName();

        if (auto* col = entityPtr->GetComponent<BaseCollider>())
        {
            label += L"  ✔";
            if (col->IsStatic()) label += L"(S)";
        }
        else
        {
            label += L"  ✕";
        }

        ::SendMessage(_hEntityList, LB_ADDSTRING, 0, (LPARAM)label.c_str());

        if (entityPtr.get() == prevSelected)
            newSelIdx = idx;

        _entitySnapshot.push_back(entityPtr.get());
        ++idx;
    }

    wchar_t buf[80];
    swprintf_s(buf, L"오브젝트 [%d개]  (✔=Collider / S=Static)",
               (int)_entitySnapshot.size());
    ::SetWindowTextW(_hEntityCount, buf);

    if (prevTop >= 0)
        ::SendMessage(_hEntityList, LB_SETTOPINDEX, prevTop, 0);

    if (newSelIdx >= 0)
        ::SendMessage(_hEntityList, LB_SETCURSEL, newSelIdx, 0);
}

void DetailWindow::SelectEntity(Entity* entity)
{
    _selectedEntity = entity;
    if (!_hEntityList || !entity) return;

    for (int i = 0; i < (int)_entitySnapshot.size(); ++i)
    {
        if (_entitySnapshot[i] == entity)
        {
            ::SendMessage(_hEntityList, LB_SETCURSEL, i, 0);
            FillDetailFromEntity(entity);
            return;
        }
    }
}

void DetailWindow::OnEntityListClicked()
{
    const int sel = (int)::SendMessage(_hEntityList, LB_GETCURSEL, 0, 0);
    if (sel == LB_ERR || sel >= (int)_entitySnapshot.size())
    {
        _selectedEntity = nullptr;
        ClearDetail();
        return;
    }
    _selectedEntity = _entitySnapshot[sel];
    FillDetailFromEntity(_selectedEntity);
}

void DetailWindow::FillDetailFromEntity(Entity* entity)
{
    if (!entity) { ClearDetail(); return; }

    DetailInfo info;
    info.entityLabel = entity->GetEntityName();

    if (ModelAnimator* animator = entity->GetComponent<ModelAnimator>())
    {
        info.rendererType = L"ModelAnimator";

        if (auto mdl = animator->GetModel())
        {
            info.modelName = (mdl->GetMeshCount() > 0)
                ? mdl->GetMeshByIndex(0)->name : L"Unknown";
            info.entityLabel = info.modelName;
            info.meshName    = info.modelName;
            info.boneCount   = (int)mdl->GetBoneCount();
            info.meshCount   = (int)mdl->GetMeshCount();

            if (mdl->GetAnimationCount() > 0)
            {
                ModelAnimation* anim = mdl->GetAnimationByIndex(0);
                info.animName   = anim->name;
                info.frameCount = (int)anim->frameCount;
                info.frameRate  = anim->frameRate;
                info.duration   = anim->duration;
            }
        }
    }
    else if (MeshRenderer* renderer = entity->GetComponent<MeshRenderer>())
    {
        info.rendererType = L"MeshRenderer";

        if (auto mesh = renderer->GetMesh())
            info.meshName = mesh->GetName();

        if (auto mat = renderer->GetMaterial())
            info.materialName = mat->GetName();
    }
    else
    {
        info.rendererType = L"없음";
    }

    if (BaseCollider* col = entity->GetComponent<BaseCollider>())
    {
        info.hasCollider      = true;
        info.colliderShape    = ColliderShapeStr(col->GetColliderType());
        info.ownChannel       = ChannelName(col->GetOwnChannel());
        info.pickableChannels = MaskToChannelStr(col->GetPickableMask());
        info.isStatic         = col->IsStatic();

        const Vec3 ext        = col->GetOffsetScale();
        info.extX = ext.x;
        info.extY = ext.y;
        info.extZ = ext.z;
    }

    if (Transform* tf = entity->GetComponent<Transform>())
    {
        const Vec3 pos = tf->GetLocalPosition();
        const Vec3 rot = tf->GetLocalRotation();
        const Vec3 scl = tf->GetLocalScale();

        info.tx = pos.x; info.ty = pos.y; info.tz = pos.z;
        info.rx = XMConvertToDegrees(rot.x);
        info.ry = XMConvertToDegrees(rot.y);
        info.rz = XMConvertToDegrees(rot.z);
        info.sx = scl.x; info.sy = scl.y; info.sz = scl.z;
    }

    UpdateDetail(info);
}

void DetailWindow::UpdateDetail(const DetailInfo& info)
{
    if (!_hWnd) return;

    auto Set  = [](HWND h, const wchar_t* t)  { if (h) ::SetWindowTextW(h, t); };
    auto SetW = [&](HWND h, const std::wstring& s)
    {
        Set(h, s.empty() ? L"—" : s.c_str());
    };
    auto SetF = [](HWND h, float v)
    {
        if (!h) return;
        wchar_t b[32]; swprintf_s(b, L"%.3f", v);
        ::SetWindowTextW(h, b);
    };
    auto SetI = [](HWND h, int v)
    {
        if (!h) return;
        wchar_t b[16]; swprintf_s(b, L"%d", v);
        ::SetWindowTextW(h, b);
    };

    Set(_hPickedLabel, (L"선택됨: " + info.entityLabel).c_str());

    SetW(_hRendererType,  info.rendererType);
    SetW(_hMeshName,      info.meshName);
    SetW(_hMaterialName,  info.materialName);

    if (info.hasCollider)
    {
        SetW(_hColliderShape,    info.colliderShape);
        SetW(_hOwnChannel,       info.ownChannel);
        SetW(_hPickableChannels, info.pickableChannels);
        Set(_hIsStatic,          info.isStatic ? L"예  (Static)" : L"아니요");

        wchar_t extBuf[64];
        swprintf_s(extBuf, L"(%.2f, %.2f, %.2f)", info.extX, info.extY, info.extZ);
        Set(_hExtents, extBuf);
    }
    else
    {
        Set(_hColliderShape,    L"없음");
        Set(_hOwnChannel,       L"—");
        Set(_hPickableChannels, L"—");
        Set(_hIsStatic,         L"—");
        Set(_hExtents,          L"—");
    }

    SetW(_hModelName,  info.modelName);
    SetI(_hBoneCount,  info.boneCount);
    SetI(_hMeshCount,  info.meshCount);
    SetW(_hAnimName,   info.animName);
    SetI(_hFrameCount, info.frameCount);
    SetF(_hFrameRate,  info.frameRate);
    SetF(_hDuration,   info.duration);

    if (!IsTransformEditFocused())
    {
        SetF(_hPosX, info.tx); SetF(_hPosY, info.ty); SetF(_hPosZ, info.tz);
        SetF(_hRotX, info.rx); SetF(_hRotY, info.ry); SetF(_hRotZ, info.rz);
        SetF(_hSclX, info.sx); SetF(_hSclY, info.sy); SetF(_hSclZ, info.sz);
    }
}

void DetailWindow::ClearDetail()
{
    if (!_hWnd) return;

    auto Set = [](HWND h, const wchar_t* t) { if (h) ::SetWindowTextW(h, t); };

    Set(_hPickedLabel, L"(없음 — 목록 클릭 또는 뷰포트 좌클릭)");

    Set(_hRendererType,   L"—");
    Set(_hMeshName,       L"—");
    Set(_hMaterialName,   L"—");

    Set(_hColliderShape,    L"없음");
    Set(_hOwnChannel,       L"—");
    Set(_hPickableChannels, L"—");
    Set(_hIsStatic,         L"—");
    Set(_hExtents,          L"—");

    Set(_hModelName,  L"—"); Set(_hBoneCount, L"—"); Set(_hMeshCount, L"—");
    Set(_hAnimName,   L"—"); Set(_hFrameCount, L"—");
    Set(_hFrameRate,  L"—"); Set(_hDuration,   L"—");

    Set(_hPosX, L"0.000"); Set(_hPosY, L"0.000"); Set(_hPosZ, L"0.000");
    Set(_hRotX, L"0.000"); Set(_hRotY, L"0.000"); Set(_hRotZ, L"0.000");
    Set(_hSclX, L"1.000"); Set(_hSclY, L"1.000"); Set(_hSclZ, L"1.000");
}

bool DetailWindow::IsTransformEditFocused() const
{
    HWND focused = ::GetFocus();
    return focused == _hPosX || focused == _hPosY || focused == _hPosZ
        || focused == _hRotX || focused == _hRotY || focused == _hRotZ
        || focused == _hSclX || focused == _hSclY || focused == _hSclZ;
}

float DetailWindow::GetEditFloat(HWND hEdit, float fallback)
{
    if (!hEdit) return fallback;
    wchar_t buf[64] = {};
    ::GetWindowTextW(hEdit, buf, 64);
    try { return std::stof(buf); }
    catch (...) { return fallback; }
}

void DetailWindow::ApplyTransform()
{
    if (!_selectedEntity) return;
    Transform* tf = _selectedEntity->GetComponent<Transform>();
    if (!tf) return;

    tf->SetLocalPosition(Vec3(
        GetEditFloat(_hPosX), GetEditFloat(_hPosY), GetEditFloat(_hPosZ)));
    tf->SetLocalRotation(Vec3(
        XMConvertToRadians(GetEditFloat(_hRotX)),
        XMConvertToRadians(GetEditFloat(_hRotY)),
        XMConvertToRadians(GetEditFloat(_hRotZ))));
    tf->SetLocalScale(Vec3(
        GetEditFloat(_hSclX, 1.f),
        GetEditFloat(_hSclY, 1.f),
        GetEditFloat(_hSclZ, 1.f)));
}

void DetailWindow::RegisterWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex    = {};
    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = DetailWindow::WndProc;
    wcex.hInstance      = hInstance;
    wcex.hCursor        = ::LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName  = CLASS_NAME;
    ::RegisterClassExW(&wcex);
}

LRESULT CALLBACK DetailWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        auto cs = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        ::SetWindowLongPtr(hWnd, GWLP_USERDATA,
                           reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return ::DefWindowProc(hWnd, msg, wParam, lParam);
    }

    DetailWindow* self = reinterpret_cast<DetailWindow*>(
        ::GetWindowLongPtr(hWnd, GWLP_USERDATA));

    if (self)
    {
        switch (msg)
        {
        case WM_CLOSE:
            self->Hide();
            return 0;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_BTN_APPLY && HIWORD(wParam) == BN_CLICKED)
            {
                self->ApplyTransform();
                return 0;
            }
            if (LOWORD(wParam) == ID_LIST_ENTITY && HIWORD(wParam) == LBN_SELCHANGE)
            {
                self->OnEntityListClicked();
                return 0;
            }
            break;

        case WM_KEYDOWN:
            if (wParam == VK_RETURN && self->IsTransformEditFocused())
            {
                self->ApplyTransform();
                return 0;
            }
            break;
        }
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}