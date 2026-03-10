
#include "Framework.h"
#include "ModelRenderer.h"
#include "Model.h"
#include "Resource/Material.h"
#include "Entity/Components/Transform.h"
#include "Scene/SceneManager.h"
#include "Entity/Components/Camera.h"

ModelRenderer::ModelRenderer(std::shared_ptr<Shader> shader)
	: Super(ComponentType::ModelRenderer), _shader(shader)
{

}

ModelRenderer::~ModelRenderer()
{

}

void ModelRenderer::Awake()
{

}

void ModelRenderer::Start()
{
	auto device = Graphics::Get()->GetDevice();

	_constantBuffer = std::make_shared<ConstantBuffer<TransformData>>();
	_constantBuffer->Create(device);

}

void ModelRenderer::Render()
{
    if (_constantBuffer == nullptr || _constantBuffer->GetComPtr() == nullptr)
        return;

    if (_model == nullptr)
        return;

    auto deviceContext = Graphics::Get()->GetDeviceContext();
	
	auto world = GetTransform()->GetWorldMatrix();
    
    TransformData data;
    data.world = GetTransform()->GetWorldMatrix().Transpose();

    auto camera = GET_SINGLE(SceneManager)->GetCurrentScene()->GetMainCamera();
    data.view = camera->GetViewMatrix().Transpose();
    data.projection = camera->GetProjectionMatrix().Transpose();

    _constantBuffer->CopyData(deviceContext, data);

    auto bufferPtr = _constantBuffer->GetComPtr();
    deviceContext->VSSetConstantBuffers(0, 1, bufferPtr.GetAddressOf());
    deviceContext->PSSetConstantBuffers(0, 1, bufferPtr.GetAddressOf());

    for (auto& mesh : _model->GetMeshes())
    {
        mesh->Render();
    }
}

void ModelRenderer::SetModel(std::shared_ptr<Model> model)
{
	_model = model;

	const auto& materials = _model->GetMaterials();
	for (auto& material : materials)
	{
		material->SetShader(_shader);
	}
}
