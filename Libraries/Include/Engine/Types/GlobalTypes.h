#pragma once

using int8			= __int8;
using int16			= __int16;
using int32			= __int32;
using int64			= __int64;
using uint8			= unsigned __int8;
using uint16		= unsigned __int16;
using uint32		= unsigned __int32;
using uint64		= unsigned __int64;

using Color			= SimpleMath::Color;

using Vec2			= SimpleMath::Vector2;
using Vec3			= SimpleMath::Vector3;
using Vec4			= SimpleMath::Vector4;
using Matrix		= SimpleMath::Matrix;
using Quaternion	= SimpleMath::Quaternion;
using Ray			= SimpleMath::Ray;

struct InstanceID
{
    uint64 resource0 = 0;
    uint64 resource1 = 0;
    uint64 bucket    = 0;

    bool IsValid() const { return resource0 != 0 || resource1 != 0; }

    bool operator==(const InstanceID& other) const
    {
        return resource0 == other.resource0
            && resource1 == other.resource1
            && bucket    == other.bucket;
    }
};

struct TransformData
{
    Matrix world;
    Matrix view;
    Matrix projection;
};
// Layer indices
enum : uint8
{
	Layer_Default = 0,
	Layer_UI      = 1,
};
