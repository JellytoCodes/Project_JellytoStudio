#pragma once

class Scene;
class Entity;

// ── Actor ─────────────────────────────────────────────────────────────────
// Scene에 배치 가능한 오브젝트의 Engine 레벨 베이스 클래스.
// Entity/Component 시스템 위에 "씬 배치 단위"를 추상화한다.
//
// 배치 위치: Engine/Source/Entity/
// 구체 구현: Client/Source/Main/Actors.h (게임별 Actor)
//
// ItemWindow(Engine)는 Actor 베이스만 알면 되므로
// Engine → Engine 의존으로 깔끔하게 해결됨.
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