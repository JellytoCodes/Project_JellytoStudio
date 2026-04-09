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
	// 1. 이미 캐시에 있으면 바로 반환 (참조 카운트 +1)
	std::shared_ptr<Texture> texture = Get<Texture>(key);
	if (texture != nullptr)
		return texture;

	// 2. 파일이 존재하는지 확인
	if (!std::filesystem::exists(std::filesystem::path(path)))
		return nullptr;

	// 3. 없으면 Load 함수 내부에서 생성, 로드, 캐시 등록을 다 해주고 반환합니다.
	return Load<Texture>(key, path);
}

void ResourceManager::CreateDefaultMesh()
{
	{
		auto mesh = std::make_shared<Mesh>();
		mesh->CreateQuad(Graphics::Get()->GetDevice());
		Add(L"Quad", mesh);
	}
	{
		auto mesh = std::make_shared<Mesh>();
		mesh->CreateCube(Graphics::Get()->GetDevice());
		Add(L"Cube", mesh);
	}
	{
		auto mesh = std::make_shared<Mesh>();
		mesh->CreateSphere(Graphics::Get()->GetDevice());
		Add(L"Sphere", mesh);
	}
}