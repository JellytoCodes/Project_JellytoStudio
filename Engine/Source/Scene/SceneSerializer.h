#pragma once

class Scene;
class Entity;
class IBlockPlacer;

class SceneSerializer
{
public:
    using ActorFactory = std::function<std::unique_ptr<class Actor>()>;

    static void RegisterActor    (const std::wstring& actorType,  ActorFactory factory);
    static void RegisterActorName(const std::wstring& entityName, const std::wstring& actorType);

    static bool Save(Scene* scene, const std::wstring& path, IBlockPlacer* placer = nullptr);
    static bool Load(Scene* scene, const std::wstring& path, IBlockPlacer* placer = nullptr);

private:
    static std::wstring FindActorType(const std::wstring& entityName);
    static std::string  WstrToStr(const std::wstring& wStr);
    static std::wstring StrToWstr(const std::string&  str);

    static std::unordered_map<std::wstring, ActorFactory> _factories;
    static std::unordered_map<std::wstring, std::wstring> _entityNameMap;
};