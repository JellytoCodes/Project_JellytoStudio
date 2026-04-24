#include "Framework.h"
#include "Converter.h"
#include <filesystem>
#include "Graphics/Graphics.h"
#include "Utils/Utils.h"
#include "Utils/tinyxml2.h"
#include "Utils/FileUtils.h"

Converter::Converter()
{
    _importer = std::make_shared<Assimp::Importer>();
    _importer->SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
}

Converter::~Converter()
{
}

void Converter::ResetState()
{
    _bones.clear();
    _meshes.clear();
    _materials.clear();
    _boneIndexMap.clear();
    _scene = nullptr;
}

void Converter::ReadAssetFile(std::wstring file)
{
    ResetState();

    std::wstring fileStr = _assetPath + file;
    auto p = std::filesystem::path(fileStr);
    assert(std::filesystem::exists(p));

    _scene = _importer->ReadFile(
        Utils::ToString(fileStr),
        aiProcess_ConvertToLeftHanded |
        aiProcess_Triangulate |
        aiProcess_GenUVCoords |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_LimitBoneWeights
    );

    assert(_scene != nullptr);
}

void Converter::ReadAssetFileAbsolute(const std::wstring& absolutePath)
{
    ResetState();

    auto p = std::filesystem::path(absolutePath);
    assert(std::filesystem::exists(p));

    _scene = _importer->ReadFile(
        Utils::ToString(absolutePath),
        aiProcess_ConvertToLeftHanded |
        aiProcess_Triangulate |
        aiProcess_GenUVCoords |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_LimitBoneWeights
    );

    assert(_scene != nullptr);
}

void Converter::ExportModelData(std::wstring savePath)
{
    std::wstring finalPath = _modelPath + savePath + L".mesh";

    ReadModelData(_scene->mRootNode, -1, -1);
    BuildBoneIndexMap();

    ReadSkinData();
    WriteModelFile(finalPath);
}

void Converter::ExportMaterialData(std::wstring savePath)
{
    std::wstring finalPath = _texturePath + savePath + L".xml";
    ReadMaterialData();
    WriteMaterialData(finalPath);
}

void Converter::ExportAnimationData(std::wstring savePath, uint32 index /*= 0*/)
{
    std::wstring finalPath = _modelPath + savePath + L".clip";
    assert(index < _scene->mNumAnimations);

    std::shared_ptr<asAnimation> animation = ReadAnimationData(_scene->mAnimations[index]);
    WriteAnimationData(animation, finalPath);
}

/*static*/ std::string Converter::NormalizeBoneName(const std::string& raw)
{
    static const std::string kFbxSuffixes[] =
    {
        "_$AssimpFbx$_PreRotation",
        "_$AssimpFbx$_PostRotation",
        "_$AssimpFbx$_RotationPivot",
        "_$AssimpFbx$_RotationPivotInverse",
        "_$AssimpFbx$_ScalingPivot",
        "_$AssimpFbx$_ScalingPivotInverse",
        "_$AssimpFbx$_Translation",
        "_$AssimpFbx$_Scaling",
        "_$AssimpFbx$_Rotation",
    };

    std::string result = raw;
    for (const auto& suffix : kFbxSuffixes)
    {
        if (result.size() > suffix.size() &&
            result.compare(result.size() - suffix.size(), suffix.size(), suffix) == 0)
        {
            result.erase(result.size() - suffix.size());
            break;  // suffix는 하나만 붙음
        }
    }

    const size_t colonPos = result.rfind(':');
    if (colonPos != std::string::npos)
        result = result.substr(colonPos + 1);

    return result;
}

void Converter::BuildBoneIndexMap()
{
    _boneIndexMap.clear();
    _boneIndexMap.reserve(_bones.size() * 2);

    for (const auto& bone : _bones)
    {
        const std::string& original = bone->name;
        const std::string  normalized = NormalizeBoneName(original);

        _boneIndexMap.emplace(original, static_cast<uint32>(bone->index));

        if (normalized != original)
            _boneIndexMap.emplace(normalized, static_cast<uint32>(bone->index));
    }
}

uint32 Converter::GetBoneIndex(const std::string& name)
{
    {
        auto it = _boneIndexMap.find(name);
        if (it != _boneIndexMap.end())
            return it->second;
    }

    {
        const std::string normalized = NormalizeBoneName(name);
        auto it = _boneIndexMap.find(normalized);
        if (it != _boneIndexMap.end())
            return it->second;
    }

    const std::string msg =
        "[Converter::GetBoneIndex] Bone not found: '"
        + name + "'. Defaulting to root(0). "
        + "Check if AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS is set to false.\n";
    ::OutputDebugStringA(msg.c_str());

    return 0;
}

void Converter::ReadModelData(aiNode* node, int32 index, int32 parent)
{
    auto bone = std::make_shared<asBone>();

    const int32 boneIndex = static_cast<int32>(_bones.size());
    bone->index = boneIndex;
    bone->parent = parent;
    bone->name = node->mName.C_Str();

    Matrix transform(node->mTransformation[0]);
    bone->transform = transform.Transpose();

    Matrix matParent = Matrix::Identity;
    if (parent >= 0)
        matParent = _bones[parent]->transform;

    bone->transform = bone->transform * matParent;

    _bones.push_back(bone);

    ReadMeshData(node, boneIndex);

    for (uint32 i = 0; i < node->mNumChildren; i++)
        ReadModelData(node->mChildren[i], static_cast<int32>(_bones.size()), boneIndex);
}

void Converter::ReadMeshData(aiNode* node, int32 bone)
{
    if (node->mNumMeshes < 1)
        return;

    auto mesh = std::make_shared<asMesh>();
    mesh->name = node->mName.C_Str();
    mesh->boneIndex = bone;

    for (uint32 i = 0; i < node->mNumMeshes; i++)
    {
        const uint32  idx = node->mMeshes[i];
        const aiMesh* srcMesh = _scene->mMeshes[idx];

        const aiMaterial* material = _scene->mMaterials[srcMesh->mMaterialIndex];
        mesh->materialName = material->GetName().C_Str();

        const uint32 startVertex = static_cast<uint32>(mesh->vertices.size());

        for (uint32 v = 0; v < srcMesh->mNumVertices; v++)
        {
            VertexType vertex;
            ::memcpy(&vertex.position, &srcMesh->mVertices[v], sizeof(Vec3));

            if (srcMesh->HasTextureCoords(0))
                ::memcpy(&vertex.uv, &srcMesh->mTextureCoords[0][v], sizeof(Vec2));

            if (srcMesh->HasNormals())
                ::memcpy(&vertex.normal, &srcMesh->mNormals[v], sizeof(Vec3));

            if (srcMesh->HasTangentsAndBitangents())
                ::memcpy(&vertex.tangent, &srcMesh->mTangents[v], sizeof(Vec3));

            mesh->vertices.push_back(vertex);
        }

        for (uint32 f = 0; f < srcMesh->mNumFaces; f++)
        {
            const aiFace& face = srcMesh->mFaces[f];
            for (uint32 k = 0; k < face.mNumIndices; k++)
                mesh->indices.push_back(face.mIndices[k] + startVertex);
        }
    }

    _meshes.push_back(mesh);
}

// ---------------------------------------------------------------------------
void Converter::ReadSkinData()
{
    for (uint32 i = 0; i < _scene->mNumMeshes; i++)
    {
        aiMesh* srcMesh = _scene->mMeshes[i];
        if (!srcMesh->HasBones())
            continue;

        auto& mesh = _meshes[i];

        std::vector<asBoneWeights> tempVertexBoneWeights(mesh->vertices.size());

        for (uint32 b = 0; b < srcMesh->mNumBones; b++)
        {
            aiBone* srcMeshBone = srcMesh->mBones[b];
            const uint32 boneIndex = GetBoneIndex(srcMeshBone->mName.C_Str());

            for (uint32 w = 0; w < srcMeshBone->mNumWeights; w++)
            {
                const uint32 index = srcMeshBone->mWeights[w].mVertexId;
                const float  weight = srcMeshBone->mWeights[w].mWeight;
                tempVertexBoneWeights[index].AddWeights(boneIndex, weight);
            }
        }

        for (uint32 v = 0; v < static_cast<uint32>(tempVertexBoneWeights.size()); v++)
        {
            tempVertexBoneWeights[v].Normalize();
            const asBlendWeight bw = tempVertexBoneWeights[v].GetBlendWeights();
            mesh->vertices[v].blendIndices = bw.indices;
            mesh->vertices[v].blendWeights = bw.weights;
        }
    }
}

void Converter::WriteModelFile(std::wstring finalPath)
{
    auto path = std::filesystem::path(finalPath);
    std::filesystem::create_directories(path.parent_path());

    auto file = std::make_shared<FileUtils>();
    file->Open(finalPath, FileMode::Write);

    file->Write<uint32>(static_cast<uint32>(_bones.size()));
    for (const auto& bone : _bones)
    {
        file->Write<int32>(bone->index);
        file->Write<std::string>(bone->name);
        file->Write<int32>(bone->parent);
        file->Write<Matrix>(bone->transform);
    }

    file->Write<uint32>(static_cast<uint32>(_meshes.size()));
    for (const auto& mesh : _meshes)
    {
        file->Write<std::string>(mesh->name);
        file->Write<int32>(mesh->boneIndex);
        file->Write<std::string>(mesh->materialName);

        file->Write<uint32>(static_cast<uint32>(mesh->vertices.size()));
        file->Write(mesh->vertices.data(), sizeof(VertexType) * mesh->vertices.size());

        file->Write<uint32>(static_cast<uint32>(mesh->indices.size()));
        file->Write(mesh->indices.data(), sizeof(uint32) * mesh->indices.size());
    }
}

void Converter::ReadMaterialData()
{
    for (uint32 i = 0; i < _scene->mNumMaterials; i++)
    {
        aiMaterial* srcMaterial = _scene->mMaterials[i];
        auto material = std::make_shared<asMaterial>();

        material->name = srcMaterial->GetName().C_Str();

        aiColor3D color;
        srcMaterial->Get(AI_MATKEY_COLOR_AMBIENT, color);
        material->ambient = Color(color.r, color.g, color.b, 1.f);

        srcMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        material->diffuse = Color(color.r, color.g, color.b, 1.f);

        srcMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color);
        material->specular = Color(color.r, color.g, color.b, 1.f);
        srcMaterial->Get(AI_MATKEY_SHININESS, material->specular.w);

        srcMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, color);
        material->emissive = Color(color.r, color.g, color.b, 1.f);

        aiString file;
        srcMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &file);
        material->diffuseFile = file.C_Str();

        srcMaterial->GetTexture(aiTextureType_SPECULAR, 0, &file);
        material->specularFile = file.C_Str();

        srcMaterial->GetTexture(aiTextureType_NORMALS, 0, &file);
        material->normalFile = file.C_Str();

        _materials.push_back(material);
    }
}

void Converter::WriteMaterialData(std::wstring finalPath)
{
    auto path = std::filesystem::path(finalPath);
    std::filesystem::create_directories(path.parent_path());

    std::string folder = path.parent_path().string();

    auto document = std::make_shared<tinyxml2::XMLDocument>();
    tinyxml2::XMLDeclaration* decl = document->NewDeclaration();
    document->LinkEndChild(decl);

    tinyxml2::XMLElement* root = document->NewElement("Materials");
    document->LinkEndChild(root);

    for (const auto& material : _materials)
    {
        tinyxml2::XMLElement* node = document->NewElement("Material");
        root->LinkEndChild(node);

        tinyxml2::XMLElement* element = nullptr;

        element = document->NewElement("Name");
        element->SetText(material->name.c_str());
        node->LinkEndChild(element);

        element = document->NewElement("DiffuseFile");
        element->SetText(WriteTexture(folder, material->diffuseFile).c_str());
        node->LinkEndChild(element);

        element = document->NewElement("SpecularFile");
        element->SetText(WriteTexture(folder, material->specularFile).c_str());
        node->LinkEndChild(element);

        element = document->NewElement("NormalFile");
        element->SetText(WriteTexture(folder, material->normalFile).c_str());
        node->LinkEndChild(element);

        auto writeColor = [&](const char* tag, const Color& c)
            {
                element = document->NewElement(tag);
                element->SetAttribute("R", c.x);
                element->SetAttribute("G", c.y);
                element->SetAttribute("B", c.z);
                element->SetAttribute("A", c.w);
                node->LinkEndChild(element);
            };

        writeColor("Ambient", material->ambient);
        writeColor("Diffuse", material->diffuse);
        writeColor("Specular", material->specular);
        writeColor("Emissive", material->emissive);
    }

    document->SaveFile(Utils::ToString(finalPath).c_str());
}

std::string Converter::WriteTexture(std::string saveFolder, std::string file)
{
    std::string fileName = std::filesystem::path(file).filename().string();
    std::string folderName = std::filesystem::path(saveFolder).filename().string();

    const aiTexture* srcTexture = _scene->GetEmbeddedTexture(file.c_str());
    if (srcTexture)
    {
        std::string pathStr = (std::filesystem::path(saveFolder) / fileName).string();
        if (srcTexture->mHeight == 0)
        {
            auto fileUtils = std::make_shared<FileUtils>();
            fileUtils->Open(Utils::ToWString(pathStr), FileMode::Write);
            fileUtils->Write(srcTexture->pcData, srcTexture->mWidth);
        }
        else
        {
            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = srcTexture->mWidth;
            desc.Height = srcTexture->mHeight;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Usage = D3D11_USAGE_IMMUTABLE;

            D3D11_SUBRESOURCE_DATA subResource = {};
            subResource.pSysMem = srcTexture->pcData;

            ComPtr<ID3D11Texture2D> texture;
            CHECK(GET_SINGLE(Graphics)->GetDevice()->CreateTexture2D(
                &desc, &subResource, texture.GetAddressOf()));

            DirectX::ScratchImage img;
            ::CaptureTexture(
                GET_SINGLE(Graphics)->GetDevice().Get(),
                GET_SINGLE(Graphics)->GetDeviceContext().Get(),
                texture.Get(), img);

            CHECK(DirectX::SaveToDDSFile(
                *img.GetImages(), DirectX::DDS_FLAGS_NONE,
                Utils::ToWString(fileName).c_str()));
        }
    }
    else
    {
        std::string originStr =
            (std::filesystem::path(_assetPath) / folderName / file).string();
        Utils::Replace(originStr, "\\", "/");

        std::string pathStr =
            (std::filesystem::path(saveFolder) / fileName).string();
        Utils::Replace(pathStr, "\\", "/");

        ::CopyFileA(originStr.c_str(), pathStr.c_str(), false);
    }

    return fileName;
}

std::shared_ptr<asAnimation> Converter::ReadAnimationData(aiAnimation* srcAnimation)
{
    auto animation = std::make_shared<asAnimation>();
    animation->name = srcAnimation->mName.C_Str();

    const float ticksPerSecond = (srcAnimation->mTicksPerSecond > 0.0)
        ? static_cast<float>(srcAnimation->mTicksPerSecond)
        : 30.0f;
    animation->frameRate = ticksPerSecond;

    uint32 maxTick = 0;
    for (uint32 i = 0; i < srcAnimation->mNumChannels; i++)
    {
        const aiNodeAnim* ch = srcAnimation->mChannels[i];

        auto scanMaxTick = [&](uint32 numKeys, auto* keys)
            {
                for (uint32 k = 0; k < numKeys; k++)
                {
                    const double t = keys[k].mTime;
                    if (std::isfinite(t) && t >= 0.0)
                        maxTick = max(maxTick, static_cast<uint32>(t));
                }
            };

        scanMaxTick(ch->mNumPositionKeys, ch->mPositionKeys);
        scanMaxTick(ch->mNumRotationKeys, ch->mRotationKeys);
        scanMaxTick(ch->mNumScalingKeys, ch->mScalingKeys);
    }

    const uint32 effectiveDuration = (maxTick > 0)
        ? maxTick
        : static_cast<uint32>(srcAnimation->mDuration);

    animation->frameCount = effectiveDuration + 1;
    animation->duration = static_cast<float>(effectiveDuration) / ticksPerSecond;

    std::unordered_map<std::string, std::shared_ptr<asAnimationNode>> cacheAnimNodes;
    cacheAnimNodes.reserve(srcAnimation->mNumChannels * 2);

    for (uint32 i = 0; i < srcAnimation->mNumChannels; i++)
    {
        aiNodeAnim* srcNode = srcAnimation->mChannels[i];
        auto node = ParseAnimationNode(animation, srcNode);

        const std::string rawName = srcNode->mNodeName.C_Str();
        const std::string normalizedName = NormalizeBoneName(rawName);

        cacheAnimNodes[rawName] = node;

        cacheAnimNodes[normalizedName] = node;
    }

    ReadKeyframeData(animation, _scene->mRootNode, cacheAnimNodes);
    return animation;
}

std::shared_ptr<asAnimationNode> Converter::ParseAnimationNode(std::shared_ptr<asAnimation> animation, aiNodeAnim* srcNode)
{
    auto node = std::make_shared<asAnimationNode>();
    node->name = srcNode->mNodeName.C_Str();

    const uint32 frameCount = animation->frameCount;

    auto buildIndexMap = [](uint32 numKeys, auto* keys) -> std::vector<uint32>
        {
            uint32 maxFrame = 0;
            for (uint32 i = 0; i < numKeys; i++)
            {
                const double t = keys[i].mTime;
                if (!std::isfinite(t) || t < 0.0) continue;
                maxFrame = max(maxFrame, static_cast<uint32>(t));
            }

            std::vector<uint32> map(maxFrame + 1, UINT32_MAX);
            for (uint32 i = 0; i < numKeys; i++)
            {
                const double t = keys[i].mTime;
                if (!std::isfinite(t) || t < 0.0) continue;
                const uint32 f = static_cast<uint32>(t);
                if (f <= maxFrame && map[f] == UINT32_MAX)
                    map[f] = i;
            }
            return map;
        };

    const auto posMap = buildIndexMap(srcNode->mNumPositionKeys, srcNode->mPositionKeys);
    const auto rotMap = buildIndexMap(srcNode->mNumRotationKeys, srcNode->mRotationKeys);
    const auto scaleMap = buildIndexMap(srcNode->mNumScalingKeys, srcNode->mScalingKeys);

    auto findKey = [](const std::vector<uint32>& map, uint32 frame) -> uint32
        {
            if (frame < map.size() && map[frame] != UINT32_MAX)
                return map[frame];

            int32 f = static_cast<int32>(std::min(frame, static_cast<uint32>(map.size()) - 1));
            while (f >= 0)
            {
                if (map[f] != UINT32_MAX) return map[f];
                f--;
            }
            return 0;
        };

    auto findNextKey = [](const std::vector<uint32>& map, uint32 frame) -> uint32
        {
            for (uint32 i = frame + 1; i < static_cast<uint32>(map.size()); i++)
                if (map[i] != UINT32_MAX) return map[i];
            return UINT32_MAX;
        };

    node->keyframe.reserve(frameCount);

    for (uint32 f = 0; f < frameCount; f++)
    {
        asKeyframeData frameData;
        frameData.time = static_cast<float>(f) / animation->frameRate;

        // --- Position ---
        if (srcNode->mNumPositionKeys > 0)
        {
            const uint32 k = findKey(posMap, f);
            const uint32 kNextIdx = findNextKey(posMap, f);
            const uint32 kNext = (kNextIdx != UINT32_MAX) ? kNextIdx : k;

            aiVector3D p0 = srcNode->mPositionKeys[k].mValue;
            if (kNext != k)
            {
                const aiVector3D p1 = srcNode->mPositionKeys[kNext].mValue;
                const float t0 = static_cast<float>(srcNode->mPositionKeys[k].mTime);
                const float t1 = static_cast<float>(srcNode->mPositionKeys[kNext].mTime);
                const float factor = std::clamp(
                    (t1 - t0) > 0.f ? (static_cast<float>(f) - t0) / (t1 - t0) : 0.f,
                    0.f, 1.f);
                p0 = p0 + (p1 - p0) * factor;
            }
            ::memcpy_s(&frameData.translation, sizeof(Vec3), &p0, sizeof(aiVector3D));
        }
        else
        {
            frameData.translation = Vec3(0.f, 0.f, 0.f);
        }

        if (srcNode->mNumRotationKeys > 0)
        {
            const uint32 k = findKey(rotMap, f);
            const uint32 kNextIdx = findNextKey(rotMap, f);
            const uint32 kNext = (kNextIdx != UINT32_MAX) ? kNextIdx : k;

            aiQuaternion r0 = srcNode->mRotationKeys[k].mValue;
            if (kNext != k)
            {
                const aiQuaternion r1 = srcNode->mRotationKeys[kNext].mValue;
                const float t0 = static_cast<float>(srcNode->mRotationKeys[k].mTime);
                const float t1 = static_cast<float>(srcNode->mRotationKeys[kNext].mTime);
                const float factor = std::clamp(
                    (t1 - t0) > 0.f ? (static_cast<float>(f) - t0) / (t1 - t0) : 0.f,
                    0.f, 1.f);
                aiQuaternion interpolated;
                aiQuaternion::Interpolate(interpolated, r0, r1, factor);
                r0 = interpolated;
            }
            frameData.rotation = { r0.x, r0.y, r0.z, r0.w };
        }
        else
        {
            frameData.rotation = { 0.f, 0.f, 0.f, 1.f };
        }

        // --- Scale ---
        if (srcNode->mNumScalingKeys > 0)
        {
            const uint32 k = findKey(scaleMap, f);
            const uint32 kNextIdx = findNextKey(scaleMap, f);
            const uint32 kNext = (kNextIdx != UINT32_MAX) ? kNextIdx : k;

            aiVector3D s0 = srcNode->mScalingKeys[k].mValue;
            if (kNext != k)
            {
                const aiVector3D s1 = srcNode->mScalingKeys[kNext].mValue;
                const float t0 = static_cast<float>(srcNode->mScalingKeys[k].mTime);
                const float t1 = static_cast<float>(srcNode->mScalingKeys[kNext].mTime);
                const float factor = std::clamp(
                    (t1 - t0) > 0.f ? (static_cast<float>(f) - t0) / (t1 - t0) : 0.f,
                    0.f, 1.f);
                s0 = s0 + (s1 - s0) * factor;
            }
            ::memcpy_s(&frameData.scale, sizeof(Vec3), &s0, sizeof(aiVector3D));
        }
        else
        {
            frameData.scale = Vec3(1.f, 1.f, 1.f);
        }

        node->keyframe.push_back(frameData);
    }

    return node;
}

void Converter::ReadKeyframeData(std::shared_ptr<asAnimation> animation, aiNode* srcNode, std::unordered_map<std::string, std::shared_ptr<asAnimationNode>>& cache)
{
    auto keyframe = std::make_shared<asKeyframe>();
    keyframe->boneName = srcNode->mName.C_Str();

    std::shared_ptr<asAnimationNode> findNode = nullptr;
    {
        const std::string rawName = srcNode->mName.C_Str();
        const auto it = cache.find(rawName);
        if (it != cache.end())
        {
            findNode = it->second;
        }
        else
        {
            const std::string normalizedName = NormalizeBoneName(rawName);
            const auto it2 = cache.find(normalizedName);
            if (it2 != cache.end())
                findNode = it2->second;
        }
    }

    keyframe->transforms.reserve(animation->frameCount);

    for (uint32 i = 0; i < animation->frameCount; i++)
    {
        asKeyframeData frameData;

        if (findNode == nullptr)
        {
            Matrix transform(srcNode->mTransformation[0]);
            transform = transform.Transpose();
            frameData.time = static_cast<float>(i) / animation->frameRate;
            transform.Decompose(OUT frameData.scale, OUT frameData.rotation,
                OUT frameData.translation);
        }
        else
        {
            frameData = findNode->keyframe[i];
        }

        keyframe->transforms.push_back(frameData);
    }

    animation->keyframes.push_back(keyframe);

    for (uint32 i = 0; i < srcNode->mNumChildren; i++)
        ReadKeyframeData(animation, srcNode->mChildren[i], cache);
}

void Converter::WriteAnimationData(
    std::shared_ptr<asAnimation> animation,
    std::wstring finalPath)
{
    auto path = std::filesystem::path(finalPath);
    std::filesystem::create_directories(path.parent_path());

    auto file = std::make_shared<FileUtils>();
    file->Open(finalPath, FileMode::Write);

    file->Write<std::string>(animation->name);
    file->Write<float>(animation->duration);
    file->Write<float>(animation->frameRate);
    file->Write<uint32>(animation->frameCount);
    file->Write<uint32>(static_cast<uint32>(animation->keyframes.size()));

    for (const auto& keyframe : animation->keyframes)
    {
        file->Write<std::string>(keyframe->boneName);
        file->Write<uint32>(static_cast<uint32>(keyframe->transforms.size()));
        if (!keyframe->transforms.empty())
        {
            file->Write(keyframe->transforms.data(),
                sizeof(asKeyframeData) * keyframe->transforms.size());
        }
    }
}

void Converter::ExportCSV(std::wstring savePath)
{
    FILE* file;
    ::fopen_s(&file, "../Vertices.csv", "w");
    if (!file) return;

    for (const auto& bone : _bones)
        ::fprintf(file, "%d,%s\n", bone->index, bone->name.c_str());
    ::fprintf(file, "\n");

    for (const auto& mesh : _meshes)
    {
        ::printf("%s\n", mesh->name.c_str());
        for (uint32 i = 0; i < static_cast<uint32>(mesh->vertices.size()); i++)
        {
            const Vec3 p = mesh->vertices[i].position;
            const Vec4 indices = mesh->vertices[i].blendIndices;
            const Vec4 weights = mesh->vertices[i].blendWeights;

            ::fprintf(file, "%f,%f,%f,", p.x, p.y, p.z);
            ::fprintf(file, "%f,%f,%f,%f,",
                indices.x, indices.y, indices.z, indices.w);
            ::fprintf(file, "%f,%f,%f,%f\n",
                weights.x, weights.y, weights.z, weights.w);
        }
    }
    ::fclose(file);
}

void Converter::ExportAnimationCSV(std::wstring savePath)
{
    std::string tag = Utils::ToString(savePath);
    for (char& c : tag) if (c == '/' || c == '\\') c = '_';
    const std::string csvPath = "../AnimDebug_" + tag + ".csv";

    FILE* file;
    if (::fopen_s(&file, csvPath.c_str(), "w") != 0 || !file)
    {
        ::printf("[ExportAnimationCSV] Failed to open: %s\n", csvPath.c_str());
        return;
    }

    for (uint32 animIdx = 0; animIdx < _scene->mNumAnimations; animIdx++)
    {
        aiAnimation* srcAnim = _scene->mAnimations[animIdx];
        const float tps = (srcAnim->mTicksPerSecond > 0.0)
            ? static_cast<float>(srcAnim->mTicksPerSecond) : 30.f;
        const uint32 frameCount = static_cast<uint32>(srcAnim->mDuration) + 1;
        const float  duration = static_cast<float>(srcAnim->mDuration) / tps;

        ::fprintf(file, "[ANIM_META]\n");
        ::fprintf(file, "AnimIndex,Name,mDuration_ticks,mTicksPerSecond,FrameCount,Duration_sec\n");
        ::fprintf(file, "%u,%s,%.4f,%.4f,%u,%.6f\n\n",
            animIdx, srcAnim->mName.C_Str(),
            static_cast<float>(srcAnim->mDuration), tps, frameCount, duration);

        ::fprintf(file, "[RAW_KEYS] AnimIndex=%u\n", animIdx);
        ::fprintf(file, "Channel,KeyType,KeyIndex,mTime_ticks\n");
        for (uint32 ch = 0; ch < srcAnim->mNumChannels; ch++)
        {
            aiNodeAnim* node = srcAnim->mChannels[ch];
            const char* nodeName = node->mNodeName.C_Str();

            for (uint32 k = 0; k < node->mNumPositionKeys; k++)
                ::fprintf(file, "%s,POS,%u,%.6f\n",
                    nodeName, k, static_cast<float>(node->mPositionKeys[k].mTime));
            for (uint32 k = 0; k < node->mNumRotationKeys; k++)
                ::fprintf(file, "%s,ROT,%u,%.6f\n",
                    nodeName, k, static_cast<float>(node->mRotationKeys[k].mTime));
            for (uint32 k = 0; k < node->mNumScalingKeys; k++)
                ::fprintf(file, "%s,SCL,%u,%.6f\n",
                    nodeName, k, static_cast<float>(node->mScalingKeys[k].mTime));
        }
        ::fprintf(file, "\n");

        const auto parsedAnim = ReadAnimationData(srcAnim);
        ::fprintf(file, "[PARSED_KEYFRAMES] AnimIndex=%u FrameCount=%u\n",
            animIdx, parsedAnim->frameCount);
        ::fprintf(file, "BoneName,Frame,Time_sec,Tx,Ty,Tz,Rx,Ry,Rz,Rw,Sx,Sy,Sz,QuatNorm\n");

        for (const auto& kf : parsedAnim->keyframes)
        {
            for (uint32 f = 0; f < static_cast<uint32>(kf->transforms.size()); f++)
            {
                const asKeyframeData& d = kf->transforms[f];
                const float quatNorm = sqrtf(
                    d.rotation.x * d.rotation.x +
                    d.rotation.y * d.rotation.y +
                    d.rotation.z * d.rotation.z +
                    d.rotation.w * d.rotation.w);

                ::fprintf(file,
                    "%s,%u,%.6f,"
                    "%.6f,%.6f,%.6f,"
                    "%.6f,%.6f,%.6f,%.6f,"
                    "%.6f,%.6f,%.6f,"
                    "%.6f\n",
                    kf->boneName.c_str(), f, d.time,
                    d.translation.x, d.translation.y, d.translation.z,
                    d.rotation.x, d.rotation.y, d.rotation.z, d.rotation.w,
                    d.scale.x, d.scale.y, d.scale.z,
                    quatNorm);
            }
        }
        ::fprintf(file, "\n");
    }

    ::fclose(file);
    ::printf("[ExportAnimationCSV] Saved: %s\n", csvPath.c_str());
}