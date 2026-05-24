#pragma once

class Entity;

enum class CollisionChannel : uint8
{
    None      = 0,
    Default   = 1 << 0,
    Character = 1 << 1,
    Priming   = 1 << 2,
    Mushroom  = 1 << 3,
    Floor     = 1 << 4,

    All = 0xFF
};

inline uint8 operator|(CollisionChannel a, CollisionChannel b)
{
    return static_cast<uint8>(a) | static_cast<uint8>(b);
}
inline uint8 operator|(uint8 a, CollisionChannel b)
{
    return a | static_cast<uint8>(b);
}

inline bool ChannelInMask(CollisionChannel ch, uint8 mask)
{
    return (static_cast<uint8>(ch) & mask) != 0;
}

struct BlockPickHit
{
    bool    valid  = false;
    Entity* entity = nullptr;
    DirectX::SimpleMath::Vector3 normal = DirectX::SimpleMath::Vector3(0.f, 1.f, 0.f);
    float   dist   = FLT_MAX;

    void Reset()
    {
        valid  = false;
        entity = nullptr;
        normal = DirectX::SimpleMath::Vector3(0.f, 1.f, 0.f);
        dist   = FLT_MAX;
    }
};

enum class PlaceFace : uint8
{
    None   = 0,
    Top    = 1 << 0,
    Side   = 1 << 1,
    Bottom = 1 << 2,
    All    = 0xFF
};
inline uint8 operator|(PlaceFace a, PlaceFace b)
{
    return static_cast<uint8>(a) | static_cast<uint8>(b);
}
inline bool FaceAllowed(PlaceFace face, uint8 mask)
{
    return (static_cast<uint8>(face) & mask) != 0;
}

inline PlaceFace NormalToFace(const DirectX::SimpleMath::Vector3& n)
{
    if (n.y >  0.7f) return PlaceFace::Top;
    if (n.y < -0.7f) return PlaceFace::Bottom;
    return PlaceFace::Side;
}
