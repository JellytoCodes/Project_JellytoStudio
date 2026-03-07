#pragma once

#define WIN32_LEAN_AND_MEAN

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "directxtk.lib")

#include <windows.h>
#include <wrl/client.h>
#include <assert.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <SimpleMath.h>

#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>

#define MAIN_WINDOW_WIDTH	1280
#define MAIN_WINDOW_HEIGHT	720

#define SUB_WINDOW_WIDTH	1280
#define SUB_WINDOW_HEIGHT	720

#define CHECK(hr) { if(FAILED(hr)) { assert(false); } }

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace std;

using int8			= __int8;
using int16			= __int16;
using int32			= __int32;
using int64			= __int64;
using uint8			= unsigned __int8;
using uint16		= unsigned __int16;
using uint32		= unsigned __int32;
using uint64		= unsigned __int64;

using Color			= SimpleMath::Color;

using Vec2			= SimpleMath::Vector2;
using Vec3			= SimpleMath::Vector3;
using Vec4			= SimpleMath::Vector4;
using Matrix		= SimpleMath::Matrix;
using Quaternion	= SimpleMath::Quaternion;
using Ray			= SimpleMath::Ray;

#include "Types/VertexData.h"
#include "Types/GlobalTypes.h"