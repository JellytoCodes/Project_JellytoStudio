#pragma once
#include "Entity/Actor.h"   // Engine Actor 베이스

// ── 구체 Actor 선언 ───────────────────────────────────────────────────────
// 위치: Client/Source/Main/
// Engine의 Actor 베이스를 상속, 게임별 컴포넌트 구성 담당

class SkySphereActor : public Actor
{
public:
	std::wstring GetActorName() const override { return L"SkySphere"; }
protected:
	void BuildEntity() override;
};

class FloorActor : public Actor
{
public:
	std::wstring GetActorName() const override { return L"Floor"; }
protected:
	void BuildEntity() override;
};

class CubeActor : public Actor
{
public:
	std::wstring GetActorName() const override { return L"Cube"; }
protected:
	void BuildEntity() override;
};

class SphereActor : public Actor
{
public:
	std::wstring GetActorName() const override { return L"Sphere"; }
protected:
	void BuildEntity() override;
};

class CharacterActor : public Actor
{
public:
	std::wstring GetActorName() const override { return L"Character"; }
protected:
	void BuildEntity() override;
};

class LightActor : public Actor
{
public:
	std::wstring GetActorName() const override { return L"DirectionalLight"; }
protected:
	void BuildEntity() override;
};