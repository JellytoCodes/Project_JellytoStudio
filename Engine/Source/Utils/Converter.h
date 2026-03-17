#pragma once
class Converter
{
public:
	Converter();
	~Converter();

	// ── 경로 동적 설정 (SubWindow FBX 컨버터 패널에서 사용) ──────
	void SetAssetPath(const std::wstring& p)   { _assetPath   = p; }
	void SetModelPath(const std::wstring& p)   { _modelPath   = p; }
	void SetTexturePath(const std::wstring& p) { _texturePath = p; }

	// 파일 읽기
	void ReadAssetFile(std::wstring file);              // _assetPath + file 조합
	void ReadAssetFileAbsolute(const std::wstring& absolutePath); // 절대경로 직접

	void ExportModelData(std::wstring savePath);
	void ExportMaterialData(std::wstring savePath);
	void ExportAnimationData(std::wstring savePath, uint32 index = 0);
	void ExportCSV(std::wstring savePath);
	void ExportAnimationCSV(std::wstring savePath);

	// 로드된 FBX가 애니메이션인지 판별
	bool HasAnimation() const
	{
		return _scene != nullptr && _scene->mNumAnimations > 0;
	}

private:
	void		ReadModelData(aiNode* node, int32 index, int32 parent);
	void		ReadMeshData(aiNode* node, int32 bone);
	void		ReadSkinData();
	void		WriteModelFile(std::wstring finalPath);

	void		ReadMaterialData();
	void		WriteMaterialData(std::wstring finalPath);
	std::string	WriteTexture(std::string saveFolder, std::string file);

	std::shared_ptr<asAnimation>		ReadAnimationData(aiAnimation* srcAnimation);
	std::shared_ptr<asAnimationNode>	ParseAnimationNode(std::shared_ptr<asAnimation> animation, aiNodeAnim* srcNode);
	void								ReadKeyframeData(std::shared_ptr<asAnimation> animation, aiNode* srcNode, std::map<std::string, std::shared_ptr<asAnimationNode>>& cache);
	void								WriteAnimationData(std::shared_ptr<asAnimation> animation, std::wstring finalPath);

	uint32 GetBoneIndex(const std::string& name);

	std::shared_ptr<Assimp::Importer>	_importer;
	const aiScene*						_scene = nullptr;

	std::wstring _assetPath   = L"../Resources/Assets/";
	std::wstring _modelPath   = L"../Resources/Models/";
	std::wstring _texturePath = L"../Resources/Textures/";

	std::vector<std::shared_ptr<asBone>>		_bones;
	std::vector<std::shared_ptr<asMesh>>		_meshes;
	std::vector<std::shared_ptr<asMaterial>>	_materials;
};