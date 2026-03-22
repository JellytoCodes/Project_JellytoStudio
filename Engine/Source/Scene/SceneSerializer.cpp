#include "Framework.h"
#include "SceneSerializer.h"

#include "Scene/Scene.h"
#include "Entity/Entity.h"
#include "Entity/Actor.h"
#include "Entity/Components/Transform.h"
#include "Utils/tinyxml2.h"

using namespace tinyxml2;

std::unordered_map<std::wstring, SceneSerializer::ActorFactory> SceneSerializer::_factories;

void SceneSerializer::RegisterActor(const std::wstring& actorType, ActorFactory factory)
{
    _factories[actorType] = factory;
}

// ── Save ──────────────────────────────────────────────────────────────────
bool SceneSerializer::Save(const std::shared_ptr<Scene>& scene, const std::wstring& path)
{
    if (!scene) return false;

    tinyxml2::XMLDocument doc;

    // <?xml ...>
    doc.InsertFirstChild(doc.NewDeclaration());

    // <Scene name="...">
    XMLElement* sceneElem = doc.NewElement("Scene");
    sceneElem->SetAttribute("name", WstrToStr(scene->GetName()).c_str());
    doc.InsertEndChild(sceneElem);

    for (auto& entity : scene->GetEntities())
    {
        if (!entity) continue;

        // Widget, Camera, Light Entity는 씬 저장 대상에서 제외
        // (런타임 시 코드로 다시 생성)
        // Actor 타입이 없는 Entity도 제외
        const std::wstring& entityName = entity->GetEntityName();

        XMLElement* entityElem = doc.NewElement("Entity");
        entityElem->SetAttribute("name", WstrToStr(entityName).c_str());

        // Transform 저장
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

    // 파일로 저장
    std::string pathStr = WstrToStr(path);
    XMLError err = doc.SaveFile(pathStr.c_str());
    if (err != XML_SUCCESS)
    {
        ::OutputDebugStringW((L"[SceneSerializer::Save] 저장 실패: " + path + L"\n").c_str());
        return false;
    }

    ::OutputDebugStringW((L"[SceneSerializer::Save] 저장 완료: " + path + L"\n").c_str());
    return true;
}

// ── Load ──────────────────────────────────────────────────────────────────
bool SceneSerializer::Load(const std::shared_ptr<Scene>& scene, const std::wstring& path)
{
    if (!scene) return false;

    tinyxml2::XMLDocument doc;
    std::string pathStr = WstrToStr(path);
    XMLError err = doc.LoadFile(pathStr.c_str());
    if (err != XML_SUCCESS)
    {
        ::OutputDebugStringW((L"[SceneSerializer::Load] 파일 없음: " + path + L"\n").c_str());
        return false;
    }

    XMLElement* sceneElem = doc.FirstChildElement("Scene");
    if (!sceneElem) return false;

    // 씬 이름 복원
    if (const char* name = sceneElem->Attribute("name"))
        scene->SetName(StrToWstr(name));

    // Entity 복원
    for (XMLElement* entityElem = sceneElem->FirstChildElement("Entity");
         entityElem;
         entityElem = entityElem->NextSiblingElement("Entity"))
    {
        const char* nameAttr     = entityElem->Attribute("name");
        const char* actorAttr    = entityElem->Attribute("actor");

        std::wstring entityName = nameAttr  ? StrToWstr(nameAttr)  : L"Entity";
        std::wstring actorType  = actorAttr ? StrToWstr(actorAttr) : L"";

        std::shared_ptr<Entity> entity;

        // Actor 팩토리로 복원
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
                }
            }
            else
            {
                ::OutputDebugStringW((L"[SceneSerializer::Load] 미등록 Actor 타입: " + actorType + L"\n").c_str());
            }
        }
        else
        {
            // 일반 Entity
            entity = std::make_shared<Entity>(entityName);
            scene->Add(entity);
        }

        if (!entity) continue;

        // Transform 복원
        if (XMLElement* tfElem = entityElem->FirstChildElement("Transform"))
        {
            Vec3 pos, rot, scl;
            tfElem->QueryFloatAttribute("px", &pos.x);
            tfElem->QueryFloatAttribute("py", &pos.y);
            tfElem->QueryFloatAttribute("pz", &pos.z);
            tfElem->QueryFloatAttribute("rx", &rot.x);
            tfElem->QueryFloatAttribute("ry", &rot.y);
            tfElem->QueryFloatAttribute("rz", &rot.z);
            tfElem->QueryFloatAttribute("sx", &scl.x);
            tfElem->QueryFloatAttribute("sy", &scl.y);
            tfElem->QueryFloatAttribute("sz", &scl.z);

            if (auto tf = entity->GetTransform())
            {
                tf->SetLocalPosition(pos);
                tf->SetLocalRotation(rot);
                tf->SetLocalScale(scl);
            }
        }
    }

    ::OutputDebugStringW((L"[SceneSerializer::Load] 로드 완료: " + path + L"\n").c_str());
    return true;
}

// ── 문자열 변환 헬퍼 ──────────────────────────────────────────────────────
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