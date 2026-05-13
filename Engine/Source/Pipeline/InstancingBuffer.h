#pragma once

class VertexBuffer;

struct InstancingData
{
    Matrix   world;
    uint32   materialIndex;
    uint32   _instPad[3];
};

#define MAX_MESH_INSTANCE 10000

class InstancingBuffer
{
public:
    static constexpr uint32 kRingCount = 3;

    explicit InstancingBuffer(bool isDynamic = false);
    ~InstancingBuffer() = default;

    InstancingBuffer(const InstancingBuffer&) = delete;
    InstancingBuffer& operator=(const InstancingBuffer&) = delete;
    InstancingBuffer(InstancingBuffer&&) = default;
    InstancingBuffer& operator=(InstancingBuffer&&) = default;

    void   ClearData();
    void   AddData(const InstancingData& data);

    void   UploadData();
    void   BindBuffer() const;
    void   PushData();

    bool   IsUploaded() const { return _uploaded; }
    void   ResetUpload() { _uploaded = false; }
    uint32 GetCount()   const { return static_cast<uint32>(_data.size()); }

    bool   IsDynamic()  const { return _isDynamic; }

    void   PromoteToDynamic();

private:
    void CreateBuffers(uint32 maxCount);

    // 정적 버퍼 전용 — 동적 버퍼는 DynamicInstancePool 공유 버퍼를 사용하므로
    // _ringBuffers[0] 만 할당하고 [1],[2] 는 비어있다.
    ComPtr<ID3D11Buffer>        _ringBuffers[kRingCount];
    uint32                      _maxCount = 0;
    uint32                      _frameIndex = 0;
    uint32                      _currentSlot = 0;

    // 동적 버퍼가 DynamicInstancePool::Append() 에서 받은 element offset.
    // BindBuffer() 가 DynamicInstancePool::BindSlice(offset) 을 호출할 때 사용.
    uint32                      _poolElementOffset = 0;

    std::vector<InstancingData> _data;

    bool _isDynamic = false;
    bool _dirty = true;
    bool _uploaded = false;
};