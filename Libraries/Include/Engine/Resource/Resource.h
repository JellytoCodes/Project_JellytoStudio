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

// ── 이전: enable_shared_from_this<Resource> 상속 ─────────────────────────
class Resource
{
public:
	Resource(ResourceType type) : _type(type) {}
	virtual ~Resource() = default;

	ResourceType         GetType()  { return _type; }
	void                 SetName(const std::wstring& name) { _name = name; }
	const std::wstring&  GetName()  { return _name; }
	uint32               GetID()    { return _id; }

protected:
	virtual void Load(const std::wstring& path) {}
	virtual void Save(const std::wstring& path) {}

	ResourceType _type = ResourceType::None;
	std::wstring _name;
	std::wstring _path;
	uint32       _id = 0;
};
