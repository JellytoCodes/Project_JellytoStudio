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

	void UploadData();
	void BindBuffer();
	void PushData();  
	bool IsUploaded() const { return _uploaded; }
	void ResetUpload()  { _uploaded = false; }

	uint32 GetCount() const { return static_cast<uint32>(_data.size()); }

	VertexBuffer* GetBuffer() { return _instanceBuffer.get(); }

private:
	void CreateBuffer(uint32 maxCount);

	uint32							_maxCount = 0;

	std::unique_ptr<VertexBuffer>	_instanceBuffer;
	std::vector<InstancingData>		_data;
	bool							_uploaded = false;
};
