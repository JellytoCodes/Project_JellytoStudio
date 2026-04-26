#pragma once

class VertexBuffer;

struct InstancingData
{
    Matrix world;
};

#define MAX_MESH_INSTANCE 10000

class InstancingBuffer
{
public:
    static constexpr uint32 kRingCount = 3;

    explicit InstancingBuffer(bool isDynamic = false);
    ~InstancingBuffer() = default;

    InstancingBuffer(const InstancingBuffer&)            = delete;
    InstancingBuffer& operator=(const InstancingBuffer&) = delete;
    InstancingBuffer(InstancingBuffer&&)                 = default;
    InstancingBuffer& operator=(InstancingBuffer&&)      = default;

    void ClearData();
    void AddData(const InstancingData& data);

    void UploadData();
    void BindBuffer() const;
    void PushData();

    bool   IsUploaded() const { return _uploaded; }
    void   ResetUpload()     { _uploaded = false; }
    uint32 GetCount()  const { return static_cast<uint32>(_data.size()); }

private:
    void CreateBuffers(uint32 maxCount);

    ComPtr<ID3D11Buffer> _ringBuffers[kRingCount];
    uint32               _maxCount    = 0;
    uint32               _frameIndex  = 0;
    uint32               _currentSlot = 0;

    std::vector<InstancingData> _data;

    bool _isDynamic = false;
    bool _dirty     = true;
    bool _uploaded  = false;
};
