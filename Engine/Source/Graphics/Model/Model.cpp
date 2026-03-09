#include "Framework.h"
#include "Model.h"
#include "Utils/Utils.h"
#include "Utils/FileUtils.h"
#include "Utils/tinyxml2.h"
#include <filesystem>
#include "Resource/Material.h"
#include "Resource/Managers/ResourceManager.h"
#include "Graphics/Model/ModelAnimation.h"

Model::Model()
{

}

Model::~Model()
{

}

void Model::ReadMaterial(const std::wstring& filename)
{
	std::wstring fullPath = _texturePath + filename + L".xml";
	auto parentPath = std::filesystem::path(fullPath).parent_path();

	tinyxml2::XMLDocument* document = new tinyxml2::XMLDocument();
	tinyxml2::XMLError error = document->LoadFile(Utils::ToString(fullPath).c_str());
	assert(error == tinyxml2::XML_SUCCESS);

	tinyxml2::XMLElement* root = document->FirstChildElement();
	tinyxml2::XMLElement* materialNode = root->FirstChildElement();

	while (materialNode)
	{
		std::shared_ptr<Material> material = std::make_shared<Material>();

		tinyxml2::XMLElement* node = nullptr;

		node = materialNode->FirstChildElement();
		material->SetName(Utils::ToWString(node->GetText()));

		// Diffuse Texture
		node = node->NextSiblingElement();
		if (node->GetText())
		{
			std::wstring textureStr = Utils::ToWString(node->GetText());
			if (textureStr.length() > 0)
			{
				auto texture = GET_SINGLE(ResourceManager)->GetOrAddTexture(textureStr, (parentPath / textureStr).wstring());
				material->SetDiffuseMap(texture);
			}
		}

		// Specular Texture
		node = node->NextSiblingElement();
		if (node->GetText())
		{
			std::wstring texture = Utils::ToWString(node->GetText());
			if (texture.length() > 0)
			{
				std::wstring textureStr = Utils::ToWString(node->GetText());
				if (textureStr.length() > 0)
				{
					auto texture = GET_SINGLE(ResourceManager)->GetOrAddTexture(textureStr, (parentPath / textureStr).wstring());
					material->SetSpecularMap(texture);
				}
			}
		}

		// Normal Texture
		node = node->NextSiblingElement();
		if (node->GetText())
		{
			std::wstring textureStr = Utils::ToWString(node->GetText());
			if (textureStr.length() > 0)
			{
				auto texture = GET_SINGLE(ResourceManager)->GetOrAddTexture(textureStr, (parentPath / textureStr).wstring());
				material->SetNormalMap(texture);
			}
		}

		// Ambient
		{
			node = node->NextSiblingElement();

			Color color;
			color.x = node->FloatAttribute("R");
			color.y = node->FloatAttribute("G");
			color.z = node->FloatAttribute("B");
			color.w = node->FloatAttribute("A");
			material->GetMaterialDesc().ambient = color;
		}

		// Diffuse
		{
			node = node->NextSiblingElement();

			Color color;
			color.x = node->FloatAttribute("R");
			color.y = node->FloatAttribute("G");
			color.z = node->FloatAttribute("B");
			color.w = node->FloatAttribute("A");
			material->GetMaterialDesc().diffuse = color;
		}

		// Specular
		{
			node = node->NextSiblingElement();

			Color color;
			color.x = node->FloatAttribute("R");
			color.y = node->FloatAttribute("G");
			color.z = node->FloatAttribute("B");
			color.w = node->FloatAttribute("A");
			material->GetMaterialDesc().specular = color;
		}

		// Emissive
		{
			node = node->NextSiblingElement();

			Color color;
			color.x = node->FloatAttribute("R");
			color.y = node->FloatAttribute("G");
			color.z = node->FloatAttribute("B");
			color.w = node->FloatAttribute("A");
			material->GetMaterialDesc().emissive = color;
		}

		_materials.push_back(material);

		// Next Material
		materialNode = materialNode->NextSiblingElement();
	}

	BindCacheInfo();
}

void Model::ReadModel(const std::wstring& filename)
{
	std::wstring fullPath = _modelPath + filename + L".mesh";

	std::shared_ptr<FileUtils> file = std::make_shared<FileUtils>();
	file->Open(fullPath, FileMode::Read);

	// Bones
	{
		const uint32 count = file->Read<uint32>();

		for (uint32 i = 0; i < count; i++)
		{
			std::shared_ptr<ModelBone> bone = std::make_shared<ModelBone>();
			bone->index = file->Read<int32>();
			bone->name = Utils::ToWString(file->Read<std::string>());
			bone->parentIndex = file->Read<int32>();
			bone->transform = file->Read<Matrix>();

			_bones.push_back(bone);
		}
	}

	// Mesh
	{
		const uint32 count = file->Read<uint32>();

		for (uint32 i = 0; i < count; i++)
		{
			std::shared_ptr<ModelMesh> mesh = std::make_shared<ModelMesh>();

			mesh->name = Utils::ToWString(file->Read<std::string>());
			mesh->boneIndex = file->Read<int32>();

			// Material
			mesh->materialName = Utils::ToWString(file->Read<std::string>());

			//VertexData
			{
				const uint32 count = file->Read<uint32>();
				std::vector<VertexTextureNormalTangentBlendData> vertices;
				vertices.resize(count);

				void* data = vertices.data();
				file->Read(&data, sizeof(VertexTextureNormalTangentBlendData) * count);
				mesh->geometry->AddVertices(vertices);
			}

			//IndexData
			{
				const uint32 count = file->Read<uint32>();

				std::vector<uint32> indices;
				indices.resize(count);

				void* data = indices.data();
				file->Read(&data, sizeof(uint32) * count);
				mesh->geometry->AddIndices(indices);
			}

			mesh->CreateBuffers();

			_meshes.push_back(mesh);
		}
	}

	BindCacheInfo();
}

void Model::ReadAnimation(const std::wstring& filename)
{
	std::wstring fullPath = _modelPath + filename + L".clip";

	std::shared_ptr<FileUtils> file = std::make_shared<FileUtils>();
	file->Open(fullPath, FileMode::Read);

	std::shared_ptr<ModelAnimation> animation = std::make_shared<ModelAnimation>();

	animation->name = Utils::ToWString(file->Read<std::string>());
	animation->duration = file->Read<float>();
	animation->frameRate = file->Read<float>();
	animation->frameCount = file->Read<uint32>();

	uint32 keyframesCount = file->Read<uint32>();

	for (uint32 i = 0; i < keyframesCount; i++)
	{
		std::shared_ptr<ModelKeyframe> keyframe = std::make_shared<ModelKeyframe>();
		keyframe->boneName = Utils::ToWString(file->Read<std::string>());

		uint32 size = file->Read<uint32>();

		if (size > 0)
		{
			keyframe->transforms.resize(size);

			void* ptr = &keyframe->transforms[0];
			file->Read(&ptr, sizeof(ModelKeyframeData) * size);
		}
		animation->keyframes[keyframe->boneName] = keyframe;
	}

	_animations.push_back(animation);
}


std::shared_ptr<Material> Model::GetMaterialByName(const std::wstring& name)
{
	for (auto& material : _materials)
	{
		if (material->GetName() == name) return material;
	}

	return nullptr;
}

std::shared_ptr<ModelMesh> Model::GetMeshByName(const std::wstring& name)
{
	for (auto& mesh : _meshes)
	{
		if (mesh->name == name) return mesh;
	}

	return nullptr;
}

std::shared_ptr<ModelBone> Model::GetBoneByName(const std::wstring& name)
{
	for (auto& bone : _bones)
	{
		if (bone->name == name) return bone;
	}

	return nullptr;
}

std::shared_ptr<ModelAnimation> Model::GetAnimationByName(const std::wstring& name)
{
	for (auto& animation : _animations)
	{
		if (animation->name == name)
			return animation;
	}

	return nullptr;
}

void Model::BindCacheInfo()
{
	// Mesh에 Material 캐싱
	for (const auto& mesh : _meshes)
	{
		// 이미 찾았으면 스킵
		if (mesh->material != nullptr) continue;

		mesh->material = GetMaterialByName(mesh->materialName);
	}

	// Mesh에 Bone 캐싱
	for (const auto& mesh : _meshes)
	{
		// 이미 찾았으면 스킵
		if (mesh->bone != nullptr) continue;

		mesh->bone = GetBoneByIndex(mesh->boneIndex);
	}

	// Bone 계층 정보 채우기
	if (_root == nullptr && _bones.size() > 0)
	{
		_root = _bones[0];

		for (const auto& bone : _bones)
		{
			if (bone->parentIndex >= 0)
			{
				bone->parent = _bones[bone->parentIndex];
				bone->parent->children.push_back(bone);
			}
			else
			{
				bone->parent = nullptr;
			}
		}
	}
}