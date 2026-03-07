#pragma once

enum class ResourceType : int8
{
	None = -1,
	Mesh,
	Shader,
	Texture,
	Material,
	Animation,

	End
};

enum { RESOURCE_TYPE_COUNT = static_cast<uint8>(ResourceType::End) };

class Resource : public enable_shared_from_this<Resource>
{
public:
	Resource(ResourceType type);
	virtual ~Resource();

	ResourceType GetType() { return _type; }

	void SetName(const wstring& name) { _name = name; }
	const wstring& GetName() { return _name; }
	uint32 GetID() { return _id; }

protected:
	virtual void Load(const wstring& path) { }
	virtual void Save(const wstring& path) { }

	ResourceType _type = ResourceType::None;
	wstring _name;
	wstring _path;
	uint32 _id = 0;
};

