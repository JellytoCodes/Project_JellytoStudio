#pragma once
#include "Entity/Actor.h"

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