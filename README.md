# JellytoStudio — DirectX 11 Custom Block Editor Engine

C++17과 DirectX 11로 구현한 커스텀 3D 블록 에디터 엔진입니다.  
외부 엔진 없이 렌더링 파이프라인 전 과정을 직접 설계하고 구현했습니다.

---

## 목차

- [빌드 환경](#빌드-환경)
- [프로젝트 사양](#프로젝트-사양)
- [아키텍처](#아키텍처)
- [렌더링 파이프라인](#렌더링-파이프라인)
- [공간 분할 시스템](#공간-분할-시스템)
- [인스턴싱 시스템](#인스턴싱-시스템)
- [버퍼 관리 전략](#버퍼-관리-전략)
- [섀도우 패스](#섀도우-패스)
- [UI 시스템](#ui-시스템)
- [엔티티 컴포넌트 시스템](#엔티티-컴포넌트-시스템)
- [블록 데이터 정의](#블록-데이터-정의)
- [씬 직렬화](#씬-직렬화)
- [셰이더 구조](#셰이더-구조)
- [최적화 목록](#최적화-목록)
- [프로젝트 구조](#프로젝트-구조)

---

## 빌드 환경

| 항목 | 요구 사양 |
|------|----------|
| OS | Windows 10/11 (x64) |
| IDE | Visual Studio 2022 이상 |
| Windows SDK | 10.0.19041 이상 |
| 추가 라이브러리 | DirectX SDK (D3DX11, Effect11), DirectXTK, nlohmann/json |

```
1. JellytoStudio.sln 열기
2. 구성: Release | x64
3. 솔루션 빌드 (Engine 정적 라이브러리 → Client 실행 파일 순서로 자동 빌드)
4. 실행: Client/x64/Release/JellytoStudio.exe
```

리소스 경로는 실행 파일 기준 상위 디렉터리 참조입니다.

```
../Resources/Models/MapModel/     FBX 모델
../Resources/Textures/            블록 텍스처
../Resources/Data/BlockMaster.json  블록 정의 JSON
../Saved/                         씬 저장 파일 (JSON)
```

---

## 프로젝트 사양

| 항목 | 값 |
|------|----|
| 해상도 | 1280 × 720 (런타임 리사이즈 지원) |
| 카메라 Near / Far | 1.0f / 1000.0f |
| 기본 FOV | π/4 (45°) |
| 청크 크기 | 16.0f 월드 단위 |
| 링 버퍼 슬롯 수 | 3 (`kRingCount`) |
| 풀 최대 인스턴스 | 30,000 (`kMaxInstances`) |
| 인스턴스 버퍼 티어 | 64 / 512 / 4,096 / 10,000 |
| 섀도우 맵 해상도 | 1024 × 1024 (`kShadowMapSize`) |
| 애니메이션 최대 본 | 250 (`MAX_MODEL_TRANSFORMS`) |
| 애니메이션 최대 키프레임 | 500 (`MAX_MODEL_KEYFRAMES`) |
| 인스턴스 당 애니메이터 | 250 (`MAX_MODEL_INSTANCE`) |
| 씬 직렬화 형식 | JSON (nlohmann/json) |

---

## 아키텍처

```
┌──────────────────────────────────────────────────────────────┐
│  Client Layer                                                │
│  EditorApp / MainApp / IsometricCameraController             │
│  BlockPlacer   ←  IBlockPlacer(순수 가상) + PlacedBlockRecord(POD) │
│  InventoryData / PaletteWidget / DebugHUD                   │
│  ID3D11* 완전 미참조 ─ RHI 의존 Zero                         │
└──────────────────────────┬───────────────────────────────────┘
                           │ PlacedBlockRecord { x, y, z, type }
                           │ IBlockPlacer { GetPlacedBlocks, PlaceBlock, ClearAll }
┌──────────────────────────▼───────────────────────────────────┐
│  Scene & Spatial Layer                                       │
│  Scene ─ 엔티티 생명주기, 뮤테이션 버퍼(pendingAdds/Removes) │
│  ChunkManager ─ 16.0f 그리드, Frustum·Occlusion Culling      │
│  CollisionManager ─ AABB 충돌 처리                           │
│  SceneSerializer ─ JSON 저장/불러오기                        │
└──────────────────────────┬───────────────────────────────────┘
                           │ vector<Entity*> visibleEntities
┌──────────────────────────▼───────────────────────────────────┐
│  Render & Pipeline Layer                                     │
│  InstancingManager ─ SmartRebuild, 업로드/드로우 페이즈 분리  │
│  DynamicInstancePool ─ Ring Buffer, Persistent Map           │
│  InstancingBuffer ─ Tiered 정적 버퍼 + 동적 풀 연결           │
│  ShadowPass ─ 2-Cascade Shadow Map (Texture2DArray)          │
│  UIManager ─ Orthographic 독립 패스                          │
│  Graphics ─ D3D11 디바이스, Shadow State Cache               │
└──────────────────────────┬───────────────────────────────────┘
                           │ DrawIndexedInstanced
┌──────────────────────────▼───────────────────────────────────┐
│  D3D11 Hardware                                              │
│  ID3D11Device / ID3D11DeviceContext / IDXGISwapChain         │
│  Ring Buffers / HLSL Shaders / Texture2DArray ShadowMap      │
└──────────────────────────────────────────────────────────────┘
```

### RHI 디커플링

`IBlockPlacer` 인터페이스와 `PlacedBlockRecord` POD 구조체가 게임 로직과 렌더러의 경계입니다.

```cpp
// BlockPlacerInterface.h
struct PlacedBlockRecord { float x, y, z; int32 type; };

class IBlockPlacer {
    virtual const std::vector<PlacedBlockRecord>& GetPlacedBlocks() const = 0;
    virtual bool PlaceBlock(float x, float y, float z, int32 type) = 0;
    virtual void ClearAllBlocks() = 0;
};
```

`BlockPlacer`는 `IBlockPlacer`와 `MonoBehaviour`를 다중 상속합니다.  
DirectX 11 객체 (`ID3D11Buffer`, `ID3D11ShaderResourceView` 등)는 단 한 줄도 포함하지 않습니다.

---

## 렌더링 파이프라인

`Scene::Render()`는 다음 순서로 실행됩니다.

```
1. ShadowPass::Render(visibleEntities, lightDir, camPos)
   └─ ComputeCascadeVPs()  ─ 2개 Light VP 계산
   └─ BeginFrame / [2× RenderCascade] / EndFrame

2. Camera::RenderForward()
   └─ InstancingManager::Render(entities)
      ├─ [Upload Phase]
      │   ├─ DynamicInstancePool::BeginFrame()  ← DISCARD Map 1회
      │   ├─ SmartRebuildMeshGroups()           ← dirty 그룹만 SetData + UploadData
      │   ├─ RenderModelRenderer()              ← 모델 데이터 빌드 + UploadData
      │   ├─ BuildAnimData()                    ← 애님 데이터 빌드 + UploadData
      │   └─ DynamicInstancePool::EndFrame()    ← Unmap 1회
      └─ [Draw Phase]
          ├─ RenderMeshRenderer()   ← BindBuffer + DrawIndexedInstanced
          ├─ RenderModelRenderer()  ← BindBuffer + DrawIndexedInstanced
          └─ DrawAnimRenderer()     ← BindBuffer + DrawIndexedInstanced (upload no-op)

3. UIManager::Render()
   └─ Orthographic 패스, Depth Test OFF
```

업로드 페이즈와 드로우 페이즈를 분리하는 이유:  
D3D11은 `Map`된 버퍼를 `IASetVertexBuffers` + `DrawIndexedInstanced`에 동시 사용하는 것을 허용하지 않습니다.  
`DynamicInstancePool::BeginFrame()`이 Map하면 `EndFrame()`이 Unmap할 때까지 해당 버퍼로 Draw할 수 없습니다.

---

## 공간 분할 시스템

### ChunkManager

모든 블록 엔티티를 `kChunkSize = 16.0f` 단위 청크로 관리합니다.

```cpp
// 청크 좌표 → uint64 키 압축
static uint64 CoordKey(int32 cx, int32 cz);

// 엔티티 → 청크 역색인 (O(1) 역추적)
std::unordered_map<Entity*, uint64> _entityToKey;

// 청크 내 정보
struct Chunk {
    std::vector<Entity*>  entities;
    DirectX::BoundingBox  aabb;
    bool                  aabbDirty;   // 블록 변화 시만 true
    bool                  wasVisible;
};
```

**Frustum Culling** — `Camera::GetVisibleEntities()`에서 `BoundingFrustum`과 각 청크 `AABB`를 교차 판정합니다.  
`DISJOINT`이면 청크 전체를 건너뜁니다. AABB는 엔티티 변화 시에만 `RebuildAABB()`로 갱신됩니다.

**Face Visibility Occlusion Culling** — `_positionMap: unordered_map<uint64, Entity*>`에  
Mesh 블록(큐브) 위치를 20비트×3 팩킹한 키로 저장합니다.

```cpp
// 20비트 × 3축, 바이어스 524288
static uint64 PositionKey(const Vec3& pos) {
    const int32 ix = static_cast<int32>(std::round(pos.x)) + 524288;
    const int32 iy = static_cast<int32>(std::round(pos.y)) + 524288;
    const int32 iz = static_cast<int32>(std::round(pos.z)) + 524288;
    return (uint64(ix) << 40) | (uint64(iy) << 20) | uint64(iz);
}
```

`CollectVisible` 시 Mesh 블록은 6방향 이웃 전체를 `HasSolidBlockAt()`(O(1))로 조회해  
모두 채워진 경우 `outEntities`에 추가하지 않습니다.  
Model 블록(나무·버섯 등 비균일 형태)은 항상 통과합니다.

**레이 피킹** — `PickBlock / PickBlocks`는 AABB 수준 조기 탈출 후 엔티티별 교차 검사를 수행합니다.  
`PickBlocks`는 Priming / Floor / Mushroom 세 채널을 단일 순회로 처리합니다.

---

## 인스턴싱 시스템

### InstanceID

동일 메시 + 머티리얼 + 청크를 공유하는 엔티티를 하나의 인스턴싱 그룹으로 자동 분류합니다.

```cpp
struct InstanceID {
    uint64 resource0;  // Mesh 포인터 해시
    uint64 resource1;  // Material 포인터 해시
    uint64 bucket;     // 청크 키
};

// Fibonacci 해시를 사용한 해시 함수
h ^= hash(resource1) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
```

### SmartRebuildMeshGroups

Mesh 블록 인스턴싱 데이터의 부분 재빌드 전략입니다.

```
전체 재빌드 조건 : _bDirty || _meshDirty  (엔티티 추가/제거, 씬 전체 변경)
부분 재빌드 조건 : _meshGroupDirty       (일부 그룹만 변형)
건너뛰기 조건   : 엔티티 수 불변 && !_partialDirtyMesh.count(id)
```

`_tmpMeshCache (EntityCache)` 를 영구 멤버로 선언해 매 프레임 `unordered_map` 힙 할당/해제를 방지합니다.

```cpp
// swap + clear 패턴: 버킷 배열 용량을 프레임 간 유지
std::swap(_meshCache, _tmpMeshCache);
_tmpMeshCache.clear();   // 원소만 제거, 버킷 배열 보존
```

`move`를 사용하면 `_tmpMeshCache`가 최소 상태(버킷 1개)로 리셋되어  
다음 프레임 `insert` 시 버킷 재성장이 발생합니다.  
`swap` 이후 `clear`를 쓰면 `_tmpMeshCache`가 이전 `_meshCache`의 충분히 성장한 버킷 배열을 이어받아  
다음 프레임 `insert`에서 재할당이 일어나지 않습니다.

### RenderStats

`InstancingManager`는 매 프레임 다음 통계를 추적합니다.

```cpp
struct RenderStats {
    uint32 modelDrawCalls;
    uint32 meshDrawCalls;
    uint32 totalDrawCalls;
    uint32 totalInstances;
    uint32 dynamicBuffers;
    uint32 staticBuffers;
    uint32 meshGroupsRebuilt;   // 실제 재빌드된 그룹 수
    uint32 meshGroupsSkipped;   // 건너뛴 그룹 수
};
```

`DumpInstancingStats()`로 로그 출력 가능합니다. `DebugHUD`에서 실시간 표시됩니다.

---

## 버퍼 관리 전략

### DynamicInstancePool — Persistent Map

매 프레임 동적 인스턴스 데이터를 CPU 스톨 없이 기록합니다.

```cpp
void DynamicInstancePool::BeginFrame()  // D3D11_MAP_WRITE_DISCARD 1회, _mappedPtr 저장
uint32 Append(const InstancingData*, uint32 count)  // memcpy만 수행
void DynamicInstancePool::EndFrame()    // Unmap 1회
```

`WRITE_DISCARD`는 드라이버가 현재 슬롯 버퍼에 새 물리 메모리를 반환하므로  
GPU가 이전 슬롯 데이터를 읽는 중이어도 CPU 기록이 즉시 가능합니다.  
3-Slot Ring Buffer로 GPU가 3프레임 이전 데이터까지 읽을 수 있는 윈도우를 보장합니다.

### InstancingBuffer — Tiered Allocation

정적(Static) 인스턴싱 버퍼는 사용량에 따라 4단계 티어로 할당합니다.

| 티어 | 크기 | 적합 용도 |
|------|------|----------|
| kTierSmall | 64 | 희박한 청크 |
| kTierMedium | 512 | 일반 블록 그룹 |
| kTierLarge | 4,096 | 밀집 씬 |
| kTierMax | 10,000 | 전체 씬 |

`NextTier(count)`는 요청 카운트를 초과하는 최소 티어를 반환합니다.  
불필요하게 큰 버퍼를 할당하지 않아 VRAM 낭비를 줄입니다.

**SetData 직접 포인터 경로** — 동적 버퍼에서 `SetData`를 호출할 때  
`_data(vector)` 복사를 건너뛰고 외부 포인터를 직접 보관합니다.  
`UploadData()`가 `Pool->Append()`를 호출할 때 외부 버퍼에서 Pool 버퍼로 `memcpy` 1회만 실행됩니다.

```cpp
// SetData 경로별 동작
if (_isDynamic && pendingPtr경로)  → memcpy 1회 (외부 → Pool)
if (_isDynamic && AddData누적경로) → memcpy 1회 (_data → Pool)
if (!_isDynamic)                   → memcpy 1회 (_data → 정적버퍼)
```

### Block Entity Pool

블록 배치·제거 시 런타임 `new/delete`를 제거합니다.

```
시작 시 사전 할당 : Mesh 블록 128개, Model 블록 16개/종류
배치 시 : _meshPool.pop_back() → Transform/MeshRenderer/AABBCollider 재구성 → Scene::AddDirect()
제거 시 : ChunkManager::Unregister() → Scene::Detach() → _meshPool.push_back()
```

`Scene::AddDirect()`는 `AddImmediate()`를 직접 호출해 `Awake/Start`를 재실행하지 않습니다.  
`Scene::Detach()`는 `swap-and-pop`으로 `_objects`에서 엔티티를 분리하고 `unique_ptr<Entity>`를 반환합니다.

---

## 섀도우 패스

### 2-Cascade Shadow Map

단일 `Texture2DArray (ArraySize=2)`로 두 카스케이드를 관리합니다.

```
Cascade 0 (near) : 카메라 반경 kNearCascadeRadius=25.0f 기준 구(sphere) tight fit VP
Cascade 1 (far)  : 씬 전체 정적 엔티티 AABB 기반 VP
```

**리소스 생성**

```cpp
// ArraySize=2 Depth Texture
td.Format    = DXGI_FORMAT_R32_TYPELESS;
td.ArraySize = kCascadeCount;           // 2
td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

// 슬라이스별 DSV
dsvd.ViewDimension                  = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
dsvd.Texture2DArray.FirstArraySlice = cascadeIdx;   // 0 또는 1
dsvd.Texture2DArray.ArraySize       = 1;

// 단일 SRV (Texture2DArray 전체)
srvd.ViewDimension                  = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
srvd.Format                         = DXGI_FORMAT_R32_FLOAT;
```

**Render 흐름**

```
BuildGroups(entities)          ← 엔티티 순회 1회
ComputeCascadeVPs(camPos)      ← 2개 VP 계산
for (i = 0; i < 2; ++i)
    RenderCascade(i, VP[i])    ← DSV[i] 바인딩, cbuffer 업데이트, Draw
```

**Depth VS** — 런타임에 문자열로부터 직접 컴파일 (`D3DCompile`)합니다.  
스태틱 메시와 스키닝 메시 각각의 Depth VS가 내장 문자열 리터럴로 포함되어 있습니다.

**픽셀 셰이더 카스케이드 선택**

```hlsl
float dist    = length(worldPos - CameraPosition());
int   cascade = (dist < CascadeSplit) ? 0 : 1;

float4 lsPos = mul(float4(worldPos, 1.0f), LightVP[cascade]);
shadow += ShadowMap.SampleCmpLevelZero(
    ShadowSampler,
    float3(uv + offset * ShadowTexelSize, (float)cascade),
    depth);
```

Shadow Sampler는 `COMPARISON_MIN_MAG_LINEAR_MIP_POINT`, `AddressUV=BORDER`, `BorderColor=white(1,1,1,1)`, `ComparisonFunc=LESS_EQUAL`로 설정합니다.  
범위를 벗어난 UV는 흰색(섀도우 없음)으로 처리됩니다.

---

## UI 시스템

`UIManager`는 3D 렌더 패스 이후 Orthographic 독립 패스로 실행됩니다.

```cpp
// Draw Command 배치
struct DrawCmd {
    uint32 indexOffset;
    uint32 indexCount;
    uint32 pass;          // 0=solid color, 1=textured
    ComPtr<ID3D11ShaderResourceView> srv;
};

// API
AddRect(x, y, w, h, color)
AddRectBorder(x, y, w, h, color, thickness)
AddTexturedRect(x, y, w, h, tint, texHandle)
AddText(text, x, y, w, h, color, fontSize, fontName)
```

텍스트는 GDI+ `Graphics::DrawString`으로 오프스크린 비트맵에 렌더한 뒤  
`CreateTexture2D` + `CreateShaderResourceView`로 D3D11 텍스처로 변환해 캐싱합니다.

`_psColor`(단색)와 `_psTex`(텍스처) 두 픽셀 셰이더로 pass 값에 따라 분기하며,  
모든 쿼드를 단일 VB/IB에 누적한 뒤 DrawCmd 순서대로 `DrawIndexed`를 호출합니다.

---

## 엔티티 컴포넌트 시스템

### Entity

컴포넌트를 고정 크기 배열에 저장해 `GetComponent<T>()`가 컴파일 타임 인덱스 기반 O(1) 조회를 수행합니다.

```cpp
std::array<std::unique_ptr<Component>, FIXED_COMPONENT_COUNT> _components;
std::vector<std::unique_ptr<MonoBehaviour>>                   _scripts;
```

`GetComponent<T>()`는 `ComponentTypeOf<T>::kType`으로 배열 인덱스를 정적으로 결정합니다.  
`MonoBehaviour` (스크립트) 는 별도 `_scripts` 벡터에 위치합니다.

### Camera

```cpp
struct CullStats {
    uint32 totalEntities;
    uint32 visibleEntities;
    uint32 culledEntities;
    uint32 meshRebuildCount;
    uint32 modelRebuildCount;
};
```

`_meshVisibilityHash`, `_modelVisibilityHash`, `_prevCamPos`, `_prevCamYaw`를 추적해  
카메라가 정지 상태일 때 불필요한 소트와 재빌드를 건너뜁니다.

레이어 기반 컬링 마스크 (`uint32 _cullingMask`)로 계층별 가시성을 제어합니다.

### Scene 뮤테이션 버퍼

이터레이션 중 엔티티 추가·제거를 안전하게 처리합니다.

```cpp
std::vector<std::unique_ptr<Entity>> _pendingAdds;
std::vector<Entity*>                 _pendingRemoves;
uint32                               _iterationDepth;
```

이터레이션 중에는 `_pendingAdds / _pendingRemoves`에 기록하고  
이터레이션 종료 후 `FlushPendingMutations()`에서 일괄 반영합니다.

`Scene::_objects`는 `vector<unique_ptr<Entity>>`로 연속 메모리 배치를 유지합니다.  
삭제 시 swap-and-pop 패턴으로 O(1) 제거를 수행합니다.

### Collision Channel

```cpp
enum class CollisionChannel : uint8 {
    None      = 0,
    Default   = 1 << 0,
    Character = 1 << 1,
    Priming   = 1 << 2,   // 블록 배치 대상 (나무 등)
    Mushroom  = 1 << 3,   // 버섯 블록
    Floor     = 1 << 4,   // 바닥 / Ground plane
    All       = 0xFF
};
```

`BlockRecord`의 `pickableMask`와 `faceMask`로 블록 종류별 피킹 채널과 배치 허용 면을 세밀하게 제어합니다.

```cpp
enum class PlaceFace : uint8 {
    Top    = 1 << 0,   // HitNormal.y >  0.7f
    Side   = 1 << 1,   // |HitNormal.y| < 0.7f
    Bottom = 1 << 2,   // HitNormal.y < -0.7f
    All    = 0xFF
};
```

---

## 블록 데이터 정의

모든 블록 속성은 `BlockMaster.json`에서 정의하며 `BlockTable::Load()`로 파싱합니다.

```cpp
struct BlockRecord {
    int32            typeId;
    std::wstring     key;           // "priming_block", "floor_block" 등
    std::wstring     label;
    std::wstring     paletteLabel;
    Color            color;
    bool             isEraser;
    BlockRenderType  renderType;    // Mesh | Model
    std::wstring     modelName;     // FBX 파일명 (Model 타입)
    float            modelScale;    // 기본 0.01f
    BlockUVRect      paletteRect;   // 팔레트 UI용 UV (HLSL 16바이트 정렬)
    ColliderSize     collider;      // Small | Unit | Tall | Wide
    CollisionChannel ownChannel;
    uint8            pickableMask;  // 어떤 채널에서 피킹 가능한가
    uint8            faceMask;      // 어떤 면에 배치 가능한가
};
```

`BlockUVRect`는 HLSL `float4` 정렬 요건을 충족하도록 `static_assert(sizeof == 16)`으로 보장합니다.

`ColliderSize`별 하프 익스텐트는 `BlockPlacer::GetHalfExtents()`에서 반환합니다.

---

## 씬 직렬화

`SceneSerializer`는 nlohmann/json을 사용해 씬을 저장하고 불러옵니다.

저장 시 `IBlockPlacer::GetPlacedBlocks()`를 통해 `PlacedBlockRecord` 목록을 수집합니다.  
`PlacedBlockRecord`는 `{ x, y, z, type }` 순수 POD 구조체이므로  
렌더러 상태와 완전히 독립적입니다.

---

## 셰이더 구조

### 상수 버퍼 레지스터 배치

| 레지스터 | 구조체 | 업데이트 주기 | 내용 |
|---------|--------|------------|------|
| b0 | ShadowBuffer | Per-Frame | LightVP[2], ShadowBias, ShadowTexelSize, CascadeSplit |
| b1 | GlobalBuffer | Per-Level | V, P, VP, VInv |
| b2 | TransformBuffer | Per-Object | W |
| — | MaterialDesc | Per-Object | ambient, diffuse, specular, emissive |
| — | LightDesc | Per-Frame | ambient, diffuse, specular, emissive, direction |

### 텍스처 레지스터 (BlockShader)

| 레지스터 | 리소스 | 설명 |
|---------|--------|------|
| t0 | `Texture2D g_BlockAtlas` | 블록 텍스처 아틀라스 |
| t1 | `StructuredBuffer<float4> g_AtlasRects` | 블록별 UV 좌표 (uOffset, vOffset, uScale, vScale) |

### 인스턴스 버퍼 입력 레이아웃

```hlsl
// 슬롯 0: 정점 데이터
float4 position : POSITION
float2 uv       : TEXCOORD
float3 normal   : NORMAL
float3 tangent  : TANGENT

// 슬롯 1: 인스턴스 데이터 (per-instance step rate = 1)
matrix world         : INST          // 월드 변환 행렬 (4 × float4)
uint   materialIndex : INST_MATERIAL // g_AtlasRects 조회 인덱스
```

### BlockShader 라이팅 모델

Blinn-Phong + Rim Light + PCF Shadow

```hlsl
// 텍스처 아틀라스 UV 재매핑
float4 rect    = g_AtlasRects[materialIndex];
float2 atlasUV = uv * rect.zw + rect.xy;

// Blinn-Phong
float4 ambient  = baseColor * GlobalLight.ambient  * Material.ambient;
float4 diffuse  = baseColor * NdotL * GlobalLight.diffuse * Material.diffuse;
float  spec     = pow(saturate(dot(R, E)), 16.0f);       // Blinn-Phong 지수 16
float4 specular = GlobalLight.specular * Material.specular * spec;

// Rim Light
float  rim     = 1.0f - saturate(dot(E, N));
float4 emissive = GlobalLight.emissive * Material.emissive * pow(rim, 2.0f);

// PCF 3×3 Shadow (9 샘플)
float shadow    = ComputeShadowFactor(worldPos);   // 0.0 ~ 1.0
float4 final    = ambient + (diffuse + specular) * shadow + emissive;
```

### State Shadow Cache

`Graphics` 클래스가 파이프라인 상태를 캐싱해 중복 API 호출을 방지합니다.

```cpp
struct ShadowStateCache {
    ID3D11RasterizerState*   rsState;    bool rsValid;
    ID3D11DepthStencilState* dssState;   bool dssValid;   UINT stencilRef;
    ID3D11BlendState*        blendState; bool blendValid; FLOAT blendFactor[4]; UINT sampleMask;
    ID3D11Buffer*            vb0;        bool vb0Valid;   UINT vb0Stride; UINT vb0Offset;
    ID3D11Buffer*            ib;                         DXGI_FORMAT ibFormat;
};
```

`SetRasterizerState()`, `SetDepthStencilState()`, `SetBlendState()`, `SetVertexBuffer()`, `SetIndexBuffer()`  
각 함수에서 현재 캐시 값과 비교해 동일하면 D3D11 API 호출을 건너뜁니다.  
셰이더 전환 시 `InvalidateStateCache()`로 전체 캐시를 무효화합니다.

---

## 최적화 목록

| 항목 | 구현 위치 | 내용 |
|------|---------|------|
| Hardware Instancing | `InstancingManager`, `InstancingBuffer` | N개 블록 → 1 DrawCall |
| Persistent Map | `DynamicInstancePool` | Map/Unmap 프레임당 1회, GPU 스톨 Zero |
| Ring Buffer 3-Slot | `DynamicInstancePool` | GPU 읽기 중 CPU 쓰기 가능 |
| PickBlocks 통합 | `BlockPlacer::Update` | 3채널 레이캐스트 3회→1회 |
| SmartRebuild | `InstancingManager` | dirty 그룹만 재빌드 |
| swap+clear 패턴 | `SmartRebuildMeshGroups` | unordered_map 버킷 용량 보존 |
| PruneEmptyGroups 조건부 | `InstancingManager` | `_hasPendingPrune` 플래그, 정적 프레임 생략 |
| Block Entity Pool | `BlockPlacer` | 시작 시 Mesh 128 + Model 16 사전 할당 |
| Frustum Culling | `ChunkManager::CollectVisible` | 청크 AABB 기준, AABB Lazy Rebuild |
| Face Occlusion Culling | `ChunkManager::CollectVisible` | 6방향 위치 해시 O(1) |
| Tiered Buffer | `InstancingBuffer::NextTier` | 64/512/4096/10000 최소 티어 선택 |
| SetData 직접 포인터 | `InstancingBuffer::SetData` | 동적 버퍼 memcpy 2회→1회 |
| Scene vector 전환 | `Scene::_objects` | unordered_set→vector, 캐시 적중률 향상 |
| 2-Cascade CSM | `ShadowPass` | 근거리 그림자 해상도 향상 |
| State Shadow Cache | `Graphics` | 중복 D3D11 상태 변경 API 호출 방지 |
| UI Orthographic 패스 | `UIManager` | 3D 패스와 분리, Depth Test OFF |
| 상수 버퍼 빈도 분할 | `ShaderDesc.h` | Per-Level/Frame/Object 분리 |

---

## 프로젝트 구조

```
JellytoStudio/
├── Client/
│   └── Source/
│       ├── Core/          pch.h (프리컴파일 헤더)
│       ├── Data/          BlockTable.h/cpp — JSON 파싱, BlockRecord 조회
│       ├── Main/          EditorApp, MainApp, Actors — 진입점 및 씬 구성
│       ├── Resource/      BlockMaterialProvider — 머티리얼·셰이더 캐싱
│       ├── Scripts/       BlockPlacer, IsometricCameraController, PointClickController
│       └── UI/            PaletteWidget, InventoryWidget, InventoryData, DebugHUD, StressPanel
│
└── Engine/
    ├── Shaders/
    │   ├── BlockShader.hlsl           인스턴싱 + 아틀라스 + Blinn-Phong + Rim + CSM
    │   ├── ShaderCommon.hlsli         공용 cbuffer 정의, ComputeShadowFactor
    │   ├── Lighting.hlsli             라이팅 유틸 함수
    │   ├── StaticMeshShader.hlsl      FBX 스태틱 메시
    │   ├── SkinnedMeshShader.hlsl     Tween 블렌딩 스키닝 애니메이션
    │   ├── UIShader.hlsl              Orthographic UI (psColor / psTex)
    │   ├── ColliderDebugShader.hlsl   AABB 와이어프레임 디버그
    │   └── SkySphereShader.hlsl       스카이 스피어
    │
    └── Source/
        ├── App/
        │   ├── Application.h/cpp      Win32 윈도우, 메인 루프, 메뉴 (AppMenuCmd)
        │   ├── DetailWindow           엔티티 상세 정보 (렌더러 타입, 본 수, 애님 정보)
        │   ├── ItemWindow             블록 목록 및 배치 제어
        │   ├── ChunkDebugWindow       청크 통계 (ChunkSnapshot, 가시 청크 수)
        │   └── Managers/WindowManager ImGui 윈도우 생명주기
        ├── Audio/
        │   ├── AudioManager           XAudio2 기반 사운드 재생
        │   └── AudioDataTable         사운드 리소스 매핑
        ├── Core/
        │   ├── Framework.h            전역 싱글턴 매크로 (DECLARE_SINGLE, GET_SINGLE)
        │   ├── InputManager           키보드·마우스 상태 폴링
        │   └── TimeManager            DeltaTime, FPS
        ├── Entity/
        │   ├── Entity.h/cpp           컴포넌트 배열, GetComponent O(1)
        │   ├── Actor.h/cpp            트랜스폼 조합 헬퍼
        │   └── Components/
        │       ├── Transform           위치·회전·스케일, 월드 행렬
        │       ├── Camera              Frustum Culling, CullStats, 정렬 해시
        │       ├── MeshRenderer        InstanceID 계산, FillPacket
        │       ├── ModelRenderer       FBX 인스턴스 렌더 (ModelScaleMatrix)
        │       ├── ModelAnimator       Tween 블렌딩, InstancedTweenDesc
        │       ├── AnimStateMachine    상태 전이 그래프
        │       ├── Light               방향광 (LightDesc)
        │       └── Collider/
        │           ├── AABBCollider    AABB 생성, static 플래그, 채널
        │           ├── BaseCollider    충돌 판정 기반 클래스
        │           └── CollisionChannel  비트 플래그 채널, PlaceFace
        ├── Graphics/
        │   ├── Graphics.h/cpp          D3D11 디바이스, SwapChain, State Cache
        │   ├── ShadowPass.h/cpp        2-Cascade Shadow Map, 내장 Depth VS
        │   ├── RenderPacket.h          POD: matWorld, materialIndex
        │   └── Managers/
        │       └── InstancingManager   SmartRebuild, 업로드/드로우 페이즈 분리, RenderStats
        ├── Pipeline/
        │   ├── DynamicInstancePool     Persistent Map, 3-Slot Ring Buffer
        │   ├── InstancingBuffer        Tiered 정적 버퍼, SetData 직접 포인터 경로
        │   ├── Shader.h/cpp            ID3DX11Effect 래퍼, Push* 함수
        │   ├── ConstantBuffer.h        cbuffer 업로드 헬퍼
        │   └── VertexBuffer.h          VB/IB 생성 헬퍼
        ├── Scene/
        │   ├── Scene.h/cpp             엔티티 생명주기, Detach/AddDirect, 뮤테이션 버퍼
        │   ├── ChunkManager.h/cpp      16.0f 청크, Frustum + Occlusion Culling, _positionMap
        │   ├── SceneSerializer         JSON 저장·불러오기 (nlohmann/json)
        │   ├── BlockPlacerInterface.h  IBlockPlacer 인터페이스, PlacedBlockRecord POD
        │   └── PickUtils.h             PickBlocks 멀티채널 레이캐스트 유틸
        ├── Types/
        │   └── ShaderDesc.h            GlobalDesc, LightDesc, ShadowDesc(CSM), BoneDesc, TweenDesc
        └── UI/
            └── UIManager.h/cpp         Orthographic 패스, DrawCmd 배치, GDI+ 텍스트 → SRV
```

---

*C++17 · DirectX 11 · HLSL · WRL::ComPtr · Hardware Instancing · Ring Buffer · CSM · Face Occlusion Culling · nlohmann/json*