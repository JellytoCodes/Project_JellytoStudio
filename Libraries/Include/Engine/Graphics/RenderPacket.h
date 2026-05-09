#pragma once

class Mesh;
class Material;

struct RenderPacket
{
    InstanceID  instanceID    = { 0, 0 };

    Mesh*       pMesh         = nullptr;
    Material*   pMaterial     = nullptr;

    uint8       pass          = 0;

    Matrix      matWorld      = Matrix::Identity;

    uint32      materialIndex = 0;

    bool IsValid() const
    {
        return pMesh != nullptr && pMaterial != nullptr;
    }
};
