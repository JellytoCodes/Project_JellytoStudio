#pragma once

#include "Graphics/Graphics.h"
#include "Pipeline/VertexBuffer.h"
#include "Pipeline/IndexBuffer.h"

class ModelAnimation;
class Material;

using geometryData = VertexTextureNormalTangentBlendData;

struct ModelBone
{
	std::wstring name;
	int32        index;
	int32        parentIndex;

	ModelBone* parent = nullptr;

	Matrix transform;
	std::vector<ModelBone*> children;
};

struct ModelMesh
{
	void CreateBuffers()
	{
		auto device = GET_SINGLE(Graphics)->GetDevice();
		vertexBuffer = std::make_unique<VertexBuffer>();
		vertexBuffer->Create(device, geometry->GetVertices());

		indexBuffer = std::make_unique<IndexBuffer>();
		indexBuffer->Create(device, geometry->GetIndices());
	}

	void Render()
	{
		auto deviceContext = GET_SINGLE(Graphics)->GetDeviceContext();
		vertexBuffer->PushData(deviceContext);
		indexBuffer->PushData(deviceContext);
		deviceContext->DrawIndexed(geometry->GetIndexCount(), 0, 0);
	}

	std::wstring name;

	std::unique_ptr<Geometry<geometryData>> geometry = std::make_unique<Geometry<geometryData>>();
	std::unique_ptr<VertexBuffer> vertexBuffer;
	std::unique_ptr<IndexBuffer>  indexBuffer;

	std::wstring materialName;

	std::shared_ptr<Material> material;

	int32       boneIndex;
	ModelBone*  bone = nullptr;
};

class Model
{
public:
	Model();
	~Model();

	void ReadMaterial(const std::wstring& filename);
	void ReadModel(const std::wstring& filename);
	void ReadAnimation(const std::wstring& filename);

	void SetModelPath(const std::wstring& p)   { _modelPath   = p; }
	void SetTexturePath(const std::wstring& p) { _texturePath = p; }

	uint32 GetMaterialCount() const { return static_cast<uint32>(_materials.size()); }

	std::vector<std::shared_ptr<Material>>& GetMaterials() { return _materials; }
	std::shared_ptr<Material> GetMaterialByIndex(uint32 index)             { return _materials[index]; }
	std::shared_ptr<Material> GetMaterialByName(const std::wstring& name);

	uint32 GetMeshCount() const { return static_cast<uint32>(_meshes.size()); }
	std::vector<std::unique_ptr<ModelMesh>>& GetMeshes() { return _meshes; }
	ModelMesh* GetMeshByIndex(uint32 index)              { return _meshes[index].get(); }
	ModelMesh* GetMeshByName(const std::wstring& name);

	uint32 GetBoneCount() const { return static_cast<uint32>(_bones.size()); }
	std::vector<std::unique_ptr<ModelBone>>& GetBones() { return _bones; }
	ModelBone* GetBoneByIndex(uint32 index)
	{
		return (index >= _bones.size()) ? nullptr : _bones[index].get();
	}
	ModelBone* GetBoneByName(const std::wstring& name);

	uint32 GetAnimationCount() const { return static_cast<uint32>(_animations.size()); }
	std::vector<std::unique_ptr<ModelAnimation>>& GetAnimations() { return _animations; }
	ModelAnimation* GetAnimationByIndex(uint32 index)
	{
		return (index >= _animations.size()) ? nullptr : _animations[index].get();
	}
	ModelAnimation* GetAnimationByName(const std::wstring& name);

private:
	void BindCacheInfo();

	std::wstring _modelPath   = L"../Resources/Models/";
	std::wstring _texturePath = L"../Resources/Textures/";

	ModelBone* _root = nullptr;
	std::vector<std::shared_ptr<Material>>       _materials;
	std::vector<std::unique_ptr<ModelBone>>      _bones;
	std::vector<std::unique_ptr<ModelMesh>>      _meshes;
	std::vector<std::unique_ptr<ModelAnimation>> _animations;
};
