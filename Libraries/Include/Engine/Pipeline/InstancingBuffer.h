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
	static constexpr uint32 kTierSmall = 64;
	static constexpr uint32 kTierMedium = 512;
	static constexpr uint32 kTierLarge = 4096;
	static constexpr uint32 kTierMax = MAX_MESH_INSTANCE;

	explicit InstancingBuffer(bool isDynamic = false);
	~InstancingBuffer() = default;

	InstancingBuffer(const InstancingBuffer&) = delete;
	InstancingBuffer& operator=(const InstancingBuffer&) = delete;
	InstancingBuffer(InstancingBuffer&&) = default;
	InstancingBuffer& operator=(InstancingBuffer&&) = default;

	void   ClearData();
	void   AddData(const InstancingData& data);
	void   SetData(const InstancingData* data, uint32 count);

	void   UploadData();
	void   BindBuffer() const;
	void   PushData();

	bool   IsUploaded() const { return _uploaded; }
	void   ResetUpload() { _uploaded = false; }
	uint32 GetCount()   const { return static_cast<uint32>(_data.size()); }

	bool   IsDynamic()  const { return _isDynamic; }

	void   PromoteToDynamic();

private:
	static uint32 NextTier(uint32 count);
	void          AllocStaticBuffer(uint32 maxCount);

	ComPtr<ID3D11Buffer>        _ringBuffers[kRingCount];
	uint32                      _maxCount = 0;
	uint32                      _frameIndex = 0;
	uint32                      _currentSlot = 0;
	uint32                      _poolElementOffset = 0;

	std::vector<InstancingData> _data;

	bool _isDynamic = false;
	bool _dirty = true;
	bool _uploaded = false;
};