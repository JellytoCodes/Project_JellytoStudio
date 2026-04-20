#include "Framework.h"
#include "ModelAnimator.h"
#include "ModelAnimation.h"
#include "Model.h"
#include "Core/Managers/InputManager.h"
#include "Core/Managers/TimeManager.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Camera.h"
#include "Pipeline/Shader.h"
#include "Resource/Material.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Entity/Components/Light.h"

ModelAnimator::ModelAnimator(std::shared_ptr<Shader> shader)
	: Super(ComponentType::Animator), _shader(shader)
{
	_tweenDesc.next.animIndex = 0;
	_tweenDesc.tweenSumTime   = 0;
}

ModelAnimator::~ModelAnimator() {}

void ModelAnimator::SetModel(std::shared_ptr<Model> model)
{
	_model = model;

	const auto& materials = _model->GetMaterials();

	for (auto& material : materials)
		material->SetShader(_shader);
}

void ModelAnimator::Update() {}

void ModelAnimator::UpdateTweenData()
{
	TweenDesc& desc = _tweenDesc;

	desc.curr.sumTime += GET_SINGLE(TimeManager)->GetDeltaTime();
	{
		ModelAnimation* currentAnim = _model->GetAnimationByIndex(desc.curr.animIndex);
		if (currentAnim)
		{
			float timePerFrame = 1.f / (currentAnim->frameRate * desc.curr.speed);
			if (desc.curr.sumTime >= timePerFrame)
			{
				desc.curr.sumTime   = 0;
				desc.curr.currFrame = (desc.curr.currFrame + 1) % currentAnim->frameCount;
				desc.curr.nextFrame = (desc.curr.currFrame + 1) % currentAnim->frameCount;
			}
			desc.curr.ratio = desc.curr.sumTime / timePerFrame;
		}
	}

	if (desc.next.animIndex >= 0)
	{
		desc.tweenSumTime += GET_SINGLE(TimeManager)->GetDeltaTime();
		desc.tweenRatio    = desc.tweenSumTime / desc.tweenDuration;

		if (desc.tweenRatio >= 1.f)
		{
			desc.curr = desc.next;
			desc.ClearNextAnim();
		}
		else
		{
			ModelAnimation* nextAnim = _model->GetAnimationByIndex(desc.next.animIndex);
			desc.next.sumTime += GET_SINGLE(TimeManager)->GetDeltaTime();

			float timePerFrame = 1.f / (nextAnim->frameRate * desc.next.speed);

			if (desc.next.ratio >= 1.f)
			{
				desc.next.sumTime   = 0;
				desc.next.currFrame = (desc.next.currFrame + 1) % nextAnim->frameCount;
				desc.next.nextFrame = (desc.next.currFrame + 1) % nextAnim->frameCount;
			}
			desc.next.ratio = desc.next.sumTime / timePerFrame;
		}
	}
}

void ModelAnimator::RenderInstancing(InstancingBuffer* buffer)
{
	if (_model == nullptr) return;
	if (_texture == nullptr) CreateTexture();

	_shader->PushGlobalData(Camera::S_MatView, Camera::S_MatProjection);

	if (Light* lightObj = GET_SINGLE(SceneManager)->GetCurrentScene()->GetLight())
		_shader->PushLightData(lightObj->GetLightDesc());

	if (Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene())
		if (auto* sp = scene->GetShadowPass())
			_shader->PushShadowData(sp->GetShadowDesc(), sp->GetShadowSRV());

	_shader->GetSRV("TransformMap")->SetResource(_srv.Get());

	BoneDesc boneDesc;
	const uint32 boneCount = _model->GetBoneCount();
	for (uint32 i = 0; i < boneCount; i++)
	{
		ModelBone* bone = _model->GetBoneByIndex(i);
		boneDesc.transforms[i] = bone->transform;
	}
	_shader->PushBoneData(boneDesc);

	_shader->PushTransformData(TransformDesc{ GetTransform()->GetWorldMatrix() });

	const auto& meshes = _model->GetMeshes();
	for (auto& mesh : meshes)
	{
		if (mesh->material)
			mesh->material->Update();

		_shader->GetScalar("BoneIndex")->SetInt(mesh->boneIndex);

		mesh->vertexBuffer->PushData(GET_SINGLE(Graphics)->GetDeviceContext());
		mesh->indexBuffer->PushData(GET_SINGLE(Graphics)->GetDeviceContext());
		buffer->PushData();

		_shader->DrawIndexedInstanced(0, _pass, mesh->indexBuffer->GetCount(), buffer->GetCount());
	}
}

InstanceID ModelAnimator::GetInstanceID()
{
	return std::make_pair(reinterpret_cast<uint64>(_model.get()), reinterpret_cast<uint64>(_shader.get()));
}

void ModelAnimator::PressedKeyForCheckFrame()
{
	if (_model == nullptr) return;

	ModelAnimation* currentAnim = _model->GetAnimationByIndex(_tweenDesc.curr.animIndex);
	if (currentAnim == nullptr) return;

	_tweenDesc.curr.speed = 0.00001f;

	if (GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::RIGHT))
	{
		_tweenDesc.curr.currFrame = (_tweenDesc.curr.currFrame + 1) % currentAnim->frameCount;
		_tweenDesc.curr.nextFrame = _tweenDesc.curr.currFrame;
		_tweenDesc.curr.sumTime   = 0.0f;
	}

	if (GET_SINGLE(InputManager)->GetButtonDown(KEY_TYPE::LEFT))
	{
		if (_tweenDesc.curr.currFrame == 0)
			_tweenDesc.curr.currFrame = currentAnim->frameCount - 1;
		else
			_tweenDesc.curr.currFrame--;

		_tweenDesc.curr.nextFrame = _tweenDesc.curr.currFrame;
		_tweenDesc.curr.sumTime   = 0.0f;
	}
}

void ModelAnimator::CreateTexture()
{
	if (_model->GetAnimationCount() == 0) return;

	_animTransforms.resize(_model->GetAnimationCount());
	for (uint32 i = 0; i < _model->GetAnimationCount(); i++)
		CreateAnimationTransform(i);

	{
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width     = MAX_MODEL_TRANSFORMS * 4;
		desc.Height    = MAX_MODEL_KEYFRAMES;
		desc.ArraySize = _model->GetAnimationCount();
		desc.Format    = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.Usage     = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;

		const uint32 dataSize = MAX_MODEL_TRANSFORMS * sizeof(Matrix);
		const uint32 pageSize = dataSize * MAX_MODEL_KEYFRAMES;
		void* mallocPtr = ::malloc(pageSize * _model->GetAnimationCount());

		for (uint32 c = 0; c < _model->GetAnimationCount(); c++)
		{
			BYTE* pageStartPtr = reinterpret_cast<BYTE*>(mallocPtr) + c * pageSize;
			for (uint32 f = 0; f < MAX_MODEL_KEYFRAMES; f++)
			{
				void* ptr = pageStartPtr + dataSize * f;
				::memcpy(ptr, _animTransforms[c].transforms[f].data(), dataSize);
			}
		}

		std::vector<D3D11_SUBRESOURCE_DATA> subResources(_model->GetAnimationCount());
		for (uint32 c = 0; c < _model->GetAnimationCount(); c++)
		{
			void* ptr = (BYTE*)mallocPtr + c * pageSize;
			subResources[c].pSysMem          = ptr;
			subResources[c].SysMemPitch      = dataSize;
			subResources[c].SysMemSlicePitch = pageSize;
		}

		HRESULT hr = GET_SINGLE(Graphics)->GetDevice()->CreateTexture2D(
			&desc, subResources.data(), _texture.GetAddressOf());
		CHECK(hr);
		::free(mallocPtr);
	}

	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
		desc.Format                        = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.ViewDimension                 = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.MipLevels      = 1;
		desc.Texture2DArray.ArraySize      = _model->GetAnimationCount();

		HRESULT hr = GET_SINGLE(Graphics)->GetDevice()->CreateShaderResourceView(
			_texture.Get(), &desc, _srv.GetAddressOf());
		CHECK(hr);
	}
}

void ModelAnimator::CreateAnimationTransform(uint32 index)
{
	std::vector<Matrix> tempAnimBoneTransforms(MAX_MODEL_TRANSFORMS, Matrix::Identity);
	std::vector<Matrix> bindPoseGlobal(MAX_MODEL_TRANSFORMS, Matrix::Identity);

	for (uint32 b = 0; b < _model->GetBoneCount(); b++)
	{
		ModelBone* bone = _model->GetBoneByIndex(b);
		bindPoseGlobal[b] = bone->transform;
	}

	ModelAnimation* animation = _model->GetAnimationByIndex(index);

	for (uint32 f = 0; f < animation->frameCount; f++)
	{
		for (uint32 b = 0; b < _model->GetBoneCount(); b++)
		{
			ModelBone* bone = _model->GetBoneByIndex(b);

			Matrix matAnimation;
			ModelKeyframe* frame = animation->GetKeyframe(bone->name);
			if (frame != nullptr)
			{
				ModelKeyframeData& data = frame->transforms[f];
				matAnimation = Matrix::CreateScale(data.scale)
					* Matrix::CreateFromQuaternion(data.rotation)
					* Matrix::CreateTranslation(data.translation);
			}
			else
			{
				matAnimation = Matrix::Identity;
			}

			int32  parentIndex = bone->parentIndex;
			Matrix matParent   = (parentIndex >= 0) ? tempAnimBoneTransforms[parentIndex] : Matrix::Identity;

			tempAnimBoneTransforms[b] = matAnimation * matParent;

			Matrix invBindPose = bindPoseGlobal[b].Invert();
			_animTransforms[index].transforms[f][b] = invBindPose * tempAnimBoneTransforms[b];
		}
	}
}