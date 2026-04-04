#pragma once

struct ModelKeyframeData
{
	float			time;
	Vec3			scale;
	Quaternion		rotation;
	Vec3			translation;
};

struct ModelKeyframe
{
	std::wstring						boneName;
	std::vector<ModelKeyframeData>		transforms;
};

class ModelAnimation
{

public :
	std::shared_ptr<ModelKeyframe> GetKeyframe(const std::wstring& name);

	std::wstring		name;
	float				duration = 0.f;
	float				frameRate = 0.f;
	uint32				frameCount = 0;

	std::unordered_map<std::wstring, std::shared_ptr<ModelKeyframe>> keyframes;
};