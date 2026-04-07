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
{ }

ModelRenderer::~ModelRenderer() { }

void ModelRenderer::Awake() { }

void ModelRenderer::Start()
{
	auto device = Graphics::Get()->GetDevice();

	_constantBuffer = std::make_shared<ConstantBuffer<TransformData>>();
	_constantBuffer->Create(device);
}

void ModelRenderer::SetModel(std::shared_ptr<Model> model)
{
	_model = model;

	// ── Material 공유 안전 처리 ──────────────────────────────
	// model은 여러 ModelRenderer가 공유 → model._materials도 공유
	// 여기서 material->SetShader()를 호출하면 마지막 호출이 덮어씀
	// → 렌더 시점(RenderInstancing)에 shader를 Push하므로 여기선 스킵
	// 단, 처음 로드 시 shader가 null이면 최초 1회만 설정
	const auto& materials = _model->GetMaterials();
	for (auto& material : materials)
	{
		if (material->GetShader() == nullptr)
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

	const auto& meshes = _model->GetMeshes();
	for (auto& mesh : meshes)
	{
		if (mesh->material)
		{
			// 공유 material에 이 렌더러의 shader를 바인딩 후 Update
			// (SetModel 시점에 덮어쓰지 않으므로 여기서 주입)
			mesh->material->SetShader(_shader);
			mesh->material->Update();
		}

		if (_bIsSkinned)
			_shader->GetScalar("BoneIndex")->SetInt(mesh->boneIndex);

		// IA — slot0: 메시 버텍스/인덱스, slot1: 인스턴스 버퍼
		mesh->vertexBuffer->PushData(Graphics::Get()->GetDeviceContext());
		mesh->indexBuffer->PushData(Graphics::Get()->GetDeviceContext());
		buffer->BindBuffer(); // GPU에 이미 올라간 버퍼를 IA slot1에 바인딩

		_shader->DrawIndexedInstanced(0, _pass, mesh->indexBuffer->GetCount(), buffer->GetCount());
	}
}

InstanceID ModelRenderer::GetInstanceID()
{
	return std::make_pair((uint64)_model.get(), (uint64)_shader.get());
}