#pragma once
#include "Entity/Components/Collider/CollisionChannel.h"
#include "Entity/Components/Collider/AABBCollider.h"
#include "Entity/Entity.h"

inline void UpdatePickHit(BlockPickHit& hit, Entity* entity, const Vec3& normal, float dist)
{
    if (dist >= hit.dist) return;

    hit.valid  = true;
    hit.entity = entity;
    hit.normal = normal;
    hit.dist   = dist;
}

inline void UpdateMatchingPickHits(uint8 queryMask, AABBCollider* aabb, Entity* entity,
                                   const Vec3& normal, float dist,
                                   BlockPickHit& priming, BlockPickHit& floor, BlockPickHit& mushroom)
{
    if ((queryMask & static_cast<uint8>(CollisionChannel::Priming)) && aabb->CanBePickedBy(CollisionChannel::Priming))
        UpdatePickHit(priming, entity, normal, dist);

    if ((queryMask & static_cast<uint8>(CollisionChannel::Floor)) && aabb->CanBePickedBy(CollisionChannel::Floor))
        UpdatePickHit(floor, entity, normal, dist);

    if ((queryMask & static_cast<uint8>(CollisionChannel::Mushroom)) && aabb->CanBePickedBy(CollisionChannel::Mushroom))
        UpdatePickHit(mushroom, entity, normal, dist);
}