
#include "Framework.h"
#include "ModelRenderer.h"
#include "Model.h"
#include "Resource/Material.h"

ModelRenderer::ModelRenderer(std::shared_ptr<Shader> shader)
	: Super(ComponentType::ModelRenderer)
{

}

ModelRenderer::~ModelRenderer()
{

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
