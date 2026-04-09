#pragma once

class Scene;
class Entity;
class IBlockPlacer;

class SceneSerializer
{
public:
	using ActorFactory = std::function<std::unique_ptr<class Actor>()>;

	static void RegisterActor(const std::wstring& actorType, ActorFactory factory);
	static bool Save(Scene* scene, const std::wstring& path, IBlockPlacer* placer = nullptr);
	static bool Load(Scene* scene, const std::wstring& path, IBlockPlacer* placer = nullptr);

private:
	static std::wstring FindActorType(const std::wstring& entityName);
	static std::string  WstrToStr(const std::wstring& w);
	static std::wstring StrToWstr(const std::string& s);

	static std::unordered_map<std::wstring, ActorFactory> _factories;
};
