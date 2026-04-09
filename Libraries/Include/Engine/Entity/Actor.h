#pragma once

class Scene;
class Entity;

class Actor
{
public:
	Actor() = default;
	virtual ~Actor() = default;

	virtual std::wstring GetActorName() const = 0;

	bool Spawn(Scene* scene);

	void Despawn(Scene* scene);

	bool    IsSpawned() const { return _entity != nullptr; }

	Entity* GetEntity() const { return _entity; }

protected:
	virtual void BuildEntity() = 0;

	Entity* _entity = nullptr;
};
