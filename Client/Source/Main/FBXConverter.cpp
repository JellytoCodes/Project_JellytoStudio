
#include "Core/Framework.h"
#include "Utils/Converter.h"
#include "FBXConverter.h"

void FBXConverter::Init()
{
	std::shared_ptr<Converter> converter = std::make_shared<Converter>();
	converter->ReadAssetFile(L"Separate/Pinguin_001.fbx");
	converter->ExportMaterialData(L"Separate/Pinguin_001");
	converter->ExportModelData(L"Separate/Pinguin_001");
}

void FBXConverter::Update()
{

}

void FBXConverter::Render()
{

}