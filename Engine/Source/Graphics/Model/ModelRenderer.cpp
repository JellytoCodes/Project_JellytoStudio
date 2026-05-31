#include "Framework.h"
#include "ModelRenderer.h"
#include "Model.h"
#include "Resource/Material.h"
#include "Entity/Components/Transform.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Entity/Components/Camera.h"
#include "Entity/Components/Light.h"
#include "Pipeline/Shader.h"

ModelRenderer::ModelRenderer(std::shared_ptr<Shader> shader, bool bIsSkinned)
	: Super(ComponentType::ModelRenderer), _shader(shader), _bIsSkinned(bIsSkinned)
{
}

ModelRenderer::~ModelRenderer() {}

void ModelRenderer::Awake() {}

void ModelRenderer::Start()
{
	auto device = GET_SINGLE(Graphics)->GetDevice();

	_constantBuffer = std::make_unique<ConstantBuffer<TransformData>>();
	_constantBuffer->Create(device);
}

void ModelRenderer::SetModel(std::shared_ptr<Model> model)
{
    _model = model;
    if (_model == nullptr) return;

    const auto& materials = _model->GetMaterials();

	for (auto& material : materials)
		material->SetShader(_shader);
}

void ModelRenderer::RenderInstancing(InstancingBuffer* buffer)
{
    if (_model == nullptr) return;
    if (_shader == nullptr || buffer == nullptr || buffer->GetCount() == 0) return;

    _shader->PushGlobalData(Camera::S_MatView, Camera::S_MatProjection);

    if (Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene())
    {
        if (Light* lightObj = scene->GetLight())
            _shader->PushLightData(lightObj->GetLightDesc());

        if (auto* sp = scene->GetShadowPass())
            _shader->PushShadowData(sp->GetShadowDesc(), sp->GetShadowSRV());
    }

	const uint32 boneCount = _model->GetBoneCount();

	if (_bIsSkinned)
	{
		BoneDesc boneDesc;
		for (uint32 i = 0; i < boneCount; i++)
		{
			ModelBone* bone = _model->GetBoneByIndex(i);
			boneDesc.transforms[i] = bone->transform;
		}
		_shader->PushBoneData(boneDesc);
	}

	const auto& meshes = _model->GetMeshes();
	for (auto& mesh : meshes)
	{
		if (mesh->material)
		{
			mesh->material->SetShader(_shader);
			mesh->material->Update();
		}

		if (_bIsSkinned)
			_shader->GetScalar("BoneIndex")->SetInt(mesh->boneIndex);

		mesh->vertexBuffer->PushData(GET_SINGLE(Graphics)->GetDeviceContext());
		mesh->indexBuffer->PushData(GET_SINGLE(Graphics)->GetDeviceContext());
		buffer->BindBuffer();

		_shader->DrawIndexedInstanced(0, _pass, mesh->indexBuffer->GetCount(), buffer->GetCount());
	}
}

void ModelRenderer::RenderRawInstanced(const ComPtr<ID3D11DeviceContext>& dc,
                                        InstancingBuffer* buffer)
{
    if (!_model || !buffer || buffer->GetCount() == 0) return;

    const auto& meshes = _model->GetMeshes();
    for (auto& mesh : meshes)
    {
        mesh->vertexBuffer->PushData(dc);
        mesh->indexBuffer->PushData(dc);

        buffer->BindBuffer();

        dc->DrawIndexedInstanced(mesh->indexBuffer->GetCount(), buffer->GetCount(), 0, 0, 0);
    }
}

InstanceID ModelRenderer::GetInstanceID()
{
    if (_model == nullptr || _shader == nullptr) return {};

	return { reinterpret_cast<uint64>(_model.get()), reinterpret_cast<uint64>(_shader.get()) };
}
