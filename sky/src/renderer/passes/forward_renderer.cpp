#include "forward_renderer.h"

#include "graphics/vulkan/vk_pipelines.h"
#include "renderer/frustum_culling.h"

namespace sky
{
void ForwardRendererPass::init(const gfx::Device &device, VkFormat format) 
{
    const auto vertexShader = gfx::vkutil::loadShaderModule("shaders/mesh.vert.spv", device.getDevice());
    const auto fragShader = gfx::vkutil::loadShaderModule("shaders/mesh.frag.spv", device.getDevice());

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(PushConstants),
    };

    const auto pushConstantRanges = std::array{bufferRange};
    const auto layouts = std::array<VkDescriptorSetLayout, 2>{
        device.getBindlessDescSetLayout(), 
        device.getStorageBufferLayout()
    };

    m_pInfo.pipelineLayout = gfx::vkutil::createPipelineLayout(device.getDevice(), layouts, pushConstantRanges);

    m_pInfo.pipeline = gfx::PipelineBuilder{m_pInfo.pipelineLayout}
                         .setShaders(vertexShader, fragShader)
                         .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                         .setPolygonMode(VK_POLYGON_MODE_FILL)
                         .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
                         .setMultisamplingNone()
                         .disableBlending()
                         .enableDepthTest(true, VK_COMPARE_OP_LESS)
                         .setColorAttachmentFormat(format)
                         .setDepthFormat(VK_FORMAT_D32_SFLOAT)
                         .build(device.getDevice());
}

void ForwardRendererPass::draw(
    gfx::Device &device,
    gfx::CommandBuffer cmd, 
    VkExtent2D extent,
    Camera &camera,
	const gfx::AllocatedBuffer &sceneDataBuffer,
    const MeshCache &meshCache,
    const std::vector<MeshDrawCommand> &drawCommands)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pInfo.pipeline);
    VkDescriptorSet descriptorSets[] = {
        device.getBindlessDescSet(),
        device.getStorageBufferDescSet(),
    };
    device.bindDescriptorSets(cmd, m_pInfo.pipelineLayout, descriptorSets);

    // set dynamic viewport and scissor
    const auto viewport = VkViewport{
        .x = 0.f,
		.y = 0.f,
		.width = (float)extent.width,
		.height = (float)extent.height,
		.minDepth = 0.f,
		.maxDepth = 1.f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    const auto scissor = VkRect2D{
        .offset = {0, 0},
        .extent = extent,
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

	const auto frustum = edge::createFrustumFromCamera(camera);
    for (const auto &dc : drawCommands)
    {
        if (!edge::isInFrustum(frustum, dc.worldBoundingSphere)) continue;

        if (dc.isVisible)
        {    
			const auto &mesh = meshCache.getMesh(dc.meshId);

			const auto pushConstants = PushConstants{
				.transform = dc.modelMatrix,
                .uniqueId = dc.uniqueId,
                .sceneDataBuffer = sceneDataBuffer.address,
				.vertexBuffer = mesh.vertexBuffer.address,
                .materialId = dc.material, 
			};

			vkCmdPushConstants(cmd, 
                m_pInfo.pipelineLayout, 
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
                0, 
                sizeof(PushConstants), 
                &pushConstants);
			vkCmdBindIndexBuffer(cmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(cmd, mesh.numIndices, 1, 0, 0, 0);
        }
	}
}

void ForwardRendererPass::draw2(gfx::Device &device, 
    gfx::CommandBuffer cmd, 
    VkExtent2D extent, 
    const gfx::AllocatedBuffer &sceneDataBuffer,  
	const MeshCache &meshCache,
    std::vector<MeshID> meshes,
    MaterialID materialId,
    bool useDefaultMaterial)
{
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pInfo.pipeline);
    VkDescriptorSet descriptorSets[] = {
        device.getBindlessDescSet(),
        device.getStorageBufferDescSet(),
    };
    device.bindDescriptorSets(cmd, m_pInfo.pipelineLayout, descriptorSets);

    // set dynamic viewport and scissor
    const auto viewport = VkViewport{
        .x = 0.f,
		.y = 0.f,
		.width = (float)extent.width,
		.height = (float)extent.height,
		.minDepth = 0.f,
		.maxDepth = 1.f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    const auto scissor = VkRect2D{
        .offset = {0, 0},
        .extent = extent,
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    for (const auto &meshId : meshes)
    {
		const auto &mesh = meshCache.getMesh(meshId);
		const auto pushConstants = PushConstants{
			.transform = glm::mat4{1.f},
			.uniqueId = (uint32_t)-1,
			.sceneDataBuffer = sceneDataBuffer.address,
			.vertexBuffer = mesh.vertexBuffer.address,
			.materialId = useDefaultMaterial ? mesh.materialId : materialId, 
		};

		vkCmdPushConstants(cmd, 
			m_pInfo.pipelineLayout, 
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
			0, 
			sizeof(PushConstants), 
			&pushConstants);
		vkCmdBindIndexBuffer(cmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(cmd, mesh.numIndices, 1, 0, 0, 0);
    }
}

void ForwardRendererPass::cleanup(const gfx::Device &device) 
{
    vkDestroyPipeline(device.getDevice(), m_pInfo.pipeline, nullptr);
    vkDestroyPipelineLayout(device.getDevice(), m_pInfo.pipelineLayout, nullptr);
}
} // namespace sky