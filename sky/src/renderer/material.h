#pragma once

#include <skypch.h>

#include <glm/glm.hpp>
#include "graphics/vulkan/vk_types.h"
#include "core/color.h"
#include "core/uuid.h"
#include "asset_management/asset.h"

namespace sky
{
struct MaterialPaths
{
	fs::path     albedoTexture;
	fs::path     normalMapTexture;
	fs::path     metallicsTexture;
	fs::path     roughnessTexture;
	fs::path     ambientOcclusionTexture;
	fs::path     emissiveTexture;
};

struct MaterialData
{
    LinearColor baseColor;
    glm::vec4   metalRoughnessEmissive;
    uint32_t    albedoTex;
    uint32_t    normalTex;
    uint32_t    metallicTex;
    uint32_t    roughnessTex;
    uint32_t    ambientOcclusionTex;
    uint32_t    emissiveTex;

    uint32_t padding[2]; // Padding to ensure 16-byte alignment
};

struct Material
{
    LinearColor baseColor{1.f, 1.f, 1.f, 1.f};
    float       metallicFactor{0.5f};
    float       roughnessFactor{0.5f};
    float       emissiveFactor{0.f};
    float       ambientOcclusionFactor{0.f};

    ImageID     albedoTexture{NULL_IMAGE_ID};
    ImageID     normalMapTexture{NULL_IMAGE_ID};
    ImageID     metallicTexture{NULL_IMAGE_ID};
    ImageID     roughnessTexture{NULL_IMAGE_ID};
    ImageID     ambientOcclusionTexture{NULL_IMAGE_ID};
    ImageID     emissiveTexture{NULL_IMAGE_ID};

    AssetHandle albedoTextureHandle{NULL_UUID};
    AssetHandle normalMapTextureHandle{NULL_UUID};
    AssetHandle metallicTextureHandle{NULL_UUID};
    AssetHandle roughnessTextureHandle{NULL_UUID};
    AssetHandle ambientOcclusionTextureHandle{NULL_UUID};
    AssetHandle emissiveTextureHandle{NULL_UUID};

    std::string name;
};

using MaterialID = uint32_t;
static const MaterialID NULL_MATERIAL_ID = -1;

struct MaterialAsset : public Asset
{
    MaterialAsset(MaterialID id): material(id) {}

    MaterialID material = NULL_MATERIAL_ID;
    AssetType getType() const override { return AssetType::Material; }
};
}