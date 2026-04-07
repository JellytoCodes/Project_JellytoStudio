#include "Framework.h"
#include "CollisionManager.h"
#include "Scene/Scene.h"
#include "Entity/Entity.h"
#include "Entity/Components/MonoBehaviour.h"
#include "Entity/Components/Collider/BaseCollider.h"

void CollisionManager::CheckCollision(std::shared_ptr<Scene>& scene)
{
	auto& collidables = scene->GetCollidableEntities();
	if (collidables.empty()) return;

	// unordered_set → random access 불가 → vector로 변환 (C가 작으므로 비용 미미)
	std::vector<std::shared_ptr<Entity>> entities(collidables.begin(), collidables.end());

	for (int32 i = 0; i < (int32)entities.size(); i++)
	{
		for (int32 j = i + 1; j < (int32)entities.size(); j++)
		{
			if (Intersects(entities[i], entities[j]))
			{
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