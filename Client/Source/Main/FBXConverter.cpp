
#include "Core/Framework.h"
#include "Utils/Converter.h"
#include "FBXConverter.h"

void FBXConverter::Init()
{
	std::shared_ptr<Converter> converter = std::make_shared<Converter>();
	converter->ReadAssetFile(L"Character/Ch03.fbx");
	converter->ExportMaterialData(L"Character/Ch03");
	converter->ExportModelData(L"Character/Ch03");
}

void FBXConverter::Update()
{

}

void FBXConverter::Render()
{

}