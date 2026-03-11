#pragma once

using VertexType = VertexTextureNormalTangentBlendData;

struct asBone
{
	std::string		name;

	int32		index = -1;
	int32		parent = -1;

	Matrix		transform;
};

struct asMesh
{
	std::string					name;

	aiMesh* mesh;
	std::vector<VertexType>		vertices;
	std::vector<uint32>			indices;

	int32						boneIndex;
	std::string					materialName;
};

struct asMaterial
{
	std::string					name;

	Color						ambient;
	Color						diffuse;
	Color						specular;
	Color						emissive;

	std::string					diffuseFile;
	std::string					specularFile;
	std::string					normalFile;
};

// Animation
struct asBlendWeight
{
	void Set(uint32 index, uint32 boneIndex, float weight)
	{
		float i = static_cast<float>(boneIndex);
		float w = weight;

		switch (index)
		{
		case 0: 
			indices.x = i; weights.x = w; 
			break;
		case 1: 
			indices.y = i; weights.y = w; 
			break;
		case 2: 
			indices.z = i; weights.z = w; 
			break;
		case 3: 
			indices.w = i; weights.w = w; 
			break;
		}
	}

	Vec4		indices = Vec4(0.f, 0.f, 0.f, 0.f);
	Vec4		weights = Vec4(0.f, 0.f, 0.f, 0.f);
};

// 정점마다 -> (관절 번호, 가중치)
struct asBoneWeights
{
	void AddWeights(uint32 boneIndex, float weight)
	{
		if (weight <= 0.f) return;

		auto findIt = std::find_if(boneWeights.begin(), boneWeights.end(),
			[weight](const Pair& p) { return weight > p.second; });

		boneWeights.insert(findIt, Pair(boneIndex, weight));
	}

	asBlendWeight GetBlendWeights()
	{
		asBlendWeight blendWeights;

		for (uint32 i = 0; i < boneWeights.size(); i++)
		{
			if (i >= 4) break;

			blendWeights.Set(i, boneWeights[i].first, boneWeights[i].second);
		}
		return blendWeights;
	}

	void Normalize()
	{
		if (boneWeights.size() >= 4) 
			boneWeights.resize(4);

		float totalWeight = 0.f;

		for (const auto& item : boneWeights) 
			totalWeight += item.second;

		for (auto& item : boneWeights) 
			item.second /= totalWeight;
	}

	using Pair = std::pair<int32, float>;
	std::vector<Pair> boneWeights;
};

struct asKeyframeData
{
	float			time;
	Vec3			scale;
	Quaternion		rotation;
	Vec3			translation;
};

struct asKeyframe
{
	std::string										boneName;
	std::vector<asKeyframeData>						transforms;
};

struct asAnimation
{
	std::string										name;
	uint32											frameCount;
	float											frameRate;
	float											duration;
	std::vector<std::shared_ptr<asKeyframe>>		keyframes;
};

// Cache
struct asAnimationNode
{
	aiString name;
	std::vector<asKeyframeData> keyframe;
};