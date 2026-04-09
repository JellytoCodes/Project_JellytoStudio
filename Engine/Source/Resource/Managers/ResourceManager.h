#pragma once

#include "Resource/Resource.h"
#include <memory>
#include <map>
#include <array>
#include <string>

class Shader;
class Texture;
class Mesh;
class Material;

class ResourceManager
{
	DECLARE_SINGLE(ResourceManager);

public:
	void Init();

	// ?? 반환형과 매개변수를 모두 shared_ptr로 변경
	template<typename T>
	std::shared_ptr<T> Load(const std::wstring& key, const std::wstring& path);

	template<typename T>
	bool Add(const std::wstring& key, std::shared_ptr<T> object);

	template<typename T>
	std::shared_ptr<T> Get(const std::wstring& key);

	std::shared_ptr<Texture> GetOrAddTexture(const std::wstring& key, const std::wstring& path);

	template<typename T>
	ResourceType GetResourceType() const;

private:
	void CreateDefaultMesh();

	std::wstring _resourcePath;

	// ?? 핵심: 리소스 매니저는 자원을 '공유 소유' 합니다.
	using KeyObjMap = std::map<std::wstring, std::shared_ptr<Resource>>;
	std::array<KeyObjMap, RESOURCE_TYPE_COUNT> _resources;
};

template<typename T>
std::shared_ptr<T> ResourceManager::Load(const std::wstring& key, const std::wstring& path)
{
	auto objectType = GetResourceType<T>();
	KeyObjMap& keyObjMap = _resources[static_cast<uint8>(objectType)];

	auto findIt = keyObjMap.find(key);
	if (findIt != keyObjMap.end())
		return std::static_pointer_cast<T>(findIt->second); // shared_ptr 다운캐스팅

	auto object = std::make_shared<T>();
	object->Load(path);
	keyObjMap[key] = object; // 캐시에 등록 (참조 카운트 +1)

	return object;
}

template<typename T>
bool ResourceManager::Add(const std::wstring& key, std::shared_ptr<T> object)
{
	ResourceType resourceType = GetResourceType<T>();
	KeyObjMap& keyObjMap = _resources[static_cast<uint8>(resourceType)];

	auto findIt = keyObjMap.find(key);
	if (findIt != keyObjMap.end())
		return false;

	keyObjMap[key] = object;
	return true;
}

template<typename T>
std::shared_ptr<T> ResourceManager::Get(const std::wstring& key)
{
	ResourceType resourceType = GetResourceType<T>();
	KeyObjMap& keyObjMap = _resources[static_cast<uint8>(resourceType)];

	auto findIt = keyObjMap.find(key);
	if (findIt != keyObjMap.end())
		return std::static_pointer_cast<T>(findIt->second); // shared_ptr 다운캐스팅

	return nullptr;
}

template<typename T>
ResourceType ResourceManager::GetResourceType() const
{
	if (std::is_same_v<T, Texture>)  return ResourceType::Texture;
	if (std::is_same_v<T, Mesh>)     return ResourceType::Mesh;
	if (std::is_same_v<T, Material>) return ResourceType::Material;

	assert(false);
	return ResourceType::None;
}