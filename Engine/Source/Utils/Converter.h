#pragma once

class Converter
{
public:
    Converter();
    ~Converter();

    void SetAssetPath(const std::wstring& p) { _assetPath = p; }
    void SetModelPath(const std::wstring& p) { _modelPath = p; }
    void SetTexturePath(const std::wstring& p) { _texturePath = p; }

    void ReadAssetFile(std::wstring file);
    void ReadAssetFileAbsolute(const std::wstring& absolutePath);

    void ExportModelData(std::wstring savePath);
    void ExportMaterialData(std::wstring savePath);
    void ExportAnimationData(std::wstring savePath, uint32 index = 0);

    void ExportCSV(std::wstring savePath);
    void ExportAnimationCSV(std::wstring savePath);

    bool HasAnimation() const
    {
        return _scene != nullptr && _scene->mNumAnimations > 0;
    }

    static std::string NormalizeBoneName(const std::string& raw);

private:
    void ResetState();
    void BuildBoneIndexMap();

    void ReadModelData(aiNode* node, int32 index, int32 parent);
    void ReadMeshData(aiNode* node, int32 bone);
    void ReadSkinData();
    void WriteModelFile(std::wstring finalPath);

    void ReadMaterialData();
    void WriteMaterialData(std::wstring finalPath);
    std::string WriteTexture(std::string saveFolder, std::string file);

    std::shared_ptr<asAnimation>     ReadAnimationData(aiAnimation* srcAnimation);
    std::shared_ptr<asAnimationNode> ParseAnimationNode(
        std::shared_ptr<asAnimation> animation,
        aiNodeAnim* srcNode);

    void ReadKeyframeData(
        std::shared_ptr<asAnimation> animation,
        aiNode* srcNode,
        std::unordered_map<std::string, std::shared_ptr<asAnimationNode>>& cache);

    void WriteAnimationData(std::shared_ptr<asAnimation> animation,
        std::wstring finalPath);

    uint32 GetBoneIndex(const std::string& name);

private:
    std::shared_ptr<Assimp::Importer> _importer;
    const aiScene* _scene = nullptr;

    std::wstring _assetPath = L"../Resources/Assets/";
    std::wstring _modelPath = L"../Resources/Models/";
    std::wstring _texturePath = L"../Resources/Textures/";

    std::vector<std::shared_ptr<asBone>>     _bones;
    std::vector<std::shared_ptr<asMesh>>     _meshes;
    std::vector<std::shared_ptr<asMaterial>> _materials;

    std::unordered_map<std::string, uint32> _boneIndexMap;
};
