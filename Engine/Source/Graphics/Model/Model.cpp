
#include "Framework.h"
#include "Model.h"

#include "Graphics/Model/ModelAnimation.h"
#include "Resource/Material.h"
#include "Resource/Managers/ResourceManager.h"
#include "Utils/Utils.h"
#include "Utils/FileUtils.h"

Model::Model() = default;
Model::~Model() = default;

void Model::ReadMaterial(const std::wstring& filename)
{
    const std::wstring fullPath   = _texturePath + filename + L".json";
    const auto         parentPath = std::filesystem::path(fullPath).parent_path();
 
    std::ifstream ifs(Utils::ToString(fullPath));
    assert(ifs.is_open() && "머티리얼 JSON 파일을 열 수 없습니다. 경로 확인 필요.");
    if (!ifs.is_open()) return;
 
    nlohmann::json doc;
    try
    {
        ifs >> doc;
    }
    catch (const nlohmann::json::parse_error& e)
    {
        ::OutputDebugStringA(e.what());
        assert(false && "머티리얼 JSON 파싱 실패");
        return;
    }
 
    assert(doc.contains("materials") && "JSON 에 \"materials\" 배열이 없습니다.");
 
    auto ReadColor = [](const nlohmann::json& j, const char* key) -> Color
    {
        Color c;
        if (j.contains(key) && j[key].is_array() && j[key].size() == 4)
        {
            c.x = j[key][0].get<float>();
            c.y = j[key][1].get<float>();
            c.z = j[key][2].get<float>();
            c.w = j[key][3].get<float>();
        }
        return c;
    };
 
    for (const auto& m : doc["materials"])
    {
        auto material = std::make_unique<Material>();
 
        material->SetName(Utils::ToWString(m.value("name", "")));
 
        auto LoadTex = [&](const char* key,
                           auto (Material::* setter)(std::shared_ptr<Texture>))
        {
            const std::string file = m.value(key, "");
            if (!file.empty())
            {
                const std::wstring wFile = Utils::ToWString(file);
                (material.get()->*setter)(
                    GET_SINGLE(ResourceManager)->GetOrAddTexture(
                        wFile, (parentPath / file).wstring()));
            }
        };
 
        LoadTex("diffuseFile",  &Material::SetDiffuseMap);
        LoadTex("specularFile", &Material::SetSpecularMap);
        LoadTex("normalFile",   &Material::SetNormalMap);
 
        MaterialDesc& desc    = material->GetMaterialDesc();
        desc.ambient  = ReadColor(m, "ambient");
        desc.diffuse  = ReadColor(m, "diffuse");
        desc.specular = ReadColor(m, "specular");
        desc.emissive = ReadColor(m, "emissive");
 
        _materials.push_back(std::move(material));
    }
 
    BindCacheInfo();
}

void Model::ReadModel(const std::wstring& filename)
{
	std::wstring fullPath = _modelPath + filename + L".mesh";

	FileUtils file;
	file.Open(fullPath, FileMode::Read);

	{
		const uint32 count = file.Read<uint32>();
		_bones.reserve(count);
		for (uint32 i = 0; i < count; i++)
		{
			auto bone       = std::make_unique<ModelBone>();
			bone->index       = file.Read<int32>();
			bone->name        = Utils::ToWString(file.Read<std::string>());
			bone->parentIndex = file.Read<int32>();
			bone->transform   = file.Read<Matrix>();
			_bones.push_back(std::move(bone));
		}
	}

	{
		const uint32 count = file.Read<uint32>();
		_meshes.reserve(count);
		for (uint32 i = 0; i < count; i++)
		{
			auto mesh         = std::make_unique<ModelMesh>();
			mesh->name        = Utils::ToWString(file.Read<std::string>());
			mesh->boneIndex   = file.Read<int32>();
			mesh->materialName = Utils::ToWString(file.Read<std::string>());

			{
				const uint32 vCount = file.Read<uint32>();
				std::vector<VertexTextureNormalTangentBlendData> vertices(vCount);
				void* data = vertices.data();
				file.Read(&data, sizeof(VertexTextureNormalTangentBlendData) * vCount);
				mesh->geometry->AddVertices(vertices);
			}

			{
				const uint32 iCount = file.Read<uint32>();
				std::vector<uint32> indices(iCount);
				void* data = indices.data();
				file.Read(&data, sizeof(uint32) * iCount);
				mesh->geometry->AddIndices(indices);
			}

			mesh->CreateBuffers();
			_meshes.push_back(std::move(mesh));
		}
	}

	BindCacheInfo();
}

void Model::ReadAnimation(const std::wstring& filename)
{
	std::wstring fullPath = _modelPath + filename + L".clip";

	FileUtils file;
	file.Open(fullPath, FileMode::Read);

	auto animation         = std::make_unique<ModelAnimation>();
	animation->name        = Utils::ToWString(file.Read<std::string>());
	animation->duration    = file.Read<float>();
	animation->frameRate   = file.Read<float>();
	animation->frameCount  = file.Read<uint32>();

	const uint32 keyframesCount = file.Read<uint32>();

	for (uint32 i = 0; i < keyframesCount; i++)
	{
		auto keyframe      = std::make_unique<ModelKeyframe>();
		keyframe->boneName = Utils::ToWString(file.Read<std::string>());

		const uint32 size = file.Read<uint32>();
		if (size > 0)
		{
			keyframe->transforms.resize(size);
			void* ptr = keyframe->transforms.data();
			file.Read(&ptr, sizeof(ModelKeyframeData) * size);
		}

		animation->keyframes[keyframe->boneName] = std::move(keyframe);
	}

	_animations.push_back(std::move(animation));
}

std::shared_ptr<Material> Model::GetMaterialByName(const std::wstring& name)
{
	for (auto& material : _materials)
		if (material->GetName() == name) return material;
	return nullptr;
}

ModelMesh* Model::GetMeshByName(const std::wstring& name)
{
	for (auto& mesh : _meshes)
		if (mesh->name == name) return mesh.get();
	return nullptr;
}

ModelBone* Model::GetBoneByName(const std::wstring& name)
{
	for (auto& bone : _bones)
		if (bone->name == name) return bone.get();
	return nullptr;
}

ModelAnimation* Model::GetAnimationByName(const std::wstring& name)
{
	for (auto& animation : _animations)
		if (animation->name == name) return animation.get();
	return nullptr;
}

void Model::BindCacheInfo()
{
	for (const auto& mesh : _meshes)
	{
		if (mesh->material != nullptr) continue;
		mesh->material = GetMaterialByName(mesh->materialName);
	}

	for (const auto& mesh : _meshes)
	{
		if (mesh->bone != nullptr) continue;
		mesh->bone = GetBoneByIndex(mesh->boneIndex);
	}

	if (_root == nullptr && !_bones.empty())
	{
		_root = _bones[0].get();

		for (const auto& bone : _bones)
		{
			if (bone->parentIndex >= 0)
			{
				bone->parent = _bones[bone->parentIndex].get();
				bone->parent->children.push_back(bone.get());
			}
			else
			{
				bone->parent = nullptr;
			}
		}
	}
}
