#include "Framework.h"
#include "SceneSerializer.h"

#include "Scene/Scene.h"
#include "Entity/Entity.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Camera.h"
#include "UI/Widget.h"
#include "Scene/BlockPlacerInterface.h"

// nlohmann/json — Framework.h 를 통해 포함됨
using json = nlohmann::json;

std::unordered_map<std::wstring, SceneSerializer::ActorFactory> SceneSerializer::_factories;

void SceneSerializer::RegisterActor(const std::wstring& actorType, ActorFactory factory)
{
    _factories[actorType] = std::move(factory);
}

// ── Save ──────────────────────────────────────────────────────────────────────
bool SceneSerializer::Save(Scene* scene, const std::wstring& path, IBlockPlacer* placer)
{
    if (!scene) return false;

    // 출력 디렉터리 생성
    std::filesystem::path fsPath = WstrToStr(path);
    if (auto dir = fsPath.parent_path(); !dir.empty())
        std::filesystem::create_directories(dir);

    // ── JSON 빌드 ─────────────────────────────────────────────────────────────
    json doc;
    doc["name"] = WstrToStr(scene->GetName());

    // 엔티티 직렬화 (Widget, Camera 는 제외)
    json entities = json::array();
    for (const auto& entityPtr : scene->GetEntities())
    {
        if (!entityPtr)                                 continue;
        if (dynamic_cast<Widget*>(entityPtr.get()))     continue;
        if (entityPtr->GetComponent<Camera>())          continue;

        json e;
        e["name"] = WstrToStr(entityPtr->GetEntityName());

        const std::wstring actorType = FindActorType(entityPtr->GetEntityName());
        if (!actorType.empty())
            e["actor"] = WstrToStr(actorType);

        if (const Transform* tf = entityPtr->GetComponent<Transform>())
        {
            const Vec3 pos = tf->GetLocalPosition();
            const Vec3 rot = tf->GetLocalRotation();
            const Vec3 scl = tf->GetLocalScale();

            e["transform"] = {
                { "position", { pos.x, pos.y, pos.z } },
                { "rotation", { rot.x, rot.y, rot.z } },
                { "scale",    { scl.x, scl.y, scl.z } }
            };
        }
        entities.push_back(std::move(e));
    }
    doc["entities"] = std::move(entities);

    // 블록 직렬화
    if (placer && !placer->GetPlacedBlocks().empty())
    {
        json blocks = json::array();
        for (const PlacedBlockRecord& rec : placer->GetPlacedBlocks())
        {
            blocks.push_back({
                { "x",    rec.x    },
                { "y",    rec.y    },
                { "z",    rec.z    },
                { "type", rec.type }
            });
        }
        doc["blocks"] = std::move(blocks);
    }

    // ── 파일 쓰기 (들여쓰기 4칸) ─────────────────────────────────────────────
    std::ofstream ofs(WstrToStr(path));
    if (!ofs.is_open()) return false;
    ofs << doc.dump(4);
    return ofs.good();
}

// ── Load ──────────────────────────────────────────────────────────────────────
bool SceneSerializer::Load(Scene* scene, const std::wstring& path, IBlockPlacer* placer)
{
    if (!scene) return false;

    std::ifstream ifs(WstrToStr(path));
    if (!ifs.is_open())
    {
        ::OutputDebugStringW((L"[SceneSerializer] 파일 없음: " + path + L"\n").c_str());
        return false;
    }

    json doc;
    try
    {
        ifs >> doc;
    }
    catch (const json::parse_error& e)
    {
        ::OutputDebugStringA(e.what());
        return false;
    }

    if (placer)
        placer->ClearAllBlocks();

    // 블록 복원
    if (doc.contains("blocks") && doc["blocks"].is_array())
    {
        for (const auto& b : doc["blocks"])
        {
            const float x    = b.value("x",    0.f);
            const float y    = b.value("y",    0.f);
            const float z    = b.value("z",    0.f);
            const int   type = b.value("type", 0);
            if (placer) placer->PlaceBlock(x, y, z, type);
        }
    }

    return true;
}

// ── 유틸 ──────────────────────────────────────────────────────────────────────
std::wstring SceneSerializer::FindActorType(const std::wstring& entityName)
{
    static const std::unordered_map<std::wstring, std::wstring> kNameToType = {
        { L"SkySphere",        L"SkySphereActor" },
        { L"Floor",            L"FloorActor"     },
        { L"Cube",             L"CubeActor"      },
        { L"Sphere",           L"SphereActor"    },
        { L"Character",        L"CharacterActor" },
        { L"DirectionalLight", L"LightActor"     },
    };
    const auto it = kNameToType.find(entityName);
    return it != kNameToType.end() ? it->second : L"";
}

std::string SceneSerializer::WstrToStr(const std::wstring& wStr)
{
    if (wStr.empty()) return {};
    const int size = ::WideCharToMultiByte(
        CP_UTF8, 0, wStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<size_t>(size - 1), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, wStr.c_str(), -1, result.data(), size, nullptr, nullptr);
    return result;
}

std::wstring SceneSerializer::StrToWstr(const std::string& str)
{
    if (str.empty()) return {};
    const int size = ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring result(static_cast<size_t>(size - 1), '\0');
    ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, result.data(), size);
    return result;
}