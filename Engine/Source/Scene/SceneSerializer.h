#pragma once

class Scene;
class Entity;

// ── SceneSerializer ───────────────────────────────────────────────────────
// 씬을 XML 파일로 저장/로드
//
// 저장 포맷:
//   <Scene name="...">
//     <Entity name="Floor" actor="FloorActor">
//       <Transform px="0" py="0" pz="0" rx="0" ry="0" rz="0" sx="1" sy="1" sz="1"/>
//     </Entity>
//     ...
//   </Scene>
//
// 로드 시 ActorFactory를 통해 Actor를 재생성하고 Transform을 복원
class SceneSerializer
{
public:
    using ActorFactory = std::function<std::shared_ptr<class Actor>()>;

    // Actor 팩토리 등록 (EditorApp::Init에서 등록)
    static void RegisterActor(const std::wstring& actorType, ActorFactory factory);

    // 씬 저장 (path: 예) L"../Scenes/main.xml")
    static bool Save(const std::shared_ptr<Scene>& scene, const std::wstring& path);

    // 씬 로드 → 씬에 Entity 추가하고 mainLight/Camera는 caller가 처리
    static bool Load(const std::shared_ptr<Scene>& scene, const std::wstring& path);

private:
    static std::string  WstrToStr(const std::wstring& w);
    static std::wstring StrToWstr(const std::string& s);

    static std::unordered_map<std::wstring, ActorFactory> _factories;
};