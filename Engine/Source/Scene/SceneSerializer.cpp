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
		for (const PlacedBlockRecord& rec : placer->GetPlacedBlocks())
		{
			tinyxml2::XMLElement* blockElem = doc.NewElement("Block");
			blockElem->SetAttribute("x",    rec.x);
			blockElem->SetAttribute("y",    rec.y);
			blockElem->SetAttribute("z",    rec.z);
			blockElem->SetAttribute("type", rec.type);
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

	if (placer)
		placer->ClearAllBlocks();

	int blockCount = 0;
	if (tinyxml2::XMLElement* blocksElem = sceneElem->FirstChildElement("Blocks"))
	{
		for (tinyxml2::XMLElement* blockElem = blocksElem->FirstChildElement("Block");
		     blockElem;
		     blockElem = blockElem->NextSiblingElement("Block"))
		{
			float x = 0.f, y = 0.f, z = 0.f;
			int   type = 0;
			blockElem->QueryFloatAttribute("x",    &x);
			blockElem->QueryFloatAttribute("y",    &y);
			blockElem->QueryFloatAttribute("z",    &z);
			blockElem->QueryIntAttribute  ("type", &type);
			if (placer->PlaceBlock(x, y, z, type))
				blockCount++;
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