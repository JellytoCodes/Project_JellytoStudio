#pragma once

class Scene;
class Entity;
class IBlockPlacer;

// ── SceneSerializer ───────────────────────────────────────────────────────
// 씬을 XML 파일로 저장/로드
//
// 저장 포맷:
//   <Scene name="...">
//     <Entity name="Floor" actor="FloorActor">
//       <Transform px="-10" py="0" pz="-10" .../>
//     </Entity>
//     <Blocks>
//       <Block col="5" row="8"/>
//       <Block col="6" row="8"/>
//     </Blocks>
//   </Scene>
class SceneSerializer
{
public:
    using ActorFactory = std::function<std::shared_ptr<class Actor>()>;

    static void RegisterActor(const std::wstring& actorType, ActorFactory factory);

    // placer가 nullptr이면 블록 섹션 저장 생략
    static bool Save(const std::shared_ptr<Scene>& scene,
        const std::wstring& path,
        IBlockPlacer* placer = nullptr);

    // placer가 nullptr이면 블록 섹션 로드 생략
    static bool Load(const std::shared_ptr<Scene>& scene,
        const std::wstring& path,
        IBlockPlacer* placer = nullptr);

private:
    static std::wstring FindActorType(const std::wstring& entityName);
    static std::string  WstrToStr(const std::wstring& w);
    static std::wstring StrToWstr(const std::string& s);

    static std::unordered_map<std::wstring, ActorFactory> _factories;
};