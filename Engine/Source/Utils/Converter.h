#pragma once

class Converter
{
public:
    Converter();
    ~Converter();

    // ── 경로 설정 ────────────────────────────────────────────────────────────
    void SetAssetPath(const std::wstring& p) { _assetPath = p; }
    void SetModelPath(const std::wstring& p) { _modelPath = p; }
    void SetTexturePath(const std::wstring& p) { _texturePath = p; }

    // ── FBX 로드 ─────────────────────────────────────────────────────────────
    void ReadAssetFile(std::wstring file);
    void ReadAssetFileAbsolute(const std::wstring& absolutePath);

    // ── 내보내기 ─────────────────────────────────────────────────────────────
    void ExportModelData(std::wstring savePath);
    void ExportMaterialData(std::wstring savePath);
    void ExportAnimationData(std::wstring savePath, uint32 index = 0);

    // ── 디버그용 CSV 덤프 ─────────────────────────────────────────────────────
    void ExportCSV(std::wstring savePath);
    void ExportAnimationCSV(std::wstring savePath);

    bool HasAnimation() const
    {
        return _scene != nullptr && _scene->mNumAnimations > 0;
    }

    // ─────────────────────────────────────────────────────────────────────────
    static std::string NormalizeBoneName(const std::string& raw);

private:
    // ── [Phase 1] 내부 헬퍼 ──────────────────────────────────────────────────

    // 동일 Converter 인스턴스 재사용 시 이전 상태 초기화
    void ResetState();

    // ReadModelData() 완료 후 O(1) 인덱스 맵 구축
    void BuildBoneIndexMap();

    // ── 파싱 파이프라인 ───────────────────────────────────────────────────────
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

    // [Phase 1] cache 시그니처: std::map -> std::unordered_map
    void ReadKeyframeData(
        std::shared_ptr<asAnimation> animation,
        aiNode* srcNode,
        std::unordered_map<std::string, std::shared_ptr<asAnimationNode>>& cache);

    void WriteAnimationData(std::shared_ptr<asAnimation> animation,
        std::wstring finalPath);

    // [Phase 1] O(n) assert -> O(1) + Graceful Degradation
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

    // [Phase 1] O(1) bone 이름 -> 인덱스 맵
    //  key   : raw 이름 & normalized 이름 양방향 등록
    //  value : _bones 배열 내 인덱스
    std::unordered_map<std::string, uint32> _boneIndexMap;
};