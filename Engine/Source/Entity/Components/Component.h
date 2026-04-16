#pragma once

class Transform;

enum class ComponentType : int8
{
    Transform,
    Light,
    Camera,
    MeshRenderer,
    Collider,
    ModelRenderer,
    Animator,
    AnimStateMachine,
    TileMap,
    UIComponent,

    ///////////////
    Script,       // Scripts (MonoBehaviour) are stored separately in _scripts
    ///////////////

    End
};

enum { FIXED_COMPONENT_COUNT = static_cast<uint8>(ComponentType::End) - 1 };

template <typename T>
struct ComponentTypeOf;

class Light;
class Camera;
class MeshRenderer;
class BaseCollider;
class AABBCollider;
class OBBCollider;
class SphereCollider;
class FrustumCollider;
class ModelRenderer;
class ModelAnimator;
class AnimStateMachine;

template<> struct ComponentTypeOf<Transform>        { static constexpr ComponentType kType = ComponentType::Transform;      };
template<> struct ComponentTypeOf<Light>            { static constexpr ComponentType kType = ComponentType::Light;          };
template<> struct ComponentTypeOf<Camera>           { static constexpr ComponentType kType = ComponentType::Camera;         };
template<> struct ComponentTypeOf<MeshRenderer>     { static constexpr ComponentType kType = ComponentType::MeshRenderer;   };

template<> struct ComponentTypeOf<BaseCollider>     { static constexpr ComponentType kType = ComponentType::Collider;       };
template<> struct ComponentTypeOf<AABBCollider>     { static constexpr ComponentType kType = ComponentType::Collider;       };
template<> struct ComponentTypeOf<OBBCollider>      { static constexpr ComponentType kType = ComponentType::Collider;       };
template<> struct ComponentTypeOf<SphereCollider>   { static constexpr ComponentType kType = ComponentType::Collider;       };
template<> struct ComponentTypeOf<FrustumCollider>  { static constexpr ComponentType kType = ComponentType::Collider;       };

template<> struct ComponentTypeOf<ModelRenderer>    { static constexpr ComponentType kType = ComponentType::ModelRenderer;  };
template<> struct ComponentTypeOf<ModelAnimator>    { static constexpr ComponentType kType = ComponentType::Animator;       };
template<> struct ComponentTypeOf<AnimStateMachine> { static constexpr ComponentType kType = ComponentType::AnimStateMachine;};

class Component
{
    friend class Entity;
    void SetEntity(Entity* entity);

public:
    Component(ComponentType type);
    virtual ~Component();

    virtual void Awake();
    virtual void Start();
    virtual void Update();
    virtual void LateUpdate();
    virtual void OnDestroy();

    ComponentType   GetType()      const { return _type; }

    Entity*         GetEntity();
    Transform*      GetTransform();

protected:
    ComponentType   _type;

    Entity*         _entity;
    Transform*      _transform;
};
