# JellytoStudio — DirectX 11 Custom Block Editor Engine

DirectX 11과 C++17을 기반으로 제작한 커스텀 3D 블록 에디터 엔진입니다.  
외부 게임 엔진 없이 Win32 애플리케이션, D3D11 렌더링 파이프라인, Entity-Component 구조, Scene 관리, Chunk 기반 공간 분할, Hardware Instancing, Shadow Mapping, UI 렌더링, JSON 기반 씬 직렬화를 직접 구현했습니다.

이 프로젝트의 목적은 단순한 블록 배치 툴 제작이 아니라, 렌더링 파이프라인과 엔진 내부 구조를 직접 설계하고 병목을 최적화하는 클라이언트/렌더링 개발 역량을 보여주는 것입니다.

---

## 핵심 구현 요약

- DirectX 11 기반 Forward Rendering Pipeline 직접 구현
- Hardware Instancing 기반 대량 블록 렌더링
- DynamicInstancePool 기반 Persistent Map + 3-Slot Ring Buffer
- ChunkManager 기반 Frustum Culling 및 Face Occlusion Culling
- 2-Cascade Shadow Map 기반 방향광 그림자 처리
- Entity-Component 구조와 Scene Mutation Buffer 구현
- JSON 기반 블록 데이터 정의 및 씬 저장/불러오기
- Orthographic 독립 UI 렌더 패스 구현
- 렌더러 의존성을 제거한 `IBlockPlacer` 인터페이스 설계
- Controlled Benchmark Mode를 통한 최적화 옵션별 비교 측정 구조 구현

---

## 빌드 환경 및 의존성

### 요구 사양

| 항목 | 요구 사양 |
|------|----------|
| OS | Windows 10/11 (x64) |
| IDE | Visual Studio 2022 이상 |
| Windows SDK | 10.0.19041 이상 |
| Language | C++17 |
| Graphics API | DirectX 11 |

### 의존성 라이브러리

| 라이브러리 | 버전 | 포함 방식 | 비고 |
|-----------|------|----------|------|
| [Effect11 / FX11](https://github.com/microsoft/FX11) | 11.30 / Archived | 소스 직접 포함 | `.fx` 기반 Effect Framework 사용 |
| [DirectXTK](https://github.com/microsoft/DirectXTK) | 최신 | 소스 직접 포함 | SimpleMath (`Vector3`, `Matrix` 등) 사용 |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.x | 단일 헤더 포함 | 씬 직렬화 용도 |
| D3DX11 | Deprecated | 미사용 | 과거 Effect Framework 계열. 현재 프로젝트는 Effect11 사용 |

> **D3DX11 / Effect11 관련 안내**  
> 이 프로젝트는 `.fx` 파일 기반 셰이더 파이프라인을 사용하므로 Effect11을 사용합니다.  
> D3DX11은 deprecated된 레거시 유틸리티 라이브러리이며, 현재 프로젝트에서는 직접 사용하지 않습니다.  
> Effect11 역시 신규 프로젝트에 권장되는 방식은 아니며, 향후에는 Shader, InputLayout, ConstantBuffer, Sampler, Rasterizer/Blend/DepthStencil State를 명시적으로 관리하는 수동 바인딩 구조로 전환하는 것을 목표로 합니다.

### 빌드 순서

```text
1. JellytoStudio.sln 열기
2. 구성: Release | x64 선택
3. 솔루션 빌드
   - Engine 정적 라이브러리
   - Client 실행 파일
4. 실행: Client/x64/Release/JellytoStudio.exe
```

리소스 경로는 실행 파일 기준 상위 디렉터리를 참조합니다.

```text
../Resources/Models/MapModel/       FBX 모델
../Resources/Textures/              텍스처
../Resources/Data/BlockMaster.json  블록 정의 JSON
../Saved/                           씬 저장 파일
```

---

## 아키텍처

```text
┌──────────────────────────────────────────────────────────────┐
│  Client Layer                                                │
│  EditorApp / MainApp / IsometricCameraController             │
│  BlockPlacer   ←  IBlockPlacer + PlacedBlockRecord           │
│  InventoryData / PaletteWidget / DebugHUD                    │
│  ID3D11* 완전 미참조 ─ 렌더러 의존성 없음                    │
└──────────────────────────┬───────────────────────────────────┘
                           │ PlacedBlockRecord { x, y, z, type }
                           │ IBlockPlacer { GetPlacedBlocks, PlaceBlock, ClearAll }
┌──────────────────────────▼───────────────────────────────────┐
│  Scene & Spatial Layer                                       │
│  Scene ─ 엔티티 생명주기, 뮤테이션 버퍼                       │
│  ChunkManager ─ 16.0f 그리드, Frustum / Occlusion Culling     │
│  CollisionManager ─ AABB 충돌 처리                           │
│  SceneSerializer ─ JSON 저장/불러오기                        │
└──────────────────────────┬───────────────────────────────────┘
                           │ vector<Entity*> visibleEntities
┌──────────────────────────▼───────────────────────────────────┐
│  Render & Pipeline Layer                                     │
│  InstancingManager ─ SmartRebuild, Upload / Draw Phase 분리   │
│  DynamicInstancePool ─ Ring Buffer, Persistent Map            │
│  InstancingBuffer ─ Tiered 정적 버퍼 + 동적 풀 연결           │
│  ShadowPass ─ 2-Cascade Shadow Map                           │
│  UIManager ─ Orthographic 독립 패스                          │
│  Graphics ─ D3D11 Device, SwapChain, State Cache              │
└──────────────────────────┬───────────────────────────────────┘
                           │ DrawIndexedInstanced
┌──────────────────────────▼───────────────────────────────────┐
│  D3D11 Hardware                                              │
│  ID3D11Device / ID3D11DeviceContext / IDXGISwapChain          │
│  Ring Buffers / HLSL Shaders / Texture2DArray ShadowMap       │
└──────────────────────────────────────────────────────────────┘
```

---

## 렌더러 의존성 분리

`IBlockPlacer` 인터페이스와 `PlacedBlockRecord` POD 구조체를 통해 게임 로직과 D3D11 렌더러 의존성을 분리했습니다.

```cpp
// BlockPlacerInterface.h
struct PlacedBlockRecord {
    float x;
    float y;
    float z;
    int32 type;
};

class IBlockPlacer {
public:
    virtual const std::vector<PlacedBlockRecord>& GetPlacedBlocks() const = 0;
    virtual bool PlaceBlock(float x, float y, float z, int32 type) = 0;
    virtual void ClearAllBlocks() = 0;
};
```

`BlockPlacer`는 `IBlockPlacer`와 `MonoBehaviour`를 다중 상속합니다.  
DirectX 11 객체인 `ID3D11Buffer`, `ID3D11ShaderResourceView` 등은 Client Layer에 포함하지 않습니다.

이 구조 덕분에 씬 저장, 블록 배치, 팔레트 UI, 인벤토리 데이터는 렌더러 상태와 독립적으로 동작합니다.

---

## 렌더링 파이프라인

`Scene::Render()`는 다음 순서로 실행됩니다.

```text
1. ShadowPass::Render(visibleEntities, lightDir, camPos)
   └─ ComputeCascadeVPs()
   └─ BeginFrame()
   └─ RenderCascade(0)
   └─ RenderCascade(1)
   └─ EndFrame()

2. Camera::RenderForward()
   └─ InstancingManager::Render(entities)
      ├─ Upload Phase
      │   ├─ DynamicInstancePool::BeginFrame()
      │   ├─ SmartRebuildMeshGroups()
      │   ├─ RenderModelRenderer()
      │   ├─ BuildAnimData()
      │   └─ DynamicInstancePool::EndFrame()
      └─ Draw Phase
          ├─ RenderMeshRenderer()
          ├─ RenderModelRenderer()
          └─ DrawAnimRenderer()

3. UIManager::Render()
   └─ Orthographic UI Pass
```

Upload Phase와 Draw Phase를 분리하는 이유는 D3D11에서 `Map`된 버퍼를 `IASetVertexBuffers`와 `DrawIndexedInstanced`에 동시에 사용하는 것을 피하기 위해서입니다.

`DynamicInstancePool::BeginFrame()`에서 버퍼를 `Map`하면, `EndFrame()`에서 `Unmap`하기 전까지 해당 버퍼를 Draw에 사용하지 않습니다.  
따라서 인스턴스 데이터 업로드를 먼저 끝낸 뒤 Draw Phase에서 바인딩과 드로우 호출을 수행합니다.

---

## 공간 분할 시스템

### ChunkManager

모든 블록 엔티티는 `kChunkSize = 16.0f` 단위 청크로 관리합니다.

```cpp
// 청크 좌표 -> uint64 키 압축
static uint64 CoordKey(int32 cx, int32 cz);

// 엔티티 -> 청크 역색인
std::unordered_map<Entity*, uint64> _entityToKey;

struct Chunk {
    std::vector<Entity*>  entities;
    DirectX::BoundingBox  aabb;
    bool                  aabbDirty;
    bool                  wasVisible;
};
```

### Frustum Culling

`BoundingFrustum`과 각 청크의 `AABB`를 교차 판정합니다.

```text
Camera Frustum
  -> Chunk AABB test
  -> DISJOINT이면 청크 전체 제외
  -> 통과한 청크만 내부 엔티티 검사
```

청크 AABB는 매 프레임 재계산하지 않습니다.  
엔티티 추가, 제거, 이동으로 청크 구성이 변경된 경우에만 `aabbDirty`를 설정하고 `RebuildAABB()`를 수행합니다.

### Face Visibility Occlusion Culling

Mesh 블록은 6방향 이웃이 모두 채워진 경우 렌더 리스트에서 제외합니다.  
위치 조회는 `_positionMap`을 사용해 O(1)로 처리합니다.

```cpp
// 20비트 x 3축, 바이어스 524288로 음수 좌표 처리
static uint64 PositionKey(const Vec3& pos) {
    const int32 ix = static_cast<int32>(std::round(pos.x)) + 524288;
    const int32 iy = static_cast<int32>(std::round(pos.y)) + 524288;
    const int32 iz = static_cast<int32>(std::round(pos.z)) + 524288;

    return (uint64(ix) << 40) | (uint64(iy) << 20) | uint64(iz);
}
```

`CollectVisible()`에서 Mesh 블록은 6방향 이웃 전체를 `HasSolidBlockAt()`으로 확인합니다.

```text
+X / -X
+Y / -Y
+Z / -Z
```

6방향이 모두 채워진 경우 내부 블록으로 판단해 `outEntities`에 추가하지 않습니다.  
Model 블록은 나무, 버섯처럼 비균일 형태를 가지므로 Face Occlusion 대상에서 제외하고 항상 통과시킵니다.

### 레이 피킹

`PickBlocks`는 Priming, Floor, Mushroom 세 채널을 단일 순회로 처리합니다.

```text
기존 방식:
  Priming Raycast
  Floor Raycast
  Mushroom Raycast

현재 방식:
  PickBlocks 1회
    -> 채널별 후보 수집
    -> 거리 기준 최종 선택
```

---

## 인스턴싱 시스템

### InstanceID

동일한 Mesh, Material, Chunk를 공유하는 엔티티를 하나의 인스턴싱 그룹으로 자동 분류합니다.

```cpp
struct InstanceID {
    uint64 resource0;  // Mesh 포인터 해시
    uint64 resource1;  // Material 포인터 해시
    uint64 bucket;     // Chunk Key
};
```

해시 결합에는 Fibonacci Hash 계열의 조합식을 사용합니다.

```cpp
h ^= hash(resource1) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
```

### SmartRebuildMeshGroups

Mesh 인스턴싱 그룹은 매 프레임 전체 재빌드하지 않고 dirty 상태에 따라 갱신합니다.

```text
전체 재빌드 조건:
  _bDirty || _meshDirty

부분 재빌드 조건:
  _meshGroupDirty

건너뛰기 조건:
  엔티티 수 불변 && !_partialDirtyMesh.count(id)
```

`_tmpMeshCache`를 영구 멤버로 유지해 매 프레임 `unordered_map` 힙 할당과 해제를 줄입니다.

```cpp
std::swap(_meshCache, _tmpMeshCache);
_tmpMeshCache.clear();
```

`move`를 사용하면 `_tmpMeshCache`가 최소 상태로 리셋되어 다음 프레임 insert 시 버킷 재성장이 발생할 수 있습니다.  
`swap` 후 `clear`를 사용하면 이전 프레임에서 충분히 성장한 버킷 배열을 유지하면서 원소만 제거할 수 있습니다.

### RenderStats

렌더링 통계는 `RenderStats`에 집계하고 `DebugHUD`에서 실시간 표시합니다.

```cpp
struct RenderStats {
    uint32 modelDrawCalls;
    uint32 meshDrawCalls;
    uint32 totalDrawCalls;
    uint32 totalInstances;
    uint32 dynamicBuffers;
    uint32 staticBuffers;
    uint32 meshGroupsRebuilt;
    uint32 meshGroupsSkipped;
};
```

`DumpInstancingStats()`를 통해 로그 출력도 지원합니다.

---

## 버퍼 관리 전략

### DynamicInstancePool — Persistent Map

```cpp
void   BeginFrame();
uint32 Append(const InstancingData* data, uint32 count);
void   EndFrame();
```

`BeginFrame()`에서 `WRITE_DISCARD`로 Map을 1회 수행하고, `Append()`는 내부에서 `memcpy`만 수행합니다.  
`EndFrame()`에서 Unmap을 1회 수행합니다.

3-Slot Ring Buffer를 사용해 GPU가 이전 프레임의 인스턴스 데이터를 읽는 동안 CPU가 다음 슬롯에 데이터를 작성할 수 있도록 했습니다.  
이를 통해 Map/Unmap 호출 횟수를 줄이고 CPU-GPU 동기화 대기 가능성을 최소화합니다.

```text
Frame N     -> Slot 0
Frame N + 1 -> Slot 1
Frame N + 2 -> Slot 2
Frame N + 3 -> Slot 0 재사용
```

### InstancingBuffer — Tiered Allocation

정적 인스턴싱 버퍼는 필요한 인스턴스 수에 따라 최소 티어를 선택합니다.

| 티어 | 크기 | 적합 용도 |
|------|------|----------|
| kTierSmall | 64 | 희박한 청크 |
| kTierMedium | 512 | 일반 블록 그룹 |
| kTierLarge | 4,096 | 밀집 씬 |
| kTierMax | 10,000 | 전체 씬 |

### SetData 직접 포인터 경로

동적 버퍼에서 `SetData`를 호출할 때 `_data(vector)` 복사를 건너뛰고 외부 포인터를 직접 보관합니다.  
`UploadData()`가 `Pool->Append()`를 호출할 때 외부 버퍼에서 Pool 버퍼로 `memcpy` 1회만 수행합니다.

```text
SetData 경로:
  외부 worldVec -> Pool
  memcpy 1회

AddData 누적 경로:
  _data -> Pool
  memcpy 1회

정적 버퍼 경로:
  _data -> Static Buffer
  memcpy 1회
```

### Block Entity Pool

블록 배치와 제거에서 런타임 `new/delete`를 줄이기 위해 Entity Pool을 사용합니다.

```text
시작 시 사전 할당:
  Mesh 블록 128개
  Model 블록 16개 / 종류

배치 시:
  _meshPool.pop_back()
  -> 컴포넌트 재구성
  -> Scene::AddDirect()

제거 시:
  ChunkManager::Unregister()
  -> Scene::Detach()
  -> _meshPool.push_back()
```

`Scene::AddDirect()`는 `AddImmediate()` 경로를 사용하며, Awake/Start 재실행 없이 씬에 직접 추가합니다.  
`Scene::Detach()`는 swap-and-pop으로 `_objects`에서 분리한 뒤 `unique_ptr<Entity>`를 반환합니다.

---

## 섀도우 패스

### 2-Cascade Shadow Map

단일 `Texture2DArray`로 두 개의 Shadow Cascade를 관리합니다.

```text
Cascade 0:
  카메라 반경 kNearCascadeRadius = 25.0f 기준 sphere tight fit VP

Cascade 1:
  씬 전체 정적 엔티티 AABB 기반 VP
```

### 리소스 생성

```cpp
td.Format    = DXGI_FORMAT_R32_TYPELESS;
td.ArraySize = kCascadeCount;
td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

// 슬라이스별 DSV
dsvd.ViewDimension                  = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
dsvd.Texture2DArray.FirstArraySlice = cascadeIdx;
dsvd.Texture2DArray.ArraySize       = 1;

// 단일 SRV
srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
srvd.Format        = DXGI_FORMAT_R32_FLOAT;
```

### Render 흐름

```text
BuildGroups(entities)
  -> 엔티티 순회 1회

ComputeCascadeVPs(camPos)
  -> 2개 Light VP 계산

for cascade in [0, 1]:
  RenderCascade(cascade, VP[cascade])
    -> DSV[cascade] 바인딩
    -> ConstantBuffer 업데이트
    -> Draw
```

Depth Vertex Shader는 런타임에 내장 문자열 리터럴에서 `D3DCompile`로 컴파일합니다.  
스태틱 메시용과 스키닝 메시용 두 가지 Depth VS를 내장하고 있습니다.

### 픽셀 셰이더 카스케이드 선택

```hlsl
float dist    = length(worldPos - CameraPosition());
int   cascade = (dist < CascadeSplit) ? 0 : 1;

float4 lsPos = mul(float4(worldPos, 1.0f), LightVP[cascade]);

shadow += ShadowMap.SampleCmpLevelZero(
    ShadowSampler,
    float3(uv + offset * ShadowTexelSize, (float)cascade),
    depth
);
```

Shadow Sampler 설정은 다음과 같습니다.

```text
Filter         : COMPARISON_MIN_MAG_LINEAR_MIP_POINT
AddressU/V     : BORDER
BorderColor    : white(1, 1, 1, 1)
ComparisonFunc : LESS_EQUAL
```

범위를 벗어난 UV는 흰색으로 처리되어 섀도우가 없는 영역으로 샘플링됩니다.

---

## UI 시스템

`UIManager`는 3D 렌더 패스 이후 Orthographic 독립 패스로 실행됩니다.  
이 패스에서는 Depth Test를 끄고 UI 전용 Vertex Buffer / Index Buffer를 사용합니다.

```cpp
struct DrawCmd {
    uint32 indexOffset;
    uint32 indexCount;
    uint32 pass;   // 0 = 단색, 1 = 텍스처
    ComPtr<ID3D11ShaderResourceView> srv;
};
```

지원 API는 다음과 같습니다.

```text
AddRect
AddRectBorder
AddTexturedRect
AddText
```

텍스트는 GDI+ `Graphics::DrawString`으로 오프스크린 비트맵에 렌더링한 뒤 `CreateTexture2D`와 `CreateShaderResourceView`를 통해 D3D11 텍스처로 변환해 캐싱합니다.

모든 쿼드는 단일 VB/IB에 누적한 뒤 `DrawCmd` 순서대로 `DrawIndexed`를 일괄 호출합니다.

---

## 엔티티 컴포넌트 시스템

### Entity

컴포넌트는 고정 크기 배열에 저장합니다.  
`GetComponent<T>()`는 컴파일 타임 인덱스 기반 O(1) 조회를 수행합니다.

```cpp
std::array<std::unique_ptr<Component>, FIXED_COMPONENT_COUNT> _components;
std::vector<std::unique_ptr<MonoBehaviour>>                   _scripts;
```

`GetComponent<T>()`는 `ComponentTypeOf<T>::kType`으로 배열 인덱스를 정적으로 결정합니다.

### Camera

카메라는 Culling 결과와 정렬 상태를 추적합니다.

```cpp
struct CullStats {
    uint32 totalEntities;
    uint32 visibleEntities;
    uint32 culledEntities;
    uint32 meshRebuildCount;
    uint32 modelRebuildCount;
};
```

`_meshVisibilityHash`, `_modelVisibilityHash`, `_prevCamPos`, `_prevCamYaw`를 추적해 카메라 정지 시 불필요한 소트와 재빌드를 건너뜁니다.

### Scene Mutation Buffer

Scene은 순회 중 엔티티 추가/삭제로 인한 iterator invalidation을 방지하기 위해 Mutation Buffer를 사용합니다.

```cpp
std::vector<std::unique_ptr<Entity>> _pendingAdds;
std::vector<Entity*>                 _pendingRemoves;
uint32                               _iterationDepth;
```

이터레이션 중 추가/제거 요청은 pending 버퍼에 기록하고, 이터레이션 종료 후 `FlushPendingMutations()`에서 일괄 반영합니다.

`_objects`는 `vector<unique_ptr<Entity>>`로 연속 메모리를 유지하고, 삭제는 swap-and-pop으로 O(1) 처리합니다.

### Collision Channel

```cpp
enum class CollisionChannel : uint8 {
    None      = 0,
    Default   = 1 << 0,
    Character = 1 << 1,
    Priming   = 1 << 2,
    Mushroom  = 1 << 3,
    Floor     = 1 << 4,
    All       = 0xFF
};

enum class PlaceFace : uint8 {
    Top    = 1 << 0,
    Side   = 1 << 1,
    Bottom = 1 << 2,
    All    = 0xFF
};
```

`CollisionChannel`은 레이 피킹과 충돌 판정 대상을 구분하는 데 사용합니다.  
`PlaceFace`는 블록 배치 가능 면을 제한하는 데 사용합니다.

---

## 블록 데이터 정의

모든 블록 속성은 `BlockMaster.json`에서 정의하며 `BlockTable::Load()`로 파싱합니다.

```cpp
struct BlockRecord {
    int32            typeId;
    std::wstring     key;
    std::wstring     label;
    bool             isEraser;
    BlockRenderType  renderType;
    std::wstring     modelName;
    float            modelScale;
    BlockUVRect      paletteRect;
    ColliderSize     collider;
    CollisionChannel ownChannel;
    uint8            pickableMask;
    uint8            faceMask;
};
```

주요 필드는 다음 역할을 가집니다.

| 필드 | 역할 |
|------|------|
| `typeId` | 블록 타입 식별자 |
| `key` | 내부 데이터 키 |
| `label` | UI 표시 이름 |
| `isEraser` | 삭제 도구 여부 |
| `renderType` | Mesh / Model 렌더링 방식 |
| `modelName` | Model 블록에서 사용할 FBX 모델 이름 |
| `modelScale` | 모델 스케일 |
| `paletteRect` | 텍스처 아틀라스 UV 영역 |
| `collider` | Collider 크기 타입 |
| `ownChannel` | 자신의 충돌 채널 |
| `pickableMask` | 피킹 가능한 대상 채널 |
| `faceMask` | 배치 가능한 면 |

---

## 씬 직렬화

`SceneSerializer`는 nlohmann/json을 사용해 씬을 저장하고 불러옵니다.

저장 대상은 `IBlockPlacer::GetPlacedBlocks()`를 통해 수집한 `PlacedBlockRecord` 목록입니다.  
따라서 저장 데이터는 렌더러 상태, D3D11 리소스, 인스턴싱 버퍼와 완전히 독립적입니다.

```text
Scene
  -> IBlockPlacer::GetPlacedBlocks()
  -> vector<PlacedBlockRecord>
  -> JSON 저장

JSON 로드
  -> PlacedBlockRecord 복원
  -> IBlockPlacer::PlaceBlock()
  -> Scene Entity 재생성
```

---

## 셰이더 구조

### 상수 버퍼 레지스터 배치

| 레지스터 | 구조체 | 업데이트 주기 | 내용 |
|---------|--------|--------------|------|
| b0 | ShadowBuffer | Per-Frame | LightVP[2], ShadowBias, ShadowTexelSize, CascadeSplit |
| b1 | GlobalBuffer | Per-Level | V, P, VP, VInv |
| b2 | TransformBuffer | Per-Object | W |
| — | MaterialDesc | Per-Object | ambient, diffuse, specular, emissive |
| — | LightDesc | Per-Frame | ambient, diffuse, specular, emissive, direction |

### 텍스처 레지스터

| 레지스터 | 리소스 | 설명 |
|---------|--------|------|
| t0 | `Texture2D g_BlockAtlas` | 블록 텍스처 아틀라스 |
| t1 | `StructuredBuffer<float4> g_AtlasRects` | UV 좌표 |
| t2 | `Texture2DArray ShadowMap` | 2-Cascade Shadow Map |

### 인스턴스 버퍼 입력 레이아웃

```hlsl
// Slot 0: Per-Vertex
float4 position : POSITION;
float2 uv       : TEXCOORD;
float3 normal   : NORMAL;
float3 tangent  : TANGENT;

// Slot 1: Per-Instance
matrix world         : INST;
uint   materialIndex : INST_MATERIAL;
```

### BlockShader 라이팅 모델

BlockShader는 Blinn-Phong, Rim Light, PCF Shadow를 조합합니다.

```hlsl
float4 rect    = g_AtlasRects[materialIndex];
float2 atlasUV = uv * rect.zw + rect.xy;

float4 ambient  = baseColor * GlobalLight.ambient  * Material.ambient;
float4 diffuse  = baseColor * NdotL * GlobalLight.diffuse * Material.diffuse;

float  spec     = pow(saturate(dot(R, E)), 16.0f);
float4 specular = GlobalLight.specular * Material.specular * spec;

float  rim      = 1.0f - saturate(dot(E, N));
float4 emissive = GlobalLight.emissive * Material.emissive * pow(rim, 2.0f);

float  shadow   = ComputeShadowFactor(worldPos);
float4 final    = ambient + (diffuse + specular) * shadow + emissive;
```

PCF Shadow는 3x3, 9 Sample 방식으로 처리합니다.

---

## State Shadow Cache

D3D11 상태 변경 API 호출을 줄이기 위해 현재 바인딩 상태를 캐싱합니다.

```cpp
struct ShadowStateCache {
    ID3D11RasterizerState*   rsState;
    bool                     rsValid;

    ID3D11DepthStencilState* dssState;
    bool                     dssValid;
    UINT                     stencilRef;

    ID3D11BlendState*        blendState;
    bool                     blendValid;
    FLOAT                    blendFactor[4];
    UINT                     sampleMask;

    ID3D11Buffer*            vb0;
    bool                     vb0Valid;
    UINT                     vb0Stride;
    UINT                     vb0Offset;

    ID3D11Buffer*            ib;
    DXGI_FORMAT              ibFormat;
};
```

`SetRasterizerState()` 등 각 함수에서 현재 바인딩 상태와 요청 상태를 비교합니다.  
동일한 상태라면 D3D11 API 호출을 건너뜁니다.

셰이더 전환 시에는 `InvalidateStateCache()`로 전체 캐시를 무효화합니다.

---

## 최적화 목록

| 항목 | 구현 위치 | 내용 |
|------|---------|------|
| Hardware Instancing | `InstancingManager`, `InstancingBuffer` | N개 블록을 1 DrawCall로 렌더링 |
| Persistent Map | `DynamicInstancePool` | 프레임당 Map/Unmap 1회, CPU-GPU 동기화 비용 완화 |
| Ring Buffer 3-Slot | `DynamicInstancePool` | GPU 읽기 중 CPU 쓰기 가능성 확보 |
| PickBlocks 통합 | `BlockPlacer::Update` | 3채널 레이캐스트를 1회 순회로 통합 |
| SmartRebuild | `InstancingManager` | dirty 그룹만 재빌드 |
| swap + clear 패턴 | `SmartRebuildMeshGroups` | `unordered_map` 버킷 용량 보존 |
| PruneEmptyGroups 조건부 | `InstancingManager` | `_hasPendingPrune` 플래그로 정적 프레임 생략 |
| Block Entity Pool | `BlockPlacer` | 사전 할당으로 런타임 new/delete 감소 |
| Frustum Culling | `ChunkManager::CollectVisible` | 청크 AABB 기준 가시성 판정 |
| Face Occlusion Culling | `ChunkManager::CollectVisible` | 6방향 위치 해시 조회로 내부 블록 제외 |
| Tiered Buffer | `InstancingBuffer::NextTier` | 64 / 512 / 4096 / 10000 최소 티어 선택 |
| SetData 직접 포인터 | `InstancingBuffer::SetData` | 동적 버퍼 memcpy 2회 경로를 1회로 축소 |
| Scene vector 전환 | `Scene::_objects` | 순회 캐시 적중률 개선 |
| 2-Cascade CSM | `ShadowPass` | Texture2DArray 기반 근거리 그림자 밀도 향상 |
| State Shadow Cache | `Graphics` | 중복 D3D11 상태 변경 호출 방지 |
| UI Orthographic 패스 | `UIManager` | 3D 패스와 UI 패스 분리 |
| 상수 버퍼 빈도 분할 | `ShaderDesc.h` | Per-Level / Frame / Object 분리 |

---

## Controlled Benchmark Mode

본 비교는 과거 커밋 기준의 before/after 비교가 아니라, 동일 씬에서 최적화 옵션을 끈 baseline과 현재 구현을 비교하는 controlled benchmark입니다.

### StressPanel Options

| Button | Option | Description |
|--------|--------|-------------|
| Frustum ON/OFF | Frustum Culling | Chunk AABB 및 비관리 렌더러 frustum test 토글 |
| Face ON/OFF | Face Occlusion | 6방향 이웃으로 완전히 가려진 mesh block 제외 토글 |
| SmartRebuild ON/OFF | SmartRebuild | Dirty mesh group rebuild와 full mesh group rebuild 비교 |

### Benchmark Presets

StressPanel에서 다음 preset을 생성해 동일 조건으로 측정합니다.

| Preset | Purpose |
|--------|---------|
| Flat 1K | 인스턴싱 draw call / total instance 기준 측정 |
| Dense 16^3 | 내부 블록 face occlusion 효과 측정 |
| Seed Random 10K | 대규모 랜덤 배치 및 chunk culling 기준 측정 |

### Measurement Table

현재 표는 측정 입력용 템플릿입니다.  
정량 결과는 동일 하드웨어, 동일 빌드 옵션, 동일 카메라 위치 기준으로 측정 후 갱신합니다.

| Scene | Mode | Total Entities | Visible Entities | Total Chunks | Visible Chunks | Mesh DrawCalls | Model DrawCalls | Total Instances | Mesh Groups Rebuilt | Mesh Groups Skipped | CPU Frame |
|-------|------|---------------:|-----------------:|-------------:|---------------:|----------------:|-----------------:|----------------:|--------------------:|--------------------:|----------:|
| Flat 1K | Baseline OFF | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD |
| Flat 1K | Optimized ON | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD |
| Dense 16^3 | Culling OFF | 4096 | 4096 | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD |
| Dense 16^3 | Face Occlusion ON | 4096 | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD |
| Seed Random 10K | SmartRebuild OFF | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD |
| Seed Random 10K | SmartRebuild ON | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD |

---

## 프로젝트 구조

```text
JellytoStudio/
|-- Client/
|   `-- Source/
|       |-- Core/
|       |   `-- pch.h
|       |-- Data/
|       |   |-- BlockTable.h
|       |   `-- BlockTable.cpp
|       |-- Main/
|       |   |-- EditorApp
|       |   |-- MainApp
|       |   `-- Actors
|       |-- Resource/
|       |   `-- BlockMaterialProvider
|       |-- Scripts/
|       |   |-- BlockPlacer
|       |   |-- IsometricCameraController
|       |   `-- PointClickController
|       `-- UI/
|           |-- PaletteWidget
|           |-- InventoryWidget
|           |-- InventoryData
|           |-- DebugHUD
|           `-- StressPanel
|
`-- Engine/
    |-- Shaders/
    |   |-- BlockShader.hlsl
    |   |-- ShaderCommon.hlsli
    |   |-- Lighting.hlsli
    |   |-- StaticMeshShader.hlsl
    |   |-- SkinnedMeshShader.hlsl
    |   |-- UIShader.hlsl
    |   |-- ColliderDebugShader.hlsl
    |   `-- SkySphereShader.hlsl
    |
    `-- Source/
        |-- App/
        |   |-- Application.h/cpp
        |   |-- DetailWindow
        |   |-- ItemWindow
        |   |-- ChunkDebugWindow
        |   `-- Managers/
        |       `-- WindowManager
        |-- Audio/
        |   |-- AudioManager
        |   `-- AudioDataTable
        |-- Core/
        |   |-- Framework.h
        |   |-- InputManager
        |   `-- TimeManager
        |-- Entity/
        |   |-- Entity.h/cpp
        |   |-- Actor.h/cpp
        |   `-- Components/
        |       |-- Transform
        |       |-- Camera
        |       |-- MeshRenderer
        |       |-- ModelRenderer
        |       |-- ModelAnimator
        |       |-- AnimStateMachine
        |       |-- Light
        |       `-- Collider/
        |           |-- AABBCollider
        |           |-- BaseCollider
        |           `-- CollisionChannel
        |-- Graphics/
        |   |-- Graphics.h/cpp
        |   |-- ShadowPass.h/cpp
        |   |-- RenderPacket.h
        |   `-- Managers/
        |       `-- InstancingManager
        |-- Pipeline/
        |   |-- DynamicInstancePool
        |   |-- InstancingBuffer
        |   |-- Shader.h/cpp
        |   |-- ConstantBuffer.h
        |   `-- VertexBuffer.h
        |-- Scene/
        |   |-- Scene.h/cpp
        |   |-- ChunkManager.h/cpp
        |   |-- SceneSerializer
        |   |-- BlockPlacerInterface.h
        |   `-- PickUtils.h
        |-- Types/
        |   `-- ShaderDesc.h
        `-- UI/
            `-- UIManager.h/cpp
```

---

## 주요 소스 진입점

| 파일 / 모듈 | 역할 |
|------------|------|
| `Application` | Win32 윈도우, 메인 루프, 메뉴 처리 |
| `Graphics` | D3D11 Device, SwapChain, Render State 관리 |
| `Scene` | 엔티티 생명주기, Add/Remove, Mutation Buffer |
| `ChunkManager` | 공간 분할, Frustum Culling, Face Occlusion Culling |
| `InstancingManager` | 인스턴싱 그룹 생성, SmartRebuild, 렌더링 통계 |
| `DynamicInstancePool` | Persistent Map, 3-Slot Ring Buffer |
| `InstancingBuffer` | Tiered Buffer, 정적/동적 인스턴스 버퍼 관리 |
| `ShadowPass` | 2-Cascade Shadow Map 렌더링 |
| `UIManager` | Orthographic UI 렌더 패스 |
| `BlockPlacer` | 블록 배치, 삭제, 피킹, Entity Pool |
| `BlockTable` | JSON 기반 블록 데이터 로드 |
| `SceneSerializer` | 씬 저장/불러오기 |
| `ShaderDesc.h` | 상수 버퍼 구조체 및 셰이더 데이터 정의 |

---

## 라이선스

이 프로젝트는 학습 및 포트폴리오 목적으로 공개되어 있습니다.  
포함된 서드파티 라이브러리는 각각의 라이선스를 따릅니다.

| 라이브러리 | 라이선스 |
|-----------|---------|
| Effect11 / FX11 | MIT |
| DirectXTK | MIT |
| nlohmann/json | MIT |

---

*C++17 · DirectX 11 · HLSL · WRL::ComPtr · Hardware Instancing · Ring Buffer · 2-Cascade CSM · Face Occlusion Culling · nlohmann/json*