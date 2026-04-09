
#include "Framework.h"
#include "ModelAnimation.h"

ModelKeyframe* ModelAnimation::GetKeyframe(const std::wstring& name)
{
	auto findIt = keyframes.find(name);
	if (findIt == keyframes.end()) return nullptr;

	return findIt->second.get();
}
