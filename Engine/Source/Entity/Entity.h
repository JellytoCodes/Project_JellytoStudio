#pragma once
#include "Entity/Components/Transform.h"

class MonoBehaviour;
class Widget;  // [FIX B] Widget 타입 직접 추적을 위한 전방 선언

class Entity
{
public:
    Entity(const std::wstring& name);
    virtual ~Entity();

    virtual void Awake();
    virtual void Start();
    virtual void Update();
    virtual void LateUpdate();
    virtual void OnDestroy();

    void OnCollision(Entity* other);

    void AddComponent(std::unique_ptr<Component> component);

    template <typename T>
    T* GetComponent();

    void SetLayerIndex(const uint8 layer) { _layerIndex = layer; }
    uint8 GetLayerIndex() const           { return _layerIndex;  }

    std::wstring GetEntityName() const { return _entityName; }

protected:
    std::array<std::unique_ptr<Component>, FIXED_COMPONENT_COUNT> _components;
    std::vector<std::unique_ptr<MonoBehaviour>> _scripts;

    uint8        _layerIndex = 0;
    std::wstring _entityName = L"";
};

template <typename T>
T* Entity::GetComponent()
{
    constexpr int8 kIndex = static_cast<int8>(ComponentTypeOf<T>::kType);

    static_assert(kIndex >= 0, "ComponentType 인덱스가 음수입니다. ComponentTypeOf 특수화를 확인하세요.");
    static_assert(kIndex < static_cast<int8>(FIXED_COMPONENT_COUNT), "Script 타입은 GetComponent가 아닌 GetScript<T>()를 사용하세요.");

    return static_cast<T*>(_components[kIndex].get());
}
