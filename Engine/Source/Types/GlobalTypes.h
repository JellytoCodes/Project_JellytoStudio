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

using InstanceID = std::pair<uint64, uint64>;

struct TransformData
{
    Matrix world;
    Matrix view;
    Matrix projection;
};