#pragma once

class VertexBuffer;

struct InstancingData
{
	Matrix world;
};

#define MAX_MESH_INSTANCE 500

class InstancingBuffer
{
public:
	InstancingBuffer();
	~InstancingBuffer();

	void ClearData();
	void AddData(const InstancingData& data);
	void PushData();

	uint32 GetCount() const						{ return static_cast<uint32>(_data.size()); }
	std::shared_ptr<VertexBuffer> GetBuffer()	{ return _instanceBuffer; }

private:
	void CreateBuffer(const uint32 maxCount = MAX_MESH_INSTANCE);

	uint64							_instanceID = 0;
	uint32							_maxCount = 0;

	std::shared_ptr<VertexBuffer>	_instanceBuffer;
	std::vector<InstancingData>		_data;
};