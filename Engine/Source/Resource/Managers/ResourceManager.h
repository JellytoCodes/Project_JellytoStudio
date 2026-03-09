#pragma once

#include "Resource/Resource.h"

class Shader;
class Texture;
class Mesh;
class Material;

class ResourceManager
{
	DECLARE_SINGLE(ResourceManager);

public:
	void Init();

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

	using KeyObjMap = std::map<std::wstring/*key*/, std::shared_ptr<Resource>>;
	std::array<KeyObjMap, RESOURCE_TYPE_COUNT> _resources;
};

template<typename T>
std::shared_ptr<T> ResourceManager::Load(const std::wstring& key, const std::wstring& path)
{
	auto objectType = GetResourceType<T>();
	KeyObjMap& keyObjMap = _resources[static_cast<uint8>(objectType)];

	auto findIt = keyObjMap.find(key);
	if (findIt != keyObjMap.end())
		return static_pointer_cast<T>(findIt->second);

	std::shared_ptr<T> object = std::make_shared<T>();
	object->Load(path);
	keyObjMap[key] = object;

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
		return static_pointer_cast<T>(findIt->second);

	return nullptr;
}

template<typename T>
ResourceType ResourceManager::GetResourceType() const
{
	if (std::is_same_v<T, Texture>)		return ResourceType::Texture;
	if (std::is_same_v<T, Mesh>)		return ResourceType::Mesh;
	if (std::is_same_v<T, Material>)	return ResourceType::Material;

	assert(false);
	return ResourceType::None;
}