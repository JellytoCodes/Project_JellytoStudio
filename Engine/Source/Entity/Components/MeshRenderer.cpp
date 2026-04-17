#include "Framework.h"
#include "Entity/Components/MeshRenderer.h"
#include "Entity/Components/Camera.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Light.h"
#include "Resource/Material.h"
#include "Resource/Mesh.h"
#include "Graphics/Graphics.h"
#include "Pipeline/Shader.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"

MeshRenderer::MeshRenderer()
    : Super(ComponentType::MeshRenderer)
{
}

MeshRenderer::~MeshRenderer() {}

void MeshRenderer::Awake()      {}
void MeshRenderer::Start()      {}
void MeshRenderer::Update()     {}
void MeshRenderer::LateUpdate() {}
void MeshRenderer::OnDestroy()  {}

bool MeshRenderer::FillPacket(const Matrix& matWorld, RenderPacket& outPacket) const
{
    if (_mesh == nullptr || _material == nullptr)
        return false;

    outPacket.instanceID = GetInstanceID();
    outPacket.pMesh      = _mesh.get();
    outPacket.pMaterial  = _material.get();
    outPacket.pass       = _pass;
    outPacket.matWorld   = matWorld;

    return true;
}

void MeshRenderer::RenderInstancing(InstancingBuffer* buffer)
{
    if (_mesh == nullptr || _material == nullptr) return;

    auto shader = _material->GetShader();
    if (shader == nullptr) return;

    shader->PushGlobalData(Camera::S_MatView, Camera::S_MatProjection);

    if (Light* lightObj = GET_SINGLE(SceneManager)->GetCurrentScene()->GetLight())
        shader->PushLightData(lightObj->GetLightDesc());

    if (Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene())
        if (auto* sp = scene->GetShadowPass())
            shader->PushShadowData(sp->GetShadowDesc(), sp->GetShadowSRV());

    _material->Update();

    auto dc = GET_SINGLE(Graphics)->GetDeviceContext();
    _mesh->GetVertexBuffer()->PushData(dc);
    _mesh->GetIndexBuffer()->PushData(dc);

    buffer->PushData();

    shader->DrawIndexedInstanced(0, _pass,
        _mesh->GetIndexBuffer()->GetCount(),
        buffer->GetCount());
}

InstanceID MeshRenderer::GetInstanceID() const
{
    return std::make_pair(
        reinterpret_cast<uint64>(_mesh.get()),
        reinterpret_cast<uint64>(_material.get()));
}