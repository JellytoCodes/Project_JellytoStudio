
#include "Framework.h"
#include "ResourceManager.h"
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
	if (texture != nullptr)
		return texture;

	if (!std::filesystem::exists(std::filesystem::path(path)))
		return nullptr;

	return Load<Texture>(key, path);
}

void ResourceManager::CreateDefaultMesh()
{
	auto device = GET_SINGLE(Graphics)->GetDevice();
	{
		auto mesh = std::make_shared<Mesh>();
		mesh->CreateQuad(device);
		Add(L"Quad", mesh);
	}
	{
		auto mesh = std::make_shared<Mesh>();
		mesh->CreateCube(device);
		Add(L"Cube", mesh);
	}
	{
		auto mesh = std::make_shared<Mesh>();
		mesh->CreateSphere(device);
		Add(L"Sphere", mesh);
	}
}