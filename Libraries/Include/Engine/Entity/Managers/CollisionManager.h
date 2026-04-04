#pragma once

class Scene;
class BaseCollider;
class Entity;

class CollisionManager
{
public:
	static void CheckCollision(std::shared_ptr<Scene>& scene);

private:
	static bool Intersects(const std::shared_ptr<Entity>& instigator, const std::shared_ptr<Entity>& target);
};