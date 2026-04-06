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

	void ClearData();                    // 데이터 초기화 (dirty 시에만 호출)
	void AddData(const InstancingData& data); // 데이터 추가 (dirty 시에만 호출)

	// ── GPU 업로드/바인딩 분리 ───────────────────────────────────────
	// UploadData : Map→memcpy→Unmap, _uploaded = true 표시 (dirty 시 1회)
	// BindBuffer : IASetVertexBuffers slot1 (매프레임 서브메시 루프마다)
	// PushData   : UploadData + BindBuffer 통합 (MeshRenderer 등 동적용)
	// IsUploaded : 이미 올라간 데이터인지 (매프레임 재업로드 방지)
	void UploadData();          // Map/Unmap — dirty 시 1회만
	void BindBuffer();          // IASetVertexBuffers slot1
	void PushData();            // UploadData + BindBuffer (동적 렌더러용)
	bool IsUploaded() const { return _uploaded; }
	void ResetUpload() { _uploaded = false; } // dirty 시 재업로드 허용

	uint32 GetCount() const { return static_cast<uint32>(_data.size()); }
	std::shared_ptr<VertexBuffer> GetBuffer() { return _instanceBuffer; }

private:
	void CreateBuffer(const uint32 maxCount);

private:
	uint32 _maxCount = 0;
	std::shared_ptr<VertexBuffer> _instanceBuffer;
	std::vector<InstancingData> _data;
	bool   _uploaded = false; // UploadData 완료 여부 — dirty 재설정 시 false
};