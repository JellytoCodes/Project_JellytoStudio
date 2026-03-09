#pragma once

#include "Graphics/Graphics.h"
#include "Pipeline/VertexBuffer.h"
#include "Pipeline/IndexBuffer.h"

class ModelAnimation;
class Material;

struct ModelBone
{
	std::wstring							name;
	int32									index;
	int32									parentIndex;
	std::shared_ptr<ModelBone>				parent;

	Matrix									transform;
	std::vector<std::shared_ptr<ModelBone>> children;
};

struct ModelMesh
{
	void CreateBuffers()
	{
		vertexBuffer = std::make_shared<VertexBuffer>();
		vertexBuffer->Create(Graphics::Get()->GetDevice(), geometry->GetVertices());

		indexBuffer = std::make_shared<IndexBuffer>();
		indexBuffer->Create(Graphics::Get()->GetDevice(), geometry->GetIndices());
	}

	void Render()
	{
		vertexBuffer->PushData(Graphics::Get()->GetDeviceContext());
		indexBuffer->PushData(Graphics::Get()->GetDeviceContext());

		Graphics::Get()->GetDeviceContext()->DrawIndexed(geometry->GetIndexCount(), 0, 0);
	}


	std::wstring				name;

	// Mesh
	std::shared_ptr<Geometry<VertexTextureNormalTangentBlendData>>		geometry = std::make_shared<Geometry<VertexTextureNormalTangentBlendData>>();
	std::shared_ptr<VertexBuffer>					vertexBuffer;
	std::shared_ptr<IndexBuffer>					indexBuffer;

	// Material
	std::wstring									materialName = L"";
	std::shared_ptr<Material>						material;			

	// Bones
	int32											boneIndex;
	std::shared_ptr<ModelBone>						bone;				
};

class Model
{
public:
	Model();
	~Model();

	void												ReadMaterial(const std::wstring& filename);
	void												ReadModel(const std::wstring& filename);
	void												ReadAnimation(const std::wstring& filename);

	uint32												GetMaterialCount() const						{ return static_cast<uint32>(_materials.size()); }
	std::vector<std::shared_ptr<Material>>&				GetMaterials()									{ return _materials; }
	std::shared_ptr<Material>							GetMaterialByIndex(uint32 index)				{ return _materials[index]; }
	std::shared_ptr<Material>							GetMaterialByName(const std::wstring& name);

	uint32												GetMeshCount() const							{ return static_cast<uint32>(_meshes.size()); }
	std::vector<std::shared_ptr<ModelMesh>>&			GetMeshes()										{ return _meshes; }
	std::shared_ptr<ModelMesh>							GetMeshByIndex(uint32 index)					{ return _meshes[index]; }
	std::shared_ptr<ModelMesh>							GetMeshByName(const std::wstring& name);

	uint32												GetBoneCount() const							{ return static_cast<uint32>(_bones.size()); }
	std::vector<std::shared_ptr<ModelBone>>&			GetBones()										{ return _bones; }
	std::shared_ptr<ModelBone>							GetBoneByIndex(uint32 index)					{ return (index < 0 || index >= _bones.size() ? nullptr : _bones[index]); }
	std::shared_ptr<ModelBone>							GetBoneByName(const std::wstring& name);

	uint32												GetAnimationCount() const						{ return _animations.size(); }
	std::vector<std::shared_ptr<ModelAnimation>>&		GetAnimations()									{ return _animations; }
	std::shared_ptr<ModelAnimation>						GetAnimationByIndex(UINT index)					{ return (index < 0 || index >= _animations.size()) ? nullptr : _animations[index]; }
	std::shared_ptr<ModelAnimation>						GetAnimationByName(const std::wstring& name);

private:
	void BindCacheInfo();

	std::wstring									_modelPath = L"../Resources/Models/";
	std::wstring									_texturePath = L"../Resources/Textures/";

	std::shared_ptr<ModelBone>						_root;
	std::vector<std::shared_ptr<Material>>			_materials;
	std::vector<std::shared_ptr<ModelBone>>			_bones;
	std::vector<std::shared_ptr<ModelMesh>>			_meshes;
	std::vector<std::shared_ptr<ModelAnimation>>	_animations;
};

