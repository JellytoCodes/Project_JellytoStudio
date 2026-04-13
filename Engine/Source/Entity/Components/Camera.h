#pragma once
#include "Entity/Components/Component.h"

enum class ProjectionType
{
    Perspective,
    Orthographic,
};

class Camera : public Component
{
    using Super = Component;
public:
    Camera();
    virtual ~Camera();

    virtual void Awake()      override;
    virtual void Start()      override;
    virtual void Update()     override;
    virtual void LateUpdate() override;
    virtual void OnDestroy()  override;

    void SetProjectionType(ProjectionType type) { _type = type; }
    ProjectionType GetProjectionType() const { return _type; }

    void UpdateMatrix();

    void SetNear(float value) { _near = value; }
    void SetFar(float value) { _far = value; }
    void SetFOV(float value) { _fov = value; }
    void SetWidth(float value) { _width = value; }
    void SetHeight(float value) { _height = value; }

    Matrix& GetViewMatrix() { return _matView; }
    Matrix& GetProjectionMatrix() { return _matProjection; }
    float   GetWidth()  const { return _width; }
    float   GetHeight() const { return _height; }

    void SortEntities();
    void RenderForward();

    // ★ Scene 구조 변경(엔티티 추가/삭제) 시 외부에서 호출
    void SetSortDirty() { _sortDirty = true; }

    void SetCullingMaskLayerOnOff(uint8 layer, bool on)
    {
        if (on) _cullingMask |= (1 << layer);
        else    _cullingMask &= ~(1 << layer);
    }
    void SetCullingMaskAll() { SetCullingMask(UINT32_MAX); }
    void SetCullingMask(uint32 mask) { _cullingMask = mask; }
    bool IsCulled(uint8 layer) const { return (_cullingMask & (1 << layer)) != 0; }

    static Matrix S_MatView;
    static Matrix S_MatProjection;

private:
    ProjectionType _type = ProjectionType::Perspective;
    Matrix         _matView = Matrix::Identity;
    Matrix         _matProjection = Matrix::Identity;

    float  _near = 1.f;
    float  _far = 1000.f;
    float  _fov = XM_PI / 4.f;
    float  _width = 0;
    float  _height = 0;
    uint32 _cullingMask = 0;

    std::vector<Entity*> _vecForward;

    bool _sortDirty = true;
    // ★ 이전 프레임의 카메라 위치/회전을 저장해 실제로 움직였을 때만 SortDirty 발동
    Vec3  _prevCamPos = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    float _prevCamYaw = FLT_MAX;
};