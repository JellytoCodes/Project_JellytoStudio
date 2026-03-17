
#include "Framework.h"
#include "CollisionManager.h"
#include "Scene/Scene.h"
#include "Entity/Entity.h"
#include "Entity/Components/MonoBehaviour.h"
#include "Entity/Components/Collider/BaseCollider.h"

void CollisionManager::CheckCollision(std::shared_ptr<Scene>& scene)
{
	std::vector<std::shared_ptr<Entity>> entities(
		scene->GetEntities().begin(),
		scene->GetEntities().end()
	);

	for (int32 i = 0; i < entities.size(); i++)
	{
		for (int32 j = i + 1; j < entities.size(); j++)
		{
			if (Intersects(entities[i], entities[j]))
			{
				// 충돌 시 양쪽 Entity의 모든 Script에 OnCollision 호출
				entities[i]->OnCollision(entities[j]);
				entities[j]->OnCollision(entities[i]);
			}
		}
	}
}

bool CollisionManager::Intersects(const std::shared_ptr<Entity>& instigator, const std::shared_ptr<Entity>& target)
{
	auto colliderA = instigator->GetComponent<BaseCollider>();
	auto colliderB = target->GetComponent<BaseCollider>();

	if (colliderA == nullptr || colliderB == nullptr)
		return false;

	return colliderA->Intersects(colliderB);
}