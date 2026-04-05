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
	InstancingBuffer();
	~InstancingBuffer();

	void ClearData();
	void AddData(const InstancingData& data);

	// UploadData: Map → memcpy → Unmap (1회만 호출)
	// BindBuffer: IASetVertexBuffers slot1 (서브메시 루프마다 호출)
	// PushData:  UploadData + BindBuffer 통합 (하위호환, 단일 서브메시용)
	void UploadData();  // GPU 버퍼에 데이터 업로드 (Map/Unmap)
	void BindBuffer();  // IA slot1에 인스턴스 버퍼 바인딩
	void PushData();    // UploadData + BindBuffer (기존 호환)

	uint32 GetCount() const { return static_cast<uint32>(_data.size()); }
	std::shared_ptr<VertexBuffer> GetBuffer() { return _instanceBuffer; }

private:
	void CreateBuffer(const uint32 maxCount);

private:
	uint32 _maxCount = 0;
	std::shared_ptr<VertexBuffer> _instanceBuffer;
	std::vector<InstancingData> _data;
};