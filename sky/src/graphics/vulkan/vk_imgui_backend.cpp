#include "vk_imgui_backend.h"

#include <imgui.h>
#include <IconsFontAwesome5.h>

#include "vk_device.h"
#include "vk_pipelines.h"

namespace
{
static const int MAX_IDX_COUNT = 1000000;
static const int MAX_VTX_COUNT = 1000000;

// this is the format we expect
struct ImGuiVertexFormat
{
    glm::vec2 position;
    glm::vec2 uv;
    std::uint32_t color;
};
static_assert(sizeof(ImDrawVert) == sizeof(ImGuiVertexFormat), "ImDrawVert and ImGuiVertexFormat size mismatch");
static_assert(sizeof(ImDrawIdx) == sizeof(std::uint16_t) || sizeof(ImDrawIdx) == sizeof(std::uint32_t),
              "Only uint16_t or uint32_t indices are supported");

std::uint32_t toBindlessTextureId(ImTextureID id)
{
    return static_cast<uint32_t>(id);
}
} // namespace

namespace sky::gfx
{
void ImGuiBackend::init(Device &gfxDevice, VkFormat swapchainFormat)
{
    idxBuffer.init(gfxDevice, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, sizeof(ImDrawIdx) * MAX_IDX_COUNT,
                   gfx::FRAME_OVERLAP, "ImGui index buffer");
    vtxBuffer.init(gfxDevice, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                   sizeof(ImDrawVert) * MAX_VTX_COUNT, gfx::FRAME_OVERLAP, "ImGui vertex buffer");

    auto &io = ImGui::GetIO();
    io.BackendRendererName = "Sky ImGui Backend";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    s_fonts["bold"] = io.Fonts->AddFontFromFileTTF("res/fonts/IBM_Plex_Sans/IBMPlexSans-Bold.ttf", 27.0f);
    s_fonts["h4:bold"] = io.Fonts->AddFontFromFileTTF("res/fonts/IBM_Plex_Sans/IBMPlexSans-Bold.ttf", 35.0f);

    s_fonts["h2"] = io.Fonts->AddFontFromFileTTF("res/fonts/IBM_Plex_Sans/IBMPlexSans-Regular.ttf", 55.0f);
    s_fonts["h3"] = io.Fonts->AddFontFromFileTTF("res/fonts/IBM_Plex_Sans/IBMPlexSans-Regular.ttf", 45.0f);
    s_fonts["sm"] = io.Fonts->AddFontFromFileTTF("res/fonts/IBM_Plex_Sans/IBMPlexSans-Regular.ttf", 25.0f); 
    
	float iconFontSize = 33.f * 2.0f / 3.0f;
    s_fonts["h4"] = io.Fonts->AddFontFromFileTTF("res/fonts/IBM_Plex_Sans/IBMPlexSans-Regular.ttf", 33.0f);
	// Define the Font Awesome icon range
    static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = iconFontSize;

    // Merge icons into the regular font
    io.Fonts->AddFontFromFileTTF("res/fonts/" FONT_ICON_FILE_NAME_FAS, iconFontSize, &icons_config, icons_ranges);

    setDarkThemeColors();

    // upload font
    {
        auto *pixels = static_cast<std::uint8_t *>(nullptr);
        int width = 0;
        int height = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        fontTextureId = gfxDevice.createImage(
            {
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                .extent = VkExtent3D{(std::uint32_t)width, (std::uint32_t)height, 1},
            }, pixels);
        io.Fonts->SetTexID(fontTextureId);
    }

    const auto &device = gfxDevice.getDevice();

    const auto vertexShader = vkutil::loadShaderModule("shaders/imgui.vert.spv", device);
    const auto fragShader = vkutil::loadShaderModule("shaders/imgui.frag.spv", device);

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(PushConstants),
    };

    const auto layouts = std::array{gfxDevice.getBindlessDescSetLayout()};

    const auto pushConstantRanges = std::array{bufferRange};
    pipelineLayout = vkutil::createPipelineLayout(device, layouts, pushConstantRanges);

    pipeline = PipelineBuilder{pipelineLayout}
                   .setShaders(vertexShader, fragShader)
                   .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                   .setPolygonMode(VK_POLYGON_MODE_FILL)
                   .disableCulling()
                   .setMultisamplingNone()
                   // see imgui.frag for explanation of this blending state
                   .enableBlending(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                                   VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
                   .setColorAttachmentFormat(swapchainFormat)
                   .disableDepthTest()
                   .build(device);
}

void ImGuiBackend::draw(VkCommandBuffer cmd, 
    Device &gfxDevice, 
    VkImageView swapchainImageView,
    VkExtent2D swapchainExtent,
    const ImDrawData *drawData)
{
    assert(drawData);
    if (drawData->TotalVtxCount == 0)
    {
        return;
    }

    if (drawData->TotalIdxCount > MAX_IDX_COUNT || drawData->TotalVtxCount > MAX_VTX_COUNT)
    {
        printf("ImGuiBackend: too many vertices/indices to render (max indices = %d, max "
               "vertices = %d), buffer resize is not yet implemented.\n",
               MAX_IDX_COUNT, MAX_VTX_COUNT);
        assert(false && "TODO: implement ImGui buffer resize");
        return;
    }

    copyBuffers(cmd, gfxDevice, drawData);

    const auto renderInfo = vkutil::createRenderingInfo({
        .renderExtent = swapchainExtent,
        .colorImageView = swapchainImageView,
    });
    vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	VkDescriptorSet descriptorSets[] = {
        gfxDevice.getBindlessDescSet(),
    };
    gfxDevice.bindDescriptorSets(cmd, pipelineLayout, descriptorSets);

    const auto targetWidth = static_cast<float>(swapchainExtent.width);
    const auto targetHeight = static_cast<float>(swapchainExtent.height);

    gfx::vkutil::setViewportAndScissor(cmd, swapchainExtent);

    const auto clipOffset = drawData->DisplayPos;
    const auto clipScale = drawData->FramebufferScale;

    std::size_t globalIdxOffset = 0;
    std::size_t globalVtxOffset = 0;

    vkCmdBindIndexBuffer(cmd, idxBuffer.getBuffer().buffer, 0,
        sizeof(ImDrawIdx) == sizeof(std::uint16_t) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

    for (int cmdListID = 0; cmdListID < drawData->CmdListsCount; ++cmdListID)
    {
        const auto &cmdList = *drawData->CmdLists[cmdListID];
        for (int cmdID = 0; cmdID < cmdList.CmdBuffer.Size; ++cmdID)
        {
            const auto &imCmd = cmdList.CmdBuffer[cmdID];
            if (imCmd.UserCallback)
            {
                if (imCmd.UserCallback != ImDrawCallback_ResetRenderState)
                {
                    imCmd.UserCallback(&cmdList, &imCmd);
                    continue;
                }
                assert(false && "ImDrawCallback_ResetRenderState is not supported");
            }

            if (imCmd.ElemCount == 0)
            {
                continue;
            }

            auto clipMin = ImVec2((imCmd.ClipRect.x - clipOffset.x) * clipScale.x,
                                  (imCmd.ClipRect.y - clipOffset.y) * clipScale.y);
            auto clipMax = ImVec2((imCmd.ClipRect.z - clipOffset.x) * clipScale.x,
                                  (imCmd.ClipRect.w - clipOffset.y) * clipScale.y);
            clipMin.x = std::clamp(clipMin.x, 0.0f, targetWidth);
            clipMax.x = std::clamp(clipMax.x, 0.0f, targetWidth);
            clipMin.y = std::clamp(clipMin.y, 0.0f, targetHeight);
            clipMax.y = std::clamp(clipMax.y, 0.0f, targetHeight);

            if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
            {
                continue;
            }

            auto textureId = gfxDevice.getWhiteTextureID();
            if (imCmd.TextureId != ImTextureID())
            {
                textureId = toBindlessTextureId(imCmd.TextureId);
                assert(textureId != NULL_IMAGE_ID);
            }
            bool textureIsSRGB = true;
            const auto &texture = gfxDevice.getImage(textureId);
            if (texture.imageFormat == VK_FORMAT_R8G8B8A8_SRGB || texture.imageFormat == VK_FORMAT_R16G16B16A16_SFLOAT)
            {
                // TODO: support more formats?
                textureIsSRGB = false;
            }

            const auto scale = glm::vec2(2.0f / drawData->DisplaySize.x, 2.0f / drawData->DisplaySize.y);
            const auto translate =
                glm::vec2(-1.0f - drawData->DisplayPos.x * scale.x, -1.0f - drawData->DisplayPos.y * scale.y);

            // set scissor
            const auto scissorX = static_cast<std::int32_t>(clipMin.x);
            const auto scissorY = static_cast<std::int32_t>(clipMin.y);
            const auto scissorWidth = static_cast<std::uint32_t>(clipMax.x - clipMin.x);
            const auto scissorHeight = static_cast<std::uint32_t>(clipMax.y - clipMin.y);
            const auto scissor = VkRect2D{
                .offset = {scissorX, scissorY},
                .extent = {scissorWidth, scissorHeight},
            };
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            const auto pcs = PushConstants{
                .vertexBuffer = vtxBuffer.getBuffer().address,
                .textureId = (std::uint32_t)textureId,
                .textureIsSRGB = textureIsSRGB,
                .translate = translate,
                .scale = scale,
            };
            vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                               sizeof(PushConstants), &pcs);

            vkCmdDrawIndexed(cmd, imCmd.ElemCount, 1, imCmd.IdxOffset + globalIdxOffset,
                             imCmd.VtxOffset + imCmd.VtxOffset + globalVtxOffset, 0);
        }

        globalIdxOffset += cmdList.IdxBuffer.Size;
        globalVtxOffset += cmdList.VtxBuffer.Size;
    }

    vkCmdEndRendering(cmd);
}

void ImGuiBackend::copyBuffers(VkCommandBuffer cmd, Device &gfxDevice, const ImDrawData *drawData) const
{
    {
        // sync with previous read
        const auto idxBufferBarrier = VkBufferMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
            .buffer = idxBuffer.getBuffer().buffer,
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        };
        const auto vtxBufferBarrier = VkBufferMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
            .buffer = vtxBuffer.getBuffer().buffer,
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        };
        const std::array<VkBufferMemoryBarrier2, 2> barriers{idxBufferBarrier, vtxBufferBarrier};
        const auto dependencyInfo = VkDependencyInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .bufferMemoryBarrierCount = 2,
            .pBufferMemoryBarriers = barriers.data(),
        };
        vkCmdPipelineBarrier2(cmd, &dependencyInfo);
    }

    const auto currFrameIndex = gfxDevice.getCurrentFrameIndex();
    std::size_t currentIndexOffset = 0;
    std::size_t currentVertexOffset = 0;
    for (int i = 0; i < drawData->CmdListsCount; ++i)
    {
        const auto &cmdList = *drawData->CmdLists[i];
        idxBuffer.uploadNewData(cmd, currFrameIndex, cmdList.IdxBuffer.Data, sizeof(ImDrawIdx) * cmdList.IdxBuffer.Size,
                                sizeof(ImDrawIdx) * currentIndexOffset, false);
        vtxBuffer.uploadNewData(cmd, currFrameIndex, cmdList.VtxBuffer.Data,
                                sizeof(ImDrawVert) * cmdList.VtxBuffer.Size, sizeof(ImDrawVert) * currentVertexOffset,
                                false);

        currentIndexOffset += cmdList.IdxBuffer.Size;
        currentVertexOffset += cmdList.VtxBuffer.Size;
    }

    { // sync write
        const auto idxBufferBarrier = VkBufferMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
            .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
            .buffer = idxBuffer.getBuffer().buffer,
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        };
        const auto vtxBufferBarrier = VkBufferMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
            .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
            .buffer = vtxBuffer.getBuffer().buffer,
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        };
        const std::array<VkBufferMemoryBarrier2, 2> barriers{idxBufferBarrier, vtxBufferBarrier};
        const auto dependencyInfo = VkDependencyInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .bufferMemoryBarrierCount = 2,
            .pBufferMemoryBarriers = barriers.data(),
        };
        vkCmdPipelineBarrier2(cmd, &dependencyInfo);
    }
}

void ImGuiBackend::cleanup(Device &gfxDevice)
{
    vkDestroyPipelineLayout(gfxDevice.getDevice(), pipelineLayout, nullptr);
    vkDestroyPipeline(gfxDevice.getDevice(), pipeline, nullptr);

    idxBuffer.cleanup(gfxDevice);
    vtxBuffer.cleanup(gfxDevice);
}

void ImGuiBackend::setDarkThemeColors()
{
    ImVec4 *colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.148f, 0.148f, 0.148f, 1.f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.078f, 0.078f, 0.078f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3f, 0.3f, 0.3f, 0.6f);

    colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);

    colors[ImGuiCol_Border] = ImVec4(0.078f, 0.078f, 0.078f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);

    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.30f, 0.30f, 0.30f, 0.55f);

    colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);

    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.93f, 0.27f, 0.27f, 0.00f);

    colors[ImGuiCol_DragDropTarget] = ImVec4(0.55f, 0.76f, 0.29f, 1.00f);

    ImGuiStyle &s = ImGui::GetStyle();

    s.WindowMenuButtonPosition = ImGuiDir_None;
    s.GrabRounding = 2.0f;
    s.CellPadding = ImVec2(8, 6);
    s.WindowPadding = ImVec2(4, 4);
    s.ScrollbarRounding = 9.0f;
    s.ScrollbarSize = 15.0f;
    s.GrabMinSize = 32.0f;
    s.TabRounding = 0;
    s.WindowRounding = 4.0f;
    s.ChildRounding = 4.0f;
    s.FrameRounding = 4.0f;
    s.GrabRounding = 0;
    s.FramePadding = ImVec2(8, 4);
    s.ItemSpacing = ImVec2(8, 4);
    s.ItemInnerSpacing = ImVec2(4, 4);
    s.TabRounding = 4.0f;
    s.WindowBorderSize = 0.0f;
    s.IndentSpacing = 12.0f;
    s.ChildBorderSize = 0.0f;
    s.PopupRounding = 4.0f;
    s.FrameBorderSize = 0.0f;
}
}