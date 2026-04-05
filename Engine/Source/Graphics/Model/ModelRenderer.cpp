#include "Framework.h"
#include "ModelRenderer.h"
#include "Model.h"
#include "Resource/Material.h"
#include "Entity/Components/Transform.h"
#include "Scene/SceneManager.h"
#include "Entity/Components/Camera.h"
#include "Entity/Components/Light.h"
#include "Pipeline/Shader.h"

ModelRenderer::ModelRenderer(const std::shared_ptr<Shader>& shader, const bool bIsSkinned /* = true */)
	: Super(ComponentType::ModelRenderer), _shader(shader), _bIsSkinned(bIsSkinned)
{

}

ModelRenderer::~ModelRenderer()
{

}

void ModelRenderer::Awake()
{

}

void ModelRenderer::Start()
{
	auto device = Graphics::Get()->GetDevice();

	_constantBuffer = std::make_shared<ConstantBuffer<TransformData>>();
	_constantBuffer->Create(device);
}

void ModelRenderer::SetModel(std::shared_ptr<Model> model)
{
	_model = model;

	const auto& materials = _model->GetMaterials();
	for (auto& material : materials)
	{
		material->SetShader(_shader);
	}
}

void ModelRenderer::RenderInstancing(std::shared_ptr<InstancingBuffer>& buffer)
{
	if (_model == nullptr) return;

	// GlobalData
	_shader->PushGlobalData(Camera::S_MatView, Camera::S_MatProjection);

	// Light
	if (std::shared_ptr<Light> lightObj = GET_SINGLE(SceneManager)->GetCurrentScene()->GetLight())
		_shader->PushLightData(lightObj->GetLightDesc());

	const uint32 boneCount = _model->GetBoneCount();

	if (_bIsSkinned)
	{
		BoneDesc boneDesc;
		for (uint32 i = 0; i < boneCount; i++)
		{
			std::shared_ptr<ModelBone> bone = _model->GetBoneByIndex(i);
			boneDesc.transforms[i] = bone->transform;
		}
		_shader->PushBoneData(boneDesc);
	}

	// ── 인스턴스 버퍼 업로드: mesh loop 밖에서 1회만 ─────────────
	// 기존: 서브메시마다 Map→memcpy→Unmap → 서브메시 N개 = N번 GPU stall
	// 수정: 1회 Upload 후 loop 안에서는 IASetVertexBuffers(Bind)만
	buffer->UploadData();

	const auto& meshes = _model->GetMeshes();
	for (auto& mesh : meshes)
	{
		if (mesh->material)
			mesh->material->Update();

		if (_bIsSkinned)
			_shader->GetScalar("BoneIndex")->SetInt(mesh->boneIndex);

		// IA — VB slot0(메시), VB slot1(인스턴스) 바인딩
		mesh->vertexBuffer->PushData(Graphics::Get()->GetDeviceContext());
		mesh->indexBuffer->PushData(Graphics::Get()->GetDeviceContext());
		buffer->BindBuffer(); // Upload는 이미 완료, Bind만

		_shader->DrawIndexedInstanced(0, _pass, mesh->indexBuffer->GetCount(), buffer->GetCount());
	}
}

InstanceID ModelRenderer::GetInstanceID()
{
	return std::make_pair((uint64)_model.get(), (uint64)_shader.get());
}