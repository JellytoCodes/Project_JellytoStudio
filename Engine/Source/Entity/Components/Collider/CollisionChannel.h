#pragma once

// ── CollisionChannel ──────────────────────────────────────────────────────
// 언리얼/유니티의 Collision Channel과 동일한 개념
// 비트 플래그로 복수 채널을 마스크로 조합 가능
//
// 사용 예:
//   collider->SetOwnChannel(CollisionChannel::Priming);
//   collider->SetPickableMask(Ch::Priming | Ch::Floor);
//     → Priming 채널 쿼리와 Floor 채널 쿼리에 의해 피킹됨
//
// 배치 규칙:
//   Priming: pickable = Priming | Floor   → 어디든 배치
//            face     = Top | Side        → 상면/측면
//   Mushroom: pickable = Priming          → Priming 위에만
//             face     = Top              → 상면만

enum class CollisionChannel : uint32
{
    None      = 0,
    Default   = 1 << 0,   // 기본값 (미지정)
    Character = 1 << 1,   // 캐릭터
    Priming   = 1 << 2,   // Priming 계열 블록
    Mushroom  = 1 << 3,   // Mushroom 계열 오브젝트
    Floor     = 1 << 4,   // 바닥/Ground plane

    All = 0xFFFFFFFF
};

// 비트 OR 연산자 (채널 마스크 조합용)
inline uint32 operator|(CollisionChannel a, CollisionChannel b)
{
    return static_cast<uint32>(a) | static_cast<uint32>(b);
}
inline uint32 operator|(uint32 a, CollisionChannel b)
{
    return a | static_cast<uint32>(b);
}

// 채널이 마스크에 포함되는지 확인
inline bool ChannelInMask(CollisionChannel ch, uint32 mask)
{
    return (static_cast<uint32>(ch) & mask) != 0;
}

// 배치 가능 면 플래그
enum class PlaceFace : uint8
{
    None   = 0,
    Top    = 1 << 0,   // 상면 (HitNormal.y > 0.7)
    Side   = 1 << 1,   // 측면 (|HitNormal.y| < 0.7)
    Bottom = 1 << 2,   // 하면 (HitNormal.y < -0.7)
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

// 히트 노말로부터 면 타입 판별
inline PlaceFace NormalToFace(const DirectX::SimpleMath::Vector3& n)
{
    if (n.y >  0.7f) return PlaceFace::Top;
    if (n.y < -0.7f) return PlaceFace::Bottom;
    return PlaceFace::Side;
}