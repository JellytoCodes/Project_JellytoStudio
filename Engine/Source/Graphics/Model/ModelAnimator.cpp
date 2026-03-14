
#include "Framework.h"
#include "ModelAnimator.h"
#include "ModelAnimation.h"
#include "Model.h"
#include "Core/Managers/TimeManager.h"
#include "Entity/Components/Transform.h"
#include "Entity/Components/Camera.h"
#include "Pipeline/Shader.h"
#include "Resource/Material.h"
#include "Scene/SceneManager.h"
#include "Entity/Components/Light.h"

ModelAnimator::ModelAnimator(std::shared_ptr<Shader> shader) 
	: Super(ComponentType::Animator), _shader(shader)
{
	_tweenDesc.next.animIndex = 0;
}

ModelAnimator::~ModelAnimator()
{

}

void ModelAnimator::SetModel(std::shared_ptr<Model> model)
{
	_model = model;

	const auto& materials = _model->GetMaterials();
	for (auto& material : materials)
	{
		material->SetShader(_shader);
	}
}

void ModelAnimator::Update()
{

}

void ModelAnimator::UpdateTweenData()
{
	TweenDesc& desc = _tweenDesc;

	desc.curr.sumTime += GET_SINGLE(TimeManager)->GetDeltaTime();
	// 현재 애니메이션
	{
		std::shared_ptr<ModelAnimation> currentAnim = _model->GetAnimationByIndex(desc.curr.animIndex);
		if (currentAnim)
		{
			float timePerFrame = 1 / (currentAnim->frameRate * desc.curr.speed);
			if (desc.curr.sumTime >= timePerFrame)
			{
				desc.curr.sumTime = 0;
				desc.curr.currFrame = (desc.curr.currFrame + 1) % currentAnim->frameCount;
				desc.curr.nextFrame = (desc.curr.currFrame + 1) % currentAnim->frameCount;
			}

			desc.curr.ratio = (desc.curr.sumTime / timePerFrame);
		}
	}

	// 다음 애니메이션이 예약 되어 있다면
	if (desc.next.animIndex >= 0)
	{
		desc.tweenSumTime += GET_SINGLE(TimeManager)->GetDeltaTime();
		desc.tweenRatio = desc.tweenSumTime / desc.tweenDuration;

		if (desc.tweenRatio >= 1.f)
		{
			// 애니메이션 교체 성공
			desc.curr = desc.next;
			desc.ClearNextAnim();
		}
		else
		{
			// 교체중
			std::shared_ptr<ModelAnimation> nextAnim = _model->GetAnimationByIndex(desc.next.animIndex);
			desc.next.sumTime += GET_SINGLE(TimeManager)->GetDeltaTime();

			float timePerFrame = 1.f / (nextAnim->frameRate * desc.next.speed);

			if (desc.next.ratio >= 1.f)
			{
				desc.next.sumTime = 0;

				desc.next.currFrame = (desc.next.currFrame + 1) % nextAnim->frameCount;
				desc.next.nextFrame = (desc.next.currFrame + 1) % nextAnim->frameCount;
			}

			desc.next.ratio = desc.next.sumTime / timePerFrame;
		}
	}
}

void ModelAnimator::RenderInstancing(std::shared_ptr<InstancingBuffer>& buffer)
{
	if (_model == nullptr) return;
	if (_texture == nullptr) CreateTexture();

	// GlobalData
	_shader->PushGlobalData(Camera::S_MatView, Camera::S_MatProjection);

	// Light
	if (std::shared_ptr<Light> lightObj = GET_SINGLE(SceneManager)->GetCurrentScene()->GetLight())
		_shader->PushLightData(lightObj->GetLightDesc());

	// SRV를 통해 정보 전달
	_shader->GetSRV("TransformMap")->SetResource(_srv.Get());

	// Bones
	BoneDesc boneDesc;

	const uint32 boneCount = _model->GetBoneCount();
	for (uint32 i = 0; i < boneCount; i++)
	{
		std::shared_ptr<ModelBone> bone = _model->GetBoneByIndex(i);
		boneDesc.transforms[i] = bone->transform;
	}
	_shader->PushBoneData(boneDesc);

	// Transform
	auto world = GetTransform()->GetWorldMatrix();
	_shader->PushTransformData(TransformDesc{ world });

	const auto& meshes = _model->GetMeshes();
	for (auto& mesh : meshes)
	{
		if (mesh->material)
			mesh->material->Update();

		// BoneIndex
		_shader->GetScalar("BoneIndex")->SetInt(mesh->boneIndex);

		mesh->vertexBuffer->PushData(Graphics::Get()->GetDeviceContext());
		mesh->indexBuffer->PushData(Graphics::Get()->GetDeviceContext());
		buffer->PushData();

		_shader->DrawIndexedInstanced(0, _pass, mesh->indexBuffer->GetCount(), buffer->GetCount());
	}
}

InstanceID ModelAnimator::GetInstanceID()
{
	return std::make_pair((uint64)_model.get(), (uint64)_shader.get());
}

void ModelAnimator::CreateTexture()
{
	if (_model->GetAnimationCount() == 0) return;

	_animTransforms.resize(_model->GetAnimationCount());

	for (uint32 i = 0; i < _model->GetAnimationCount(); i++) 
		CreateAnimationTransform(i);

	// Creature Texture
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
		desc.Width = MAX_MODEL_TRANSFORMS * 4;
		desc.Height = MAX_MODEL_KEYFRAMES;
		desc.ArraySize = _model->GetAnimationCount();
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;

		const uint32 dataSize = MAX_MODEL_TRANSFORMS * sizeof(Matrix);
		const uint32 pageSize = dataSize * MAX_MODEL_KEYFRAMES;
		void* mallocPtr = ::malloc(pageSize * _model->GetAnimationCount());

		// 파편화된 데이터를 조립한다.
		for (uint32 c = 0; c < _model->GetAnimationCount(); c++)
		{
			uint32 startOffset = c * pageSize;

			BYTE* pageStartPtr = reinterpret_cast<BYTE*>(mallocPtr) + startOffset;

			for (uint32 f = 0; f < MAX_MODEL_KEYFRAMES; f++)
			{
				void* ptr = pageStartPtr + dataSize * f;
				::memcpy(ptr, _animTransforms[c].transforms[f].data(), dataSize);
			}
		}
		 
		// 리소스 만들기
		std::vector<D3D11_SUBRESOURCE_DATA> subResources(_model->GetAnimationCount());

		for (uint32 c = 0; c < _model->GetAnimationCount(); c++)
		{
			void* ptr = (BYTE*)mallocPtr + c * pageSize;
			subResources[c].pSysMem = ptr;
			subResources[c].SysMemPitch = dataSize;
			subResources[c].SysMemSlicePitch = pageSize;
		}

		HRESULT hr = Graphics::Get()->GetDevice()->CreateTexture2D(&desc, subResources.data(), _texture.GetAddressOf());
		CHECK(hr);

		::free(mallocPtr);
	}

	// Create SRV
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc;
		ZeroMemory(&desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.MipLevels = 1;
		desc.Texture2DArray.ArraySize = _model->GetAnimationCount();

		HRESULT hr = Graphics::Get()->GetDevice()->CreateShaderResourceView(_texture.Get(), &desc, _srv.GetAddressOf());
		CHECK(hr);
	}
}

void ModelAnimator::CreateAnimationTransform(uint32 index)
{
    std::vector<Matrix> tempAnimBoneTransforms(MAX_MODEL_TRANSFORMS, Matrix::Identity);

    std::vector<Matrix> bindPoseGlobal(MAX_MODEL_TRANSFORMS, Matrix::Identity);
    for (uint32 b = 0; b < _model->GetBoneCount(); b++)
    {
        std::shared_ptr<ModelBone> bone = _model->GetBoneByIndex(b);
        
        // bone->transform은 누적된 global이 아니라 local로 저장돼 있어야 함
        // 현재 Converter가 global로 구워넣고 있으므로, 
        // 부모 global의 역행렬 × 현재 global = local 복원
        int32 parentIndex = bone->parentIndex;
        if (parentIndex < 0)
        {
            bindPoseGlobal[b] = bone->transform;
        }
        else
        {
            // 부모가 이미 계산됐으므로 그걸 그대로 사용
            bindPoseGlobal[b] = bone->transform; // 이미 global
        }
    }

    std::shared_ptr<ModelAnimation> animation = _model->GetAnimationByIndex(index);

    for (uint32 f = 0; f < animation->frameCount; f++)
    {
        for (uint32 b = 0; b < _model->GetBoneCount(); b++)
        {
            std::shared_ptr<ModelBone> bone = _model->GetBoneByIndex(b);

            Matrix matAnimation;
            std::shared_ptr<ModelKeyframe> frame = animation->GetKeyframe(bone->name);
            if (frame != nullptr)
            {
                ModelKeyframeData& data = frame->transforms[f];

                Matrix S = Matrix::CreateScale(data.scale);
                Matrix R = Matrix::CreateFromQuaternion(data.rotation);
                Matrix T = Matrix::CreateTranslation(data.translation);
                matAnimation = S * R * T;
            }
            else
            {
                matAnimation = Matrix::Identity;
            }

            int32 parentIndex = bone->parentIndex;
            Matrix matParent = Matrix::Identity;
            if (parentIndex >= 0)
                matParent = tempAnimBoneTransforms[parentIndex];

            tempAnimBoneTransforms[b] = matAnimation * matParent;

            Matrix invBindPose = bindPoseGlobal[b].Invert();
            _animTransforms[index].transforms[f][b] = invBindPose * tempAnimBoneTransforms[b];
        }
    }
}