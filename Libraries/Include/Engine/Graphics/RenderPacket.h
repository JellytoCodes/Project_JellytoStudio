#pragma once

class Mesh;
class Material;

using InstanceID = std::pair<uint64, uint64>;

struct RenderPacket
{
    InstanceID  instanceID = { 0, 0 };

    Mesh*       pMesh      = nullptr;
    Material*   pMaterial  = nullptr;
    uint8       pass       = 0;
    Matrix      matWorld   = Matrix::Identity;

    bool IsValid() const
    {
        return pMesh != nullptr && pMaterial != nullptr;
    }
};
