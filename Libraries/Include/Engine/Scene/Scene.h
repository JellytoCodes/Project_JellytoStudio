#pragma once
#include "Entity/Components/Collider/CollisionChannel.h"
#include "Graphics/ShadowPass.h"

class Light;
class Camera;
class Entity;
class Widget;

class Scene
{
public:
    Scene();
    virtual ~Scene();

    void SetName(const std::wstring& name) { _name = name; }
    const std::wstring& GetName() const { return _name; }

    virtual void Awake();
    virtual void Start();
    virtual void Update();
    virtual void LateUpdate();
    virtual void OnDestroy();

    virtual void Render();
    virtual void Add(std::unique_ptr<Entity> object);
    virtual void Remove(Entity* object);

    void    SetMainCamera(Camera* mainCamera)                                   { _mainCamera = mainCamera; }
    Camera* GetMainCamera()                                                     { return _mainCamera; }

    Light* GetLight()                                                           { return _mainLight; }
    void SetMainLight(Light* light)                                             { _mainLight = light; }

    const std::unordered_set<std::unique_ptr<Entity>>& GetEntities() const      { return _objects; }
    std::unordered_set<std::unique_ptr<Entity>>& GetEntities()                  { return _objects; }

    const std::unordered_set<Entity*>& GetCollidableEntities() const            { return _collidableObjects; }
    std::unordered_set<Entity*>& GetCollidableEntities()                        { return _collidableObjects; }

    ShadowPass* GetShadowPass() const                                           { return _shadowPass.get(); }

    Entity* Pick(int32 screenX, int32 screenY);

    bool PickBlock(int32 screenX, int32 screenY, CollisionChannel queryChan, Entity*& outEntity, Vec3& outHitNormal, float& outDist);
    bool PickGroundPoint(int32 screenX, int32 screenY, Vec3& outWorldPos, float groundY = 0.f);

private:
    bool BuildPickRay(int32 screenX, int32 screenY, Vec3& outOrigin, Vec3& outDir) const;

    std::wstring _name = L"Unnamed Scene";

    std::unordered_set<std::unique_ptr<Entity>> _objects;

    Camera* _mainCamera = nullptr;
    Light* _mainLight = nullptr;

    std::unordered_set<Entity*> _collidableObjects;
    std::vector<Widget*> _widgetObjects;
    std::unique_ptr<ShadowPass> _shadowPass;
};