#pragma once

class Viewport
{
public :
	Viewport();
	Viewport(float width, float height, float x= 0, float y = 0, float minDepth = 0, float maxDepth = 1);
	~Viewport();

	void RSSetViewport(const ComPtr<ID3D11DeviceContext>& deviceContext);
	void Set(float width, float height, float x= 0, float y = 0, float minDepth = 0, float maxDepth = 1);

	float GetWidth() const		{ return _vp.Width; }
	float GetHeight() const		{ return _vp.Height; }

	Vec3 Project(const Vec3& pos, const Matrix& W, const Matrix& V, const Matrix& P);
	Vec3 UnProject(const Vec3& pos, const Matrix& W, const Matrix& V, const Matrix& P);

private :
	D3D11_VIEWPORT _vp;
};