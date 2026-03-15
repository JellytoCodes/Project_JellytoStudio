
#include "Framework.h"
#include "Terrain.h"

#include "MeshRenderer.h"
#include "Entity/Entity.h"
#include "Graphics/Graphics.h"
#include "Resource/Mesh.h"

Terrain::Terrain()
	: Super(ComponentType::Terrain)
{

}

Terrain::~Terrain()
{

}

void Terrain::Awake()
{
	
}

void Terrain::Start()
{
	
}

void Terrain::Update()
{
	
}

void Terrain::LateUpdate()
{
	
}

void Terrain::OnDestroy()
{
	
}

void Terrain::Create(float sizeX, float sizeZ, std::shared_ptr<Material> material)
{
    _sizeX = sizeX;
    _sizeZ = sizeZ;

    auto entity = _entity.lock();

    if (entity->GetComponent<Transform>() == nullptr)
        entity->AddComponent(std::make_shared<Transform>());

    if (entity->GetComponent<MeshRenderer>() == nullptr)
        entity->AddComponent(std::make_shared<MeshRenderer>());

    _mesh = std::make_shared<Mesh>();
    _mesh->CreateGrid(Graphics::Get()->GetDevice(), sizeX, sizeZ);

    entity->GetComponent<MeshRenderer>()->SetMesh(_mesh);
    entity->GetComponent<MeshRenderer>()->SetPass(0);
    entity->GetComponent<MeshRenderer>()->SetMaterial(material);
}
