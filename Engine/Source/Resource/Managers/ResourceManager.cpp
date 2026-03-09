
#include "Framework.h"
#include "ResourceManager.h"
#include <filesystem>

#include "Graphics/Graphics.h"
#include "Resource/Texture.h"
#include "Resource/Mesh.h"
void ResourceManager::Init()
{
	CreateDefaultMesh();
}

std::shared_ptr<Texture> ResourceManager::GetOrAddTexture(const std::wstring& key, const std::wstring& path)
{
	std::shared_ptr<Texture> texture = Get<Texture>(key);

	if (std::filesystem::exists(std::filesystem::path(path)) == false) return nullptr;

	texture = Load<Texture>(key, path);

	if (texture == nullptr)
	{
		texture = std::make_shared<Texture>();
		texture->Load(path);
		Add(key, texture);
	}

	return texture;
}

void ResourceManager::CreateDefaultMesh()
{
	{
		std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
		mesh->CreateQuad(Graphics::Get()->GetDevice());
		Add(L"Quad", mesh);
	}
	{
		std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
		mesh->CreateCube(Graphics::Get()->GetDevice());
		Add(L"Cube", mesh);
	}
	{
		std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
		mesh->CreateSphere(Graphics::Get()->GetDevice());
		Add(L"Sphere", mesh);
	}
}

