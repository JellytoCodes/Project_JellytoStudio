#ifndef _LIGHT_FX_
#define _LIGHT_FX_
#include "Global.hlsli"

struct LightDesc
{
    float4 ambient;
    float4 diffuse;
    float4 specular;
    float4 emissive;
    float3 direction;
    float  padding;
};

struct MaterialDesc
{
    float4 ambient;
    float4 diffuse;
    float4 specular;
    float4 emissive;
};

cbuffer LightBuffer : register(b3)
{
    LightDesc GlobalLight;
}

cbuffer MaterialBuffer : register(b4)
{
    MaterialDesc Material;
}

Texture2D DiffuseMap;
Texture2D SpecularMap;
Texture2D NormalMap;

float4 ComputeLight(float3 normal, float2 uv, float3 worldPosition)
{
    float4 ambientColor  = 0;
    float4 diffuseColor  = 0;
    float4 specularColor = 0;
    float4 emissiveColor = 0;

    {
        float4 color = GlobalLight.ambient * Material.ambient;
        ambientColor = DiffuseMap.Sample(LinearSampler, uv) * color;
    }

    {
        float4 color = DiffuseMap.Sample(LinearSampler, uv);
        float  value = saturate(dot(-GlobalLight.direction, normalize(normal)));
        diffuseColor = color * value * GlobalLight.diffuse * Material.diffuse;
    }

    {
        float3 R       = reflect(GlobalLight.direction, normal);
        float3 camPos  = CameraPosition();
        float3 E       = normalize(camPos - worldPosition);
        float  value   = saturate(dot(R, E));
        float  spec    = pow(value, 10);
        specularColor  = GlobalLight.specular * Material.specular * spec;
    }

    {
        float3 camPos  = CameraPosition();
        float3 E       = normalize(camPos - worldPosition);
        float  rim     = pow(1.0f - saturate(dot(E, normalize(normal))), 2.0f);
        emissiveColor  = GlobalLight.emissive * Material.emissive * rim;
    }

    return ambientColor + diffuseColor + specularColor + emissiveColor;
}

#endif
