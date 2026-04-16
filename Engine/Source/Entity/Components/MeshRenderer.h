#pragma once

#include "Component.h"
#include "Pipeline/InstancingBuffer.h"
#include "Graphics/RenderPacket.h"   // [FIX B] 순수 데이터 패킷 헤더

class Shader;
class Material;
class Mesh;

class MeshRenderer : public Component
{
    using Super = Component;

public:
    MeshRenderer();
    virtual ~MeshRenderer();

    virtual void Awake()      override;
    virtual void Start()      override;
    virtual void Update()     override;
    virtual void LateUpdate() override;
    virtual void OnDestroy()  override;

    bool FillPacket(const Matrix& matWorld, RenderPacket& outPacket) const;

    void RenderInstancing(InstancingBuffer* buffer);

    InstanceID GetInstanceID() const;

    void SetMesh(const std::shared_ptr<Mesh>& mesh)         { _mesh     = mesh;     }
    void SetMaterial(const std::shared_ptr<Material>& mat)  { _material = mat;      }
    void SetPass(uint8 pass)                                 { _pass     = pass;     }

    const std::shared_ptr<Mesh>&     GetMesh()     const { return _mesh;     }
    const std::shared_ptr<Material>& GetMaterial() const { return _material; }
    uint8                            GetPass()     const { return _pass;     }

private:
    std::shared_ptr<Mesh>       _mesh;
    std::shared_ptr<Shader>     _shader;
    std::shared_ptr<Material>   _material;

    uint8                       _pass = 0;
};
