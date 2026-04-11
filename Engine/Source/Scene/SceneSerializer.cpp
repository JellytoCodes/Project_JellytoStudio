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
#include "Scene/BlockPlacerInterface.h"

std::unordered_map<std::wstring, SceneSerializer::ActorFactory> SceneSerializer::_factories;

void SceneSerializer::RegisterActor(const std::wstring& actorType, ActorFactory factory)
{
	_factories[actorType] = std::move(factory);
}

// ── Save ──────────────────────────────────────────────────────────────────
bool SceneSerializer::Save(Scene* scene, const std::wstring& path, IBlockPlacer* placer)
{
	if (!scene) return false;

	std::filesystem::path fsPath = WstrToStr(path);
	auto dir = fsPath.parent_path();
	if (!dir.empty() && !std::filesystem::exists(dir))
	{
		std::filesystem::create_directories(dir);
		::OutputDebugStringW((L"[SceneSerializer] 폴더 생성: " + StrToWstr(dir.string()) + L"\n").c_str());
	}

	tinyxml2::XMLDocument doc;
	doc.InsertFirstChild(doc.NewDeclaration());

	tinyxml2::XMLElement* sceneElem = doc.NewElement("Scene");
	sceneElem->SetAttribute("name", WstrToStr(scene->GetName()).c_str());
	doc.InsertEndChild(sceneElem);

	for (const auto& entityPtr : scene->GetEntities())
	{
		if (!entityPtr) continue;
		if (dynamic_cast<Widget*>(entityPtr.get())) continue;
		if (entityPtr->GetComponent<Camera>()) continue;

		std::wstring actorType = FindActorType(entityPtr->GetEntityName());

		tinyxml2::XMLElement* entityElem = doc.NewElement("Entity");
		entityElem->SetAttribute("name", WstrToStr(entityPtr->GetEntityName()).c_str());
		if (!actorType.empty())
			entityElem->SetAttribute("actor", WstrToStr(actorType).c_str());

		if (Transform* tf = entityPtr->GetComponent<Transform>())
		{
			Vec3 pos = tf->GetLocalPosition();
			Vec3 rot = tf->GetLocalRotation();
			Vec3 scl = tf->GetLocalScale();

			tinyxml2::XMLElement* tfElem = doc.NewElement("Transform");
			tfElem->SetAttribute("px", pos.x); tfElem->SetAttribute("py", pos.y); tfElem->SetAttribute("pz", pos.z);
			tfElem->SetAttribute("rx", rot.x); tfElem->SetAttribute("ry", rot.y); tfElem->SetAttribute("rz", rot.z);
			tfElem->SetAttribute("sx", scl.x); tfElem->SetAttribute("sy", scl.y); tfElem->SetAttribute("sz", scl.z);
			entityElem->InsertEndChild(tfElem);
		}
		sceneElem->InsertEndChild(entityElem);
	}

	if (placer && !placer->GetPlacedBlocks().empty())
	{
		tinyxml2::XMLElement* blocksElem = doc.NewElement("Blocks");
		for (auto& [col, row] : placer->GetPlacedBlocks())
		{
			tinyxml2::XMLElement* blockElem = doc.NewElement("Block");
			blockElem->SetAttribute("col", col);
			blockElem->SetAttribute("row", row);
			blocksElem->InsertEndChild(blockElem);
		}
		sceneElem->InsertEndChild(blocksElem);
	}

	tinyxml2::XMLError err = doc.SaveFile(WstrToStr(path).c_str());
	if (err != tinyxml2::XML_SUCCESS)
	{
		::OutputDebugStringW((L"[SceneSerializer] 저장 실패: " + path + L"\n").c_str());
		return false;
	}

	wchar_t dbg[128];
	swprintf_s(dbg, L"[SceneSerializer] 저장 완료: %s  (블록 %d개)\n",
		path.c_str(), placer ? (int)placer->GetPlacedBlocks().size() : 0);
	::OutputDebugStringW(dbg);
	return true;
}

// ── Load ──────────────────────────────────────────────────────────────────
bool SceneSerializer::Load(Scene* scene, const std::wstring& path, IBlockPlacer* placer)
{
	if (!scene) return false;

	tinyxml2::XMLDocument doc;
	if (doc.LoadFile(WstrToStr(path).c_str()) != tinyxml2::XML_SUCCESS)
	{
		::OutputDebugStringW((L"[SceneSerializer] 파일 없음: " + path + L"\n").c_str());
		return false;
	}

	tinyxml2::XMLElement* sceneElem = doc.FirstChildElement("Scene");
	if (!sceneElem) return false;

	if (const char* name = sceneElem->Attribute("name"))
		scene->SetName(StrToWstr(name));

	std::vector<Entity*> toRemove;
	for (const auto& entityPtr : scene->GetEntities())
	{
		if (!entityPtr) continue;
		if (dynamic_cast<Widget*>(entityPtr.get())) continue;
		if (entityPtr->GetComponent<Camera>()) continue;
		toRemove.push_back(entityPtr.get());
	}
	for (Entity* e : toRemove)
		scene->Remove(e);

	if (placer)
		placer->ClearAllBlocks();

	// ── Entity 복원 ───────────────────────────────────────────────────────
	for (tinyxml2::XMLElement* entityElem = sceneElem->FirstChildElement("Entity");
	     entityElem;
	     entityElem = entityElem->NextSiblingElement("Entity"))
	{
		const char* nameAttr  = entityElem->Attribute("name");
		const char* actorAttr = entityElem->Attribute("actor");

		std::wstring entityName = nameAttr  ? StrToWstr(nameAttr)  : L"Entity";
		std::wstring actorType  = actorAttr ? StrToWstr(actorAttr) : L"";

		Entity* entity = nullptr;

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
					if (entity)
					{
						if (Light* light = entity->GetComponent<Light>())
							scene->SetMainLight(light);
					}
				}
			}
			else
			{
				::OutputDebugStringW((L"[SceneSerializer] 미등록 Actor: " + actorType + L"\n").c_str());
			}
		}
		else
		{
			auto newEntity = std::make_unique<Entity>(entityName);
			newEntity->AddComponent(std::make_unique<Transform>());
			entity = newEntity.get();
			scene->Add(std::move(newEntity));
		}

		if (!entity) continue;

		if (tinyxml2::XMLElement* tfElem = entityElem->FirstChildElement("Transform"))
		{
			Vec3 pos = {}, rot = {}, scl = { 1, 1, 1 };
			tfElem->QueryFloatAttribute("px", &pos.x); tfElem->QueryFloatAttribute("py", &pos.y); tfElem->QueryFloatAttribute("pz", &pos.z);
			tfElem->QueryFloatAttribute("rx", &rot.x); tfElem->QueryFloatAttribute("ry", &rot.y); tfElem->QueryFloatAttribute("rz", &rot.z);
			tfElem->QueryFloatAttribute("sx", &scl.x); tfElem->QueryFloatAttribute("sy", &scl.y); tfElem->QueryFloatAttribute("sz", &scl.z);
			if (Transform* tf = entity->GetComponent<Transform>())
			{
				tf->SetLocalPosition(pos);
				tf->SetLocalRotation(rot);
				tf->SetLocalScale(scl);
			}
		}
	}

	int blockCount = 0;
	if (placer)
	{
		if (tinyxml2::XMLElement* blocksElem = sceneElem->FirstChildElement("Blocks"))
		{
			for (tinyxml2::XMLElement* blockElem = blocksElem->FirstChildElement("Block");
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

std::string SceneSerializer::WstrToStr(const std::wstring& wStr)
{
	if (wStr.empty()) return {};
	int size = ::WideCharToMultiByte(CP_UTF8, 0, wStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string result(size - 1, 0);
	::WideCharToMultiByte(CP_UTF8, 0, wStr.c_str(), -1, result.data(), size, nullptr, nullptr);
	return result;
}

std::wstring SceneSerializer::StrToWstr(const std::string& str)
{
	if (str.empty()) return {};
	int size = ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	std::wstring result(size - 1, 0);
	::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, result.data(), size);
	return result;
}
