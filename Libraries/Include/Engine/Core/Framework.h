#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <memory>
#include <iostream>
#include <array>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <functional>
#include <string>
#include <sstream>
#include <assert.h>
#include <optional>
#include <filesystem>
#include <fstream>

#include <windows.h>
#include <d3d11.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXTex/DirectXTex.h>
#include <DirectXTex/DirectXTex.inl>
#include <wrl.h>
#include <FX11/d3dx11effect.h>

#include "Utils/SimpleMath.h"

using namespace Microsoft::WRL;
using namespace DirectX;

#include <Assimp/Importer.hpp>
#include <Assimp/scene.h>
#include <Assimp/postprocess.h>

#include "Types/GlobalTypes.h"
#include "Types/VertexData.h"
#include "Types/AsTypes.h"
#include "Types/ShaderDesc.h"

#include "Utils/Geometry/Geometry.h"
#include "Utils/Geometry/GeometryHelper.h"
#include "Utils/json.hpp"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#ifdef _DEBUG
#pragma comment(lib, "DirectXTex/DirectXTex_debug.lib")
#pragma comment(lib, "FX11/Effects11d.lib")
#pragma comment(lib, "Assimp/assimp-vc143-mtd.lib")
#pragma comment(lib, "FMOD/fmodL-vc.lib")
#else
#pragma comment(lib, "DirectXTex/DirectXTex.lib")
#pragma comment(lib, "FX11/Effects11.lib")
#pragma comment(lib, "Assimp/assimp-vc143-mt.lib")
#pragma comment(lib, "FMOD/fmod-vc.lib")
#endif

#define DECLARE_SINGLE(classname)			\
private:									\
	classname() { }							\
public:										\
	static classname* GetInstance()			\
	{										\
		static classname s_instance;		\
		return &s_instance;					\
	}

#define GET_SINGLE(classname)	classname::GetInstance()

#define CHECK(hr) { if(FAILED(hr)) { assert(false); } }
