#pragma once

class Scene;
class Entity;

class Actor
{
public:
	Actor() = default;
	virtual ~Actor() = default;

	// 카탈로그/목록에 표시할 이름
	virtual std::wstring GetActorName() const = 0;

	// 씬에 스폰 (이미 스폰된 경우 false 반환)
	bool Spawn(std::shared_ptr<Scene> scene);

	// 씬에서 제거 및 Entity 해제
	void Despawn(std::shared_ptr<Scene> scene);

	bool IsSpawned() const { return _entity != nullptr; }
	std::shared_ptr<Entity> GetEntity() const { return _entity; }

protected:
	// 하위 클래스에서 오버라이드 — _entity에 컴포넌트를 붙이는 작업
	virtual void BuildEntity() = 0;

	std::shared_ptr<Entity> _entity;
};