#pragma once

class Scene;
class Entity;

// ── SceneSerializer ───────────────────────────────────────────────────────
// 씬을 XML 파일로 저장/로드
//
// 저장 포맷:
//   <Scene name="...">
//     <Entity name="Floor" actor="FloorActor">
//       <Transform px="-10" py="0" pz="-10" rx="0" ry="0" rz="0" sx="1" sy="1" sz="1"/>
//     </Entity>
//     ...
//   </Scene>
//
// - Save: 폴더 없으면 자동 생성, actor 속성 포함 저장
// - Load: 기존 씬 오브젝트 정리 후 Actor 팩토리로 재생성 + Transform 복원
class SceneSerializer
{
public:
    using ActorFactory = std::function<std::shared_ptr<class Actor>()>;

    // Actor 팩토리 등록 (EditorApp::Init에서 호출)
    // actorType: "SkySphereActor", "FloorActor" 등 고유 식별자
    static void RegisterActor(const std::wstring& actorType, ActorFactory factory);

    // 씬 저장 (폴더 없으면 자동 생성)
    static bool Save(const std::shared_ptr<Scene>& scene, const std::wstring& path);

    // 씬 로드 (기존 오브젝트 정리 후 재생성)
    static bool Load(const std::shared_ptr<Scene>& scene, const std::wstring& path);

private:
    // Entity 이름 → actorType 역매핑
    static std::wstring FindActorType(const std::wstring& entityName);

    static std::string  WstrToStr(const std::wstring& w);
    static std::wstring StrToWstr(const std::string& s);

    static std::unordered_map<std::wstring, ActorFactory> _factories;
};