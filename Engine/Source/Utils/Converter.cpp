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
}

Converter::~Converter()
{

}

void Converter::ReadAssetFile(std::wstring file)
{
	std::wstring fileStr = _assetPath + file;

	auto p = std::filesystem::path(fileStr);
	assert(std::filesystem::exists(p));

	_scene = _importer->ReadFile(
		Utils::ToString(fileStr),
		aiProcess_ConvertToLeftHanded |
		aiProcess_Triangulate |
		aiProcess_GenUVCoords |
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace
	);

	assert(_scene != nullptr);
}

void Converter::ReadAssetFileAbsolute(const std::wstring& absolutePath)
{
	auto p = std::filesystem::path(absolutePath);
	assert(std::filesystem::exists(p));

	_scene = _importer->ReadFile(
		Utils::ToString(absolutePath),
		aiProcess_ConvertToLeftHanded |
		aiProcess_Triangulate |
		aiProcess_GenUVCoords |
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace
	);
	assert(_scene != nullptr);
}

void Converter::ExportModelData(std::wstring savePath)
{
	std::wstring finalPath = _modelPath + savePath + L".mesh";
	ReadModelData(_scene->mRootNode, -1, -1);
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

void Converter::ExportCSV(std::wstring savePath)
{
	FILE* file;
	::fopen_s(&file, "../Vertices.csv", "w");

	for (std::shared_ptr<asBone>& bone : _bones)
	{
		std::string name = bone->name;
		::fprintf(file, "%d,%s\n", bone->index, bone->name.c_str());
	}

	::fprintf(file, "\n");

	for (std::shared_ptr<asMesh>& mesh : _meshes)
	{
		std::string name = mesh->name;
		::printf("%s\n", name.c_str());

		for (UINT i = 0; i < mesh->vertices.size(); i++)
		{
			Vec3 p = mesh->vertices[i].position;
			Vec4 indices = mesh->vertices[i].blendIndices;
			Vec4 weights = mesh->vertices[i].blendWeights;

			::fprintf(file, "%f,%f,%f,", p.x, p.y, p.z);
			::fprintf(file, "%f,%f,%f,%f,", indices.x, indices.y, indices.z, indices.w);
			::fprintf(file, "%f,%f,%f,%f\n", weights.x, weights.y, weights.z, weights.w);
		}
	}

	::fclose(file);
}

void Converter::ExportAnimationCSV(std::wstring savePath)
{
	std::string tag = Utils::ToString(savePath);
	for (char& c : tag) if (c == '/' || c == '\\') c = '_';
	std::string csvPath = "../AnimDebug_" + tag + ".csv";

	FILE* file;
	if (::fopen_s(&file, csvPath.c_str(), "w") != 0 || !file)
	{
		::printf("[ExportAnimationCSV] Failed to open: %s\n", csvPath.c_str());
		return;
	}

	for (uint32 animIdx = 0; animIdx < _scene->mNumAnimations; animIdx++)
	{
		aiAnimation* srcAnim = _scene->mAnimations[animIdx];
		float tps = (srcAnim->mTicksPerSecond > 0.0) ? (float)srcAnim->mTicksPerSecond : 30.f;
		uint32 frameCount = (uint32)srcAnim->mDuration + 1;
		float duration = (float)srcAnim->mDuration / tps;

		// 1. 메타 정보
		::fprintf(file, "[ANIM_META]\n");
		::fprintf(file, "AnimIndex,Name,mDuration_ticks,mTicksPerSecond,FrameCount,Duration_sec\n");
		::fprintf(file, "%u,%s,%.4f,%.4f,%u,%.6f\n\n",
			animIdx, srcAnim->mName.C_Str(),
			(float)srcAnim->mDuration, tps, frameCount, duration);

		// 2. 원본 mTime 틱 값 (이상 프레임 진단용)
		::fprintf(file, "[RAW_KEYS] AnimIndex=%u\n", animIdx);
		::fprintf(file, "Channel,KeyType,KeyIndex,mTime_ticks\n");
		for (uint32 ch = 0; ch < srcAnim->mNumChannels; ch++)
		{
			aiNodeAnim* node = srcAnim->mChannels[ch];
			const char* nodeName = node->mNodeName.C_Str();

			for (uint32 k = 0; k < node->mNumPositionKeys; k++)
				::fprintf(file, "%s,POS,%u,%.6f\n", nodeName, k, (float)node->mPositionKeys[k].mTime);
			for (uint32 k = 0; k < node->mNumRotationKeys; k++)
				::fprintf(file, "%s,ROT,%u,%.6f\n", nodeName, k, (float)node->mRotationKeys[k].mTime);
			for (uint32 k = 0; k < node->mNumScalingKeys; k++)
				::fprintf(file, "%s,SCL,%u,%.6f\n", nodeName, k, (float)node->mScalingKeys[k].mTime);
		}
		::fprintf(file, "\n");

		// 3. 파싱 후 asKeyframeData (bone별 프레임별 TRS + QuatNorm)
		std::shared_ptr<asAnimation> parsedAnim = ReadAnimationData(srcAnim);

		::fprintf(file, "[PARSED_KEYFRAMES] AnimIndex=%u FrameCount=%u\n", animIdx, parsedAnim->frameCount);
		::fprintf(file, "BoneName,Frame,Time_sec,Tx,Ty,Tz,Rx,Ry,Rz,Rw,Sx,Sy,Sz,QuatNorm\n");

		for (auto& kf : parsedAnim->keyframes)
		{
			for (uint32 f = 0; f < (uint32)kf->transforms.size(); f++)
			{
				const asKeyframeData& d = kf->transforms[f];
				float quatNorm = sqrtf(
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

void Converter::ReadModelData(aiNode* node, int32 index, int32 parent)
{
	std::shared_ptr<asBone> bone = std::make_shared<asBone>();

	int32 boneIndex = (int32)_bones.size();
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
	{
		ReadModelData(node->mChildren[i], (int32)_bones.size(), boneIndex);
	}
}

void Converter::ReadMeshData(aiNode* node, int32 bone)
{
	if (node->mNumMeshes < 1)
		return;

	std::shared_ptr<asMesh> mesh = std::make_shared<asMesh>();
	mesh->name = node->mName.C_Str();
	mesh->boneIndex = bone;

	for (uint32 i = 0; i < node->mNumMeshes; i++)
	{
		uint32 index = node->mMeshes[i];
		const aiMesh* srcMesh = _scene->mMeshes[index];

		const aiMaterial* material = _scene->mMaterials[srcMesh->mMaterialIndex];
		mesh->materialName = material->GetName().C_Str();

		const uint32 startVertex = mesh->vertices.size();

		for (uint32 v = 0; v < srcMesh->mNumVertices; v++)
		{
			VertexType vertex;
			::memcpy(&vertex.position, &srcMesh->mVertices[v], sizeof(Vec3));

			if (srcMesh->HasTextureCoords(0))
				::memcpy(&vertex.uv, &srcMesh->mTextureCoords[0][v], sizeof(Vec2));

			if (srcMesh->HasNormals())
				::memcpy(&vertex.normal, &srcMesh->mNormals[v], sizeof(Vec3));

			mesh->vertices.push_back(vertex);
		}

		for (uint32 f = 0; f < srcMesh->mNumFaces; f++)
		{
			aiFace& face = srcMesh->mFaces[f];

			for (uint32 k = 0; k < face.mNumIndices; k++)
				mesh->indices.push_back(face.mIndices[k] + startVertex);
		}
	}

	_meshes.push_back(mesh);
}

void Converter::ReadSkinData()
{
	for (uint32 i = 0; i < _scene->mNumMeshes; i++)
	{
		aiMesh* srcMesh = _scene->mMeshes[i];
		if (srcMesh->HasBones() == false)
			continue;

		std::shared_ptr<asMesh> mesh = _meshes[i];

		std::vector<asBoneWeights> tempVertexBoneWeights;
		tempVertexBoneWeights.resize(mesh->vertices.size());

		for (uint32 b = 0; b < srcMesh->mNumBones; b++)
		{
			aiBone* srcMeshBone = srcMesh->mBones[b];
			uint32 boneIndex = GetBoneIndex(srcMeshBone->mName.C_Str());

			for (uint32 w = 0; w < srcMeshBone->mNumWeights; w++)
			{
				uint32 index = srcMeshBone->mWeights[w].mVertexId;
				float weight = srcMeshBone->mWeights[w].mWeight;
				tempVertexBoneWeights[index].AddWeights(boneIndex, weight);
			}
		}

		for (uint32 v = 0; v < tempVertexBoneWeights.size(); v++)
		{
			tempVertexBoneWeights[v].Normalize();

			asBlendWeight blendWeight = tempVertexBoneWeights[v].GetBlendWeights();
			mesh->vertices[v].blendIndices = blendWeight.indices;
			mesh->vertices[v].blendWeights = blendWeight.weights;
		}
	}
}

void Converter::WriteModelFile(std::wstring finalPath)
{
	auto path = std::filesystem::path(finalPath);
	std::filesystem::create_directory(path.parent_path());

	std::shared_ptr<FileUtils> file = std::make_shared<FileUtils>();
	file->Open(finalPath, FileMode::Write);

	file->Write<uint32>(_bones.size());
	for (std::shared_ptr<asBone>& bone : _bones)
	{
		file->Write<int32>(bone->index);
		file->Write<std::string>(bone->name);
		file->Write<int32>(bone->parent);
		file->Write<Matrix>(bone->transform);
	}

	file->Write<uint32>(_meshes.size());
	for (std::shared_ptr<asMesh>& meshData : _meshes)
	{
		file->Write<std::string>(meshData->name);
		file->Write<int32>(meshData->boneIndex);
		file->Write<std::string>(meshData->materialName);

		file->Write<uint32>(meshData->vertices.size());
		file->Write(&meshData->vertices[0], sizeof(VertexType) * meshData->vertices.size());

		file->Write<uint32>(meshData->indices.size());
		file->Write(&meshData->indices[0], sizeof(uint32) * meshData->indices.size());
	}
}

void Converter::ReadMaterialData()
{
	for (uint32 i = 0; i < _scene->mNumMaterials; i++)
	{
		aiMaterial* srcMaterial = _scene->mMaterials[i];

		std::shared_ptr<asMaterial> material = std::make_shared<asMaterial>();
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
		material->emissive = Color(color.r, color.g, color.b, 1.0f);

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
	std::filesystem::create_directory(path.parent_path());

	std::string folder = path.parent_path().string();

	std::shared_ptr<tinyxml2::XMLDocument> document = std::make_shared<tinyxml2::XMLDocument>();

	tinyxml2::XMLDeclaration* decl = document->NewDeclaration();
	document->LinkEndChild(decl);

	tinyxml2::XMLElement* root = document->NewElement("Materials");
	document->LinkEndChild(root);

	for (std::shared_ptr<asMaterial> material : _materials)
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

		element = document->NewElement("Ambient");
		element->SetAttribute("R", material->ambient.x);
		element->SetAttribute("G", material->ambient.y);
		element->SetAttribute("B", material->ambient.z);
		element->SetAttribute("A", material->ambient.w);
		node->LinkEndChild(element);

		element = document->NewElement("Diffuse");
		element->SetAttribute("R", material->diffuse.x);
		element->SetAttribute("G", material->diffuse.y);
		element->SetAttribute("B", material->diffuse.z);
		element->SetAttribute("A", material->diffuse.w);
		node->LinkEndChild(element);

		element = document->NewElement("Specular");
		element->SetAttribute("R", material->specular.x);
		element->SetAttribute("G", material->specular.y);
		element->SetAttribute("B", material->specular.z);
		element->SetAttribute("A", material->specular.w);
		node->LinkEndChild(element);

		element = document->NewElement("Emissive");
		element->SetAttribute("R", material->emissive.x);
		element->SetAttribute("G", material->emissive.y);
		element->SetAttribute("B", material->emissive.z);
		element->SetAttribute("A", material->emissive.w);
		node->LinkEndChild(element);
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
			std::shared_ptr<FileUtils> file = std::make_shared<FileUtils>();
			file->Open(Utils::ToWString(pathStr), FileMode::Write);
			file->Write(srcTexture->pcData, srcTexture->mWidth);
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
			HRESULT hr = GET_SINGLE(Graphics)->GetDevice()->CreateTexture2D(&desc, &subResource, texture.GetAddressOf());
			CHECK(hr);

			DirectX::ScratchImage img;
			::CaptureTexture(GET_SINGLE(Graphics)->GetDevice().Get(), GET_SINGLE(Graphics)->GetDeviceContext().Get(), texture.Get(), img);

			hr = DirectX::SaveToDDSFile(*img.GetImages(), DirectX::DDS_FLAGS_NONE, Utils::ToWString(fileName).c_str());
			CHECK(hr);
		}
	}
	else
	{
		std::string originStr = (std::filesystem::path(_assetPath) / folderName / file).string();
		Utils::Replace(originStr, "\\", "/");

		std::string pathStr = (std::filesystem::path(saveFolder) / fileName).string();
		Utils::Replace(pathStr, "\\", "/");

		::CopyFileA(originStr.c_str(), pathStr.c_str(), false);
	}

	return fileName;
}

std::shared_ptr<asAnimation> Converter::ReadAnimationData(aiAnimation* srcAnimation)
{
	std::shared_ptr<asAnimation> animation = std::make_shared<asAnimation>();
	animation->name = srcAnimation->mName.C_Str();

	float ticksPerSecond = (srcAnimation->mTicksPerSecond > 0.0)
		? (float)srcAnimation->mTicksPerSecond
		: 30.0f;

	animation->frameRate = ticksPerSecond;

	uint32 maxRotTick = 0;
	for (uint32 i = 0; i < srcAnimation->mNumChannels; i++)
	{
		aiNodeAnim* ch = srcAnimation->mChannels[i];
		for (uint32 k = 0; k < ch->mNumRotationKeys; k++)
		{
			double t = ch->mRotationKeys[k].mTime;
			if (std::isfinite(t) && t >= 0.0)
				maxRotTick = max(maxRotTick, (uint32)t);
		}
	}
	uint32 effectiveDuration = (maxRotTick > 0) ? maxRotTick : (uint32)srcAnimation->mDuration;
	animation->frameCount = effectiveDuration + 1;
	animation->duration = (float)effectiveDuration / ticksPerSecond;

	std::map<std::string, std::shared_ptr<asAnimationNode>> cacheAnimNodes;

	for (uint32 i = 0; i < srcAnimation->mNumChannels; i++)
	{
		aiNodeAnim* srcNode = srcAnimation->mChannels[i];
		std::shared_ptr<asAnimationNode> node = ParseAnimationNode(animation, srcNode);

		std::string channelName = srcNode->mNodeName.C_Str();
		const std::string fbxSuffix = "_$AssimpFbx$_Rotation";
		if (channelName.size() > fbxSuffix.size() &&
			channelName.substr(channelName.size() - fbxSuffix.size()) == fbxSuffix)
		{
			channelName = channelName.substr(0, channelName.size() - fbxSuffix.size());
		}
		cacheAnimNodes[channelName] = node;
	}

	ReadKeyframeData(animation, _scene->mRootNode, cacheAnimNodes);

	return animation;
}

std::shared_ptr<asAnimationNode> Converter::ParseAnimationNode(std::shared_ptr<asAnimation> animation, aiNodeAnim* srcNode)
{
	std::shared_ptr<asAnimationNode> node = std::make_shared<asAnimationNode>();
	node->name = srcNode->mNodeName.C_Str();

	const uint32 frameCount = animation->frameCount;

	auto buildIndexMap = [](uint32 numKeys, auto* keys) -> std::vector<uint32>
		{
			uint32 maxFrame = 0;
			for (uint32 i = 0; i < numKeys; i++)
			{
				double t = keys[i].mTime;
				if (!std::isfinite(t) || t < 0.0) continue;
				maxFrame = max(maxFrame, (uint32)t);
			}

			std::vector<uint32> map(maxFrame + 1, UINT32_MAX);
			for (uint32 i = 0; i < numKeys; i++)
			{
				double t = keys[i].mTime;
				if (!std::isfinite(t) || t < 0.0) continue;
				uint32 f = (uint32)t;
				if (f <= maxFrame && map[f] == UINT32_MAX) map[f] = i;
			}
			return map;
		};

	auto posMap = buildIndexMap(srcNode->mNumPositionKeys, srcNode->mPositionKeys);
	auto rotMap = buildIndexMap(srcNode->mNumRotationKeys, srcNode->mRotationKeys);
	auto scaleMap = buildIndexMap(srcNode->mNumScalingKeys, srcNode->mScalingKeys);

	auto findKey = [](const std::vector<uint32>& map, uint32 frame) -> uint32
		{
			if (frame < map.size() && map[frame] != UINT32_MAX)
				return map[frame];

			int32 f = (int32)std::min(frame, (uint32)map.size() - 1);
			while (f >= 0)
			{
				if (map[f] != UINT32_MAX) return map[f];
				f--;
			}
			return 0;
		};

	auto findNextKey = [](const std::vector<uint32>& map, uint32 frame) -> uint32
		{
			for (uint32 i = frame + 1; i < map.size(); i++)
				if (map[i] != UINT32_MAX) return map[i];
			return UINT32_MAX;
		};

	for (uint32 f = 0; f < frameCount; f++)
	{
		asKeyframeData frameData;
		frameData.time = (float)f / animation->frameRate;

		// --- Position ---
		if (srcNode->mNumPositionKeys > 0)
		{
			uint32 k = findKey(posMap, f);
			uint32 kNextIdx = findNextKey(posMap, f);
			uint32 kNext = (kNextIdx != UINT32_MAX) ? kNextIdx : k;

			aiVector3D p0 = srcNode->mPositionKeys[k].mValue;
			if (kNext != k)
			{
				aiVector3D p1 = srcNode->mPositionKeys[kNext].mValue;
				float t0 = (float)srcNode->mPositionKeys[k].mTime;
				float t1 = (float)srcNode->mPositionKeys[kNext].mTime;
				float factor = (t1 - t0) > 0.f ? ((float)f - t0) / (t1 - t0) : 0.f;
				factor = max(0.f, std::min(1.f, factor));
				p0 = p0 + (p1 - p0) * factor;
			}
			::memcpy_s(&frameData.translation, sizeof(Vec3), &p0, sizeof(aiVector3D));
		}
		else
		{
			frameData.translation = Vec3(0, 0, 0);
		}

		// --- Rotation ---
		if (srcNode->mNumRotationKeys > 0)
		{
			uint32 k = findKey(rotMap, f);
			uint32 kNextIdx = findNextKey(rotMap, f);
			uint32 kNext = (kNextIdx != UINT32_MAX) ? kNextIdx : k;

			aiQuaternion r0 = srcNode->mRotationKeys[k].mValue;
			if (kNext != k)
			{
				aiQuaternion r1 = srcNode->mRotationKeys[kNext].mValue;
				float t0 = (float)srcNode->mRotationKeys[k].mTime;
				float t1 = (float)srcNode->mRotationKeys[kNext].mTime;
				float factor = (t1 - t0) > 0.f ? ((float)f - t0) / (t1 - t0) : 0.f;
				factor = max(0.f, std::min(1.f, factor));
				aiQuaternion interpolated;
				aiQuaternion::Interpolate(interpolated, r0, r1, factor);
				r0 = interpolated;
			}
			frameData.rotation = { r0.x, r0.y, r0.z, r0.w };
		}
		else
		{
			frameData.rotation = { 0, 0, 0, 1 };
		}

		// --- Scale ---
		if (srcNode->mNumScalingKeys > 0)
		{
			uint32 k = findKey(scaleMap, f);
			uint32 kNextIdx = findNextKey(scaleMap, f);
			uint32 kNext = (kNextIdx != UINT32_MAX) ? kNextIdx : k;

			aiVector3D s0 = srcNode->mScalingKeys[k].mValue;
			if (kNext != k)
			{
				aiVector3D s1 = srcNode->mScalingKeys[kNext].mValue;
				float t0 = (float)srcNode->mScalingKeys[k].mTime;
				float t1 = (float)srcNode->mScalingKeys[kNext].mTime;
				float factor = (t1 - t0) > 0.f ? ((float)f - t0) / (t1 - t0) : 0.f;
				factor = max(0.f, std::min(1.f, factor));
				s0 = s0 + (s1 - s0) * factor;
			}
			::memcpy_s(&frameData.scale, sizeof(Vec3), &s0, sizeof(aiVector3D));
		}
		else
		{
			frameData.scale = Vec3(1, 1, 1);
		}

		node->keyframe.push_back(frameData);
	}

	return node;
}

void Converter::ReadKeyframeData(std::shared_ptr<asAnimation> animation, aiNode* srcNode, std::map<std::string, std::shared_ptr<asAnimationNode>>& cache)
{
	std::shared_ptr<asKeyframe> keyframe = std::make_shared<asKeyframe>();
	keyframe->boneName = srcNode->mName.C_Str();

	std::shared_ptr<asAnimationNode> findNode = cache[srcNode->mName.C_Str()];

	for (uint32 i = 0; i < animation->frameCount; i++)
	{
		asKeyframeData frameData;

		if (findNode == nullptr)
		{
			Matrix transform(srcNode->mTransformation[0]);
			transform = transform.Transpose();
			frameData.time = (float)i / animation->frameRate;
			transform.Decompose(OUT frameData.scale, OUT frameData.rotation, OUT frameData.translation);
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

void Converter::WriteAnimationData(std::shared_ptr<asAnimation> animation, std::wstring finalPath)
{
	auto path = std::filesystem::path(finalPath);
	std::filesystem::create_directory(path.parent_path());

	std::shared_ptr<FileUtils> file = std::make_shared<FileUtils>();
	file->Open(finalPath, FileMode::Write);

	file->Write<std::string>(animation->name);
	file->Write<float>(animation->duration);
	file->Write<float>(animation->frameRate);
	file->Write<uint32>(animation->frameCount);

	file->Write<uint32>(animation->keyframes.size());

	for (std::shared_ptr<asKeyframe> keyframe : animation->keyframes)
	{
		file->Write<std::string>(keyframe->boneName);

		file->Write<uint32>(keyframe->transforms.size());
		file->Write(&keyframe->transforms[0], sizeof(asKeyframeData) * keyframe->transforms.size());
	}
}

uint32 Converter::GetBoneIndex(const std::string& name)
{
	for (std::shared_ptr<asBone>& bone : _bones)
	{
		if (bone->name == name)
			return bone->index;
	}

	assert(false);
	return 0;
}