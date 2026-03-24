#include "Framework.h"
#include "SceneSerializer.h"

#include "Scene/Scene.h"
#include "Entity/Entity.h"
#include "Entity/Actor.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Light.h"
#include "Entity/Components/Camera.h"
#include "UI/Widget.h"
#include "Utils/tinyxml2.h"

#include <filesystem>

#include "Scene/BlockPlacerInterface.h"

using namespace tinyxml2;

std::unordered_map<std::wstring, SceneSerializer::ActorFactory> SceneSerializer::_factories;

void SceneSerializer::RegisterActor(const std::wstring& actorType, ActorFactory factory)
{
    _factories[actorType] = factory;
}

// ── Save ──────────────────────────────────────────────────────────────────
bool SceneSerializer::Save(const std::shared_ptr<Scene>& scene,
    const std::wstring& path,
    IBlockPlacer* placer)
{
    if (!scene) return false;

    // 폴더 자동 생성
    std::filesystem::path fsPath = WstrToStr(path);
    auto dir = fsPath.parent_path();
    if (!dir.empty() && !std::filesystem::exists(dir))
    {
        std::filesystem::create_directories(dir);
        ::OutputDebugStringW((L"[SceneSerializer] 폴더 생성: " + StrToWstr(dir.string()) + L"\n").c_str());
    }

    tinyxml2::XMLDocument doc;
    doc.InsertFirstChild(doc.NewDeclaration());

    XMLElement* sceneElem = doc.NewElement("Scene");
    sceneElem->SetAttribute("name", WstrToStr(scene->GetName()).c_str());
    doc.InsertEndChild(sceneElem);

    // ── Entity 저장 ──────────────────────────────────────────────
    for (auto& entity : scene->GetEntities())
    {
        if (!entity) continue;
        if (std::dynamic_pointer_cast<Widget>(entity)) continue;
        if (entity->GetComponent<Camera>()) continue;

        std::wstring actorType = FindActorType(entity->GetEntityName());

        XMLElement* entityElem = doc.NewElement("Entity");
        entityElem->SetAttribute("name", WstrToStr(entity->GetEntityName()).c_str());
        if (!actorType.empty())
            entityElem->SetAttribute("actor", WstrToStr(actorType).c_str());

        if (auto tf = entity->GetTransform())
        {
            Vec3 pos = tf->GetLocalPosition();
            Vec3 rot = tf->GetLocalRotation();
            Vec3 scl = tf->GetLocalScale();

            XMLElement* tfElem = doc.NewElement("Transform");
            tfElem->SetAttribute("px", pos.x); tfElem->SetAttribute("py", pos.y); tfElem->SetAttribute("pz", pos.z);
            tfElem->SetAttribute("rx", rot.x); tfElem->SetAttribute("ry", rot.y); tfElem->SetAttribute("rz", rot.z);
            tfElem->SetAttribute("sx", scl.x); tfElem->SetAttribute("sy", scl.y); tfElem->SetAttribute("sz", scl.z);
            entityElem->InsertEndChild(tfElem);
        }

        sceneElem->InsertEndChild(entityElem);
    }

    // ── 블록 저장 (placer 있을 때만) ─────────────────────────────
    if (placer && !placer->GetPlacedBlocks().empty())
    {
        XMLElement* blocksElem = doc.NewElement("Blocks");
        for (auto& [col, row] : placer->GetPlacedBlocks())
        {
            XMLElement* blockElem = doc.NewElement("Block");
            blockElem->SetAttribute("col", col);
            blockElem->SetAttribute("row", row);
            blocksElem->InsertEndChild(blockElem);
        }
        sceneElem->InsertEndChild(blocksElem);
    }

    XMLError err = doc.SaveFile(WstrToStr(path).c_str());
    if (err != XML_SUCCESS)
    {
        ::OutputDebugStringW((L"[SceneSerializer] 저장 실패: " + path + L"\n").c_str());
        return false;
    }

    int blockCount = placer ? (int)placer->GetPlacedBlocks().size() : 0;
    wchar_t dbg[128];
    swprintf_s(dbg, L"[SceneSerializer] 저장 완료: %s  (블록 %d개)\n", path.c_str(), blockCount);
    ::OutputDebugStringW(dbg);
    return true;
}

// ── Load ──────────────────────────────────────────────────────────────────
bool SceneSerializer::Load(const std::shared_ptr<Scene>& scene,
    const std::wstring& path,
    IBlockPlacer* placer)
{
    if (!scene) return false;

    tinyxml2::XMLDocument doc;
    XMLError err = doc.LoadFile(WstrToStr(path).c_str());
    if (err != XML_SUCCESS)
    {
        ::OutputDebugStringW((L"[SceneSerializer] 파일 없음: " + path + L"\n").c_str());
        return false;
    }

    XMLElement* sceneElem = doc.FirstChildElement("Scene");
    if (!sceneElem) return false;

    if (const char* name = sceneElem->Attribute("name"))
        scene->SetName(StrToWstr(name));

    // 기존 씬 오브젝트 정리 (Widget, Camera 제외)
    std::vector<std::shared_ptr<Entity>> toRemove;
    for (auto& entity : scene->GetEntities())
    {
        if (!entity) continue;
        if (std::dynamic_pointer_cast<Widget>(entity)) continue;
        if (entity->GetComponent<Camera>()) continue;
        toRemove.push_back(entity);
    }
    for (auto& entity : toRemove)
        scene->Remove(entity);

    // 기존 블록 데이터도 초기화
    if (placer)
        placer->ClearAllBlocks();

    // ── Entity 복원 ──────────────────────────────────────────────
    for (XMLElement* entityElem = sceneElem->FirstChildElement("Entity");
        entityElem;
        entityElem = entityElem->NextSiblingElement("Entity"))
    {
        const char* nameAttr = entityElem->Attribute("name");
        const char* actorAttr = entityElem->Attribute("actor");

        std::wstring entityName = nameAttr ? StrToWstr(nameAttr) : L"Entity";
        std::wstring actorType = actorAttr ? StrToWstr(actorAttr) : L"";

        std::shared_ptr<Entity> entity;

        if (!actorType.empty())
        {
            auto it = _factories.find(actorType);
            if (it != _factories.end())
            {
                auto actor = it->second();
                if (actor)
                {
                    actor->Spawn(scene);
                    entity = actor->GetEntity();
                    if (auto light = entity->GetComponent<Light>())
                        scene->SetMainLight(light);
                }
            }
            else
            {
                ::OutputDebugStringW((L"[SceneSerializer] 미등록 Actor: " + actorType + L"\n").c_str());
            }
        }
        else
        {
            entity = std::make_shared<Entity>(entityName);
            scene->Add(entity);
        }

        if (!entity) continue;

        if (XMLElement* tfElem = entityElem->FirstChildElement("Transform"))
        {
            Vec3 pos = {}, rot = {}, scl = { 1,1,1 };
            tfElem->QueryFloatAttribute("px", &pos.x); tfElem->QueryFloatAttribute("py", &pos.y); tfElem->QueryFloatAttribute("pz", &pos.z);
            tfElem->QueryFloatAttribute("rx", &rot.x); tfElem->QueryFloatAttribute("ry", &rot.y); tfElem->QueryFloatAttribute("rz", &rot.z);
            tfElem->QueryFloatAttribute("sx", &scl.x); tfElem->QueryFloatAttribute("sy", &scl.y); tfElem->QueryFloatAttribute("sz", &scl.z);
            if (auto tf = entity->GetTransform())
            {
                tf->SetLocalPosition(pos);
                tf->SetLocalRotation(rot);
                tf->SetLocalScale(scl);
            }
        }
    }

    // ── 블록 복원 (placer 있을 때만) ─────────────────────────────
    int blockCount = 0;
    if (placer)
    {
        if (XMLElement* blocksElem = sceneElem->FirstChildElement("Blocks"))
        {
            for (XMLElement* blockElem = blocksElem->FirstChildElement("Block");
                blockElem;
                blockElem = blockElem->NextSiblingElement("Block"))
            {
                int col = 0, row = 0;
                blockElem->QueryIntAttribute("col", &col);
                blockElem->QueryIntAttribute("row", &row);
                placer->PlaceBlock(col, row);
                blockCount++;
            }
        }
    }

    wchar_t dbg[128];
    swprintf_s(dbg, L"[SceneSerializer] 로드 완료: %s  (블록 %d개)\n", path.c_str(), blockCount);
    ::OutputDebugStringW(dbg);
    return true;
}

// ── 유틸 ─────────────────────────────────────────────────────────────────
std::wstring SceneSerializer::FindActorType(const std::wstring& entityName)
{
    static const std::unordered_map<std::wstring, std::wstring> nameToType = {
        { L"SkySphere",        L"SkySphereActor" },
        { L"Floor",            L"FloorActor"     },
        { L"Cube",             L"CubeActor"      },
        { L"Sphere",           L"SphereActor"    },
        { L"Character",        L"CharacterActor" },
        { L"DirectionalLight", L"LightActor"     },
    };
    auto it = nameToType.find(entityName);
    return it != nameToType.end() ? it->second : L"";
}

std::string SceneSerializer::WstrToStr(const std::wstring& w)
{
    if (w.empty()) return {};
    int size = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(size - 1, 0);
    ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, result.data(), size, nullptr, nullptr);
    return result;
}

std::wstring SceneSerializer::StrToWstr(const std::string& s)
{
    if (s.empty()) return {};
    int size = ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring result(size - 1, 0);
    ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, result.data(), size);
    return result;
}