
#include "pch.h"
#include "Utils/Converter.h"
#include "FBXConverter.h"

void FBXConverter::Init()
{
	{
		std::shared_ptr<Converter> converter = std::make_shared<Converter>();
		converter->ReadAssetFile(L"Character/Ch03.fbx");
		converter->ExportMaterialData(L"Character/Ch03");
		converter->ExportModelData(L"Character/Ch03");
	}
	{
		std::shared_ptr<Converter> converter = std::make_shared<Converter>();
		converter->ReadAssetFile(L"Character/Capoeira.fbx");
		converter->ExportAnimationData(L"Character/Capoeira");
		converter->ExportAnimationCSV(L"Character/Capoeira");
	}
}

void FBXConverter::Update()
{

}

void FBXConverter::Render()
{

}