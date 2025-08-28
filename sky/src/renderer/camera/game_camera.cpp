#include "game_camera.h"

#include "scene/scene_manager.h"
#include "core/application.h"
#include <glm/fwd.hpp>

namespace sky 
{
GameCamera::GameCamera(const glm::vec3& position, const glm::quat& rotation)
    : m_position(position), m_rotation(rotation)
{
}

void GameCamera::setPosition(const glm::vec3& position)
{
    m_position = position;
    m_viewDirty = true;
    m_viewProjectionDirty = true;
}

void GameCamera::setRotation(const glm::quat& rotation)
{
    m_rotation = glm::normalize(rotation);
    m_viewDirty = true;
    m_viewProjectionDirty = true;
    m_vectorsDirty = true;
}

void GameCamera::setRotation(const glm::vec3& eulerAngles)
{
    m_rotation = glm::quat(eulerAngles);
    m_viewDirty = true;
    m_viewProjectionDirty = true;
    m_vectorsDirty = true;
}

void GameCamera::translate(const glm::vec3& translation)
{
    m_position += translation;
    m_viewDirty = true;
    m_viewProjectionDirty = true;
}

void GameCamera::rotate(const glm::quat& rotation)
{
    m_rotation = glm::normalize(m_rotation * rotation);
    m_viewDirty = true;
    m_viewProjectionDirty = true;
    m_vectorsDirty = true;
}

void GameCamera::rotate(const glm::vec3& eulerAngle)
{
    glm::quat rotation = glm::quat(eulerAngle);
    rotate(rotation);
}

void GameCamera::lookAt(const glm::vec3& target, const glm::vec3& up)
{
    glm::vec3 forward = glm::normalize(target - m_position);
    glm::vec3 right = glm::normalize(glm::cross(forward, up));
    glm::vec3 cameraUp = glm::cross(right, forward);
    
    glm::mat3 rotationMatrix = glm::mat3(right, cameraUp, -forward);
    m_rotation = glm::normalize(glm::quat_cast(rotationMatrix));
    
    m_viewDirty = true;
    m_viewProjectionDirty = true;
    m_vectorsDirty = true;
}

void GameCamera::setProjectionType(ProjectionType type)
{
    if (m_projectionType != type)
    {
        m_projectionType = type;
        m_projectionDirty = true;
        m_viewProjectionDirty = true;
    }
}

void GameCamera::setPerspective(float fov, float aspect, float nearPlane, float farPlane)
{
    m_projectionType = ProjectionType::Perspective;
    m_fieldOfView = fov;
    m_aspectRatio = aspect;
    m_nearClipPlane = nearPlane;
    m_farClipPlane = farPlane;
    m_projectionDirty = true;
    m_viewProjectionDirty = true;
}

void GameCamera::setOrthographic(float size, float aspect, float nearPlane, float farPlane)
{
    m_projectionType = ProjectionType::Orthographic;
    m_orthographicSize = size;
    m_aspectRatio = aspect;
    m_nearClipPlane = nearPlane;
    m_farClipPlane = farPlane;
    m_projectionDirty = true;
    m_viewProjectionDirty = true;
}

void GameCamera::setFieldOfView(float fov)
{
    m_fieldOfView = glm::clamp(fov, 1.0f, 179.0f);
    if (m_projectionType == ProjectionType::Perspective)
    {
        m_projectionDirty = true;
        m_viewProjectionDirty = true;
    }
}

void GameCamera::setOrthographicSize(float size)
{
    m_orthographicSize = glm::max(size, 0.01f);
    if (m_projectionType == ProjectionType::Orthographic)
    {
        m_projectionDirty = true;
        m_viewProjectionDirty = true;
    }
}

void GameCamera::setNearClipPlane(float nearPlane)
{
    m_nearClipPlane = glm::max(nearPlane, 0.001f);
    m_projectionDirty = true;
    m_viewProjectionDirty = true;
}

void GameCamera::setFarClipPlane(float farPlane)
{
    m_farClipPlane = glm::max(farPlane, m_nearClipPlane + 0.001f);
    m_projectionDirty = true;
    m_viewProjectionDirty = true;
}

void GameCamera::setAspectRatio(float aspect)
{
    auto viewportRatio = m_viewport.z / m_viewport.w;
    m_aspectRatio = glm::max(aspect * viewportRatio, 0.01f);
    m_projectionDirty = true;
    m_viewProjectionDirty = true;
}

void GameCamera::setViewport(float x, float y, float width, float height)
{
    m_viewport = glm::vec4(
        glm::clamp(x, 0.0f, 1.0f),
        glm::clamp(y, 0.0f, 1.0f),
        glm::clamp(width, 0.0f, 1.0f),
        glm::clamp(height, 0.0f, 1.0f)
    );
}

void GameCamera::setViewport(const glm::vec4& viewport)
{
    setViewport(viewport.x, viewport.y, viewport.z, viewport.w);
}

void GameCamera::setDepth(int depth)
{
    m_depth = depth;
}

void GameCamera::setClearFlags(ClearFlags flags)
{
    m_clearFlags = flags;
}

void GameCamera::setBackgroundColor(const glm::vec4& color)
{
    m_backgroundColor = color;
}

glm::vec3 GameCamera::getEulerAngles() const
{
    return glm::degrees(glm::eulerAngles(m_rotation));
}

void GameCamera::updateViewMatrix() const
{
    if (!m_viewDirty) return;
    
    // Use the conjugate (inverse) of the rotation quaternion for the view matrix
    glm::quat invRotation = glm::conjugate(m_rotation);
    glm::mat4 rotationMatrix = glm::mat4_cast(invRotation);
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), -m_position);
    
    // View matrix = R^-1 * T^-1 (rotation inverse then translation inverse)
    m_viewMatrix = rotationMatrix * translationMatrix;
    
    m_viewDirty = false;
}

void GameCamera::updateProjectionMatrix() const
{
    if (!m_projectionDirty) return;
    
    if (m_projectionType == ProjectionType::Perspective)
    {
        // Use Vulkan-style projection (inverted Y, 0-1 depth)
        m_projectionMatrix = glm::perspective(glm::radians(m_fieldOfView), m_aspectRatio, m_nearClipPlane, m_farClipPlane);
    }
    else
    {
        float width = m_orthographicSize * m_aspectRatio;
        float height = m_orthographicSize;
        // Use Vulkan-style projection (inverted Y, 0-1 depth)
        m_projectionMatrix = glm::ortho(-width, width, height, -height, m_nearClipPlane, m_farClipPlane);
    }
    
    m_projectionDirty = false;
}

void GameCamera::updateViewProjectionMatrix() const
{
    if (!m_viewProjectionDirty) return;
    
    if (m_viewDirty) updateViewMatrix();
    if (m_projectionDirty) updateProjectionMatrix();
    
    m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
    m_viewProjectionDirty = false;
}

void GameCamera::updateVectors() const
{
    if (!m_vectorsDirty) return;
    
    glm::mat3 rotationMatrix = glm::mat3_cast(m_rotation);
    m_forward = -rotationMatrix[2]; // Negative Z is forward
    m_right = rotationMatrix[0];    // Positive X is right
    m_up = rotationMatrix[1];       // Positive Y is up
    
    m_vectorsDirty = false;
}

ImageID GameCamera::getPreviewImage()
{
    auto scene = SceneManager::get().getEditorScene();
    auto renderer = Application::getRenderer();
    auto &device = Application::getRenderer()->getDevice();
    auto cmd = device.beginOffscreenFrame();

    static glm::ivec2 size = {420, 280};

    static auto drawImageId = renderer->createNewDrawImage(size, VK_FORMAT_R16G16B16A16_SFLOAT);
    static auto depthImageId = renderer->createNewDepthImage(size);

    auto drawImage = device.getImage(drawImageId);
    auto depthImage = device.getImage(depthImageId);

    static ForwardRendererPass forwardRenderer;
    if (!forwardRenderer.initialized)
    {
        forwardRenderer.init(device, VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT);
    }
    static gfx::NBuffer sceneDataBuffer;
    if (!sceneDataBuffer.initialized)
    {
        sceneDataBuffer.init(
            device,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            sizeof(SceneRenderer::GPUSceneData),
            gfx::FRAME_OVERLAP,
            "scene data");
    }

    auto lightCache = renderer->getLightCache();

    // Calculate new projection matrix with aspect ratio of 420/280
    auto viewportRatio = m_viewport.z / m_viewport.w;
    auto aspectRatio = glm::max(420.f/280.f * viewportRatio, 0.01f);
    auto projection = 
        glm::perspective(glm::radians(m_fieldOfView), aspectRatio, m_nearClipPlane, m_farClipPlane);

    auto viewProjection = projection * m_viewMatrix;

    const auto gpuSceneData = SceneRenderer::GPUSceneData{
		.view = this->getView(),
		.proj = projection,
		.viewProj = viewProjection,
		.cameraPos = glm::vec4(0.f),
        .mousePos = {0.f, 0.f},
		.ambientColor = LinearColorNoAlpha::white(),
		.ambientIntensity = 0.4f,
        .irradianceMapId  = renderer->getIBL().getIrradianceMapId(),
        .prefilterMapId  = renderer->getIBL().getPrefilterMapId(),
        .brdfLutId  = renderer->getIBL().getBrdfLutId(),
		.lightsBuffer = lightCache.getBuffer().address,
        .numLights = (uint32_t)lightCache.getSize(),
		.materialsBuffer = renderer->getMaterialCache().getMaterialDataBufferAddress(),
    };
	sceneDataBuffer.uploadNewData(
		cmd, 
		device.getCurrentFrameIndex(), 
		(void *)&gpuSceneData,
		sizeof(SceneRenderer::GPUSceneData));
    renderer->getMaterialCache().upload(device, cmd);

    gfx::vkutil::transitionImage(cmd, drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    gfx::vkutil::transitionImage(cmd, depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	const auto renderInfo = gfx::vkutil::createRenderingInfo({
        .renderExtent = drawImage.getExtent2D(),
        .colorImageView = drawImage.imageView,
        .colorImageClearValue = m_backgroundColor,
        .depthImageView = depthImage.imageView,
        .depthImageClearValue = 1.f,
    });

    vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);

    if (SceneManager::get().sceneIsType(SceneType::Scene3D)) {
        renderer->getIBL().drawSky(device, cmd, drawImage.getExtent2D(), sceneDataBuffer.getBuffer());
    }

    forwardRenderer.draw3(device, 
        cmd, 
        drawImage.getExtent2D(), 
        *this, 
        sceneDataBuffer.getBuffer(),
        renderer->getBuiltInModels(),
        renderer->getMeshCache(),
        renderer->getMaterialCache(),
        scene); 

    device.endOffscreenFrame(cmd);

    return drawImageId;
}

std::string clearFlagsToString(ClearFlags flags)
{
    if (flags == ClearFlags::Skybox) return "Skybox";
    else if (flags == ClearFlags::SolidColor) return "SolidColor";
    else if (flags == ClearFlags::DepthOnly) return "DepthOnly";
    else if (flags == ClearFlags::DontClear) return "DontClear";
    else return "Unknown";
}

ClearFlags clearFlagsFromString(const std::string & str)
{
    if (str == "Skybox") return ClearFlags::Skybox;
    else if (str == "SolidColor") return ClearFlags::SolidColor;
    else if (str == "DepthOnly") return ClearFlags::DepthOnly;
    else if (str == "DontClear") return ClearFlags::DontClear;
    else
    {
        SKY_ERROR("Unknown clear flags string: {}", str);
        return ClearFlags::Skybox; // Default fallback  
    }
}

std::string projectionTypeToString(ProjectionType type)
{
    if (type == ProjectionType::Perspective) return "Perspective";
    else if (type == ProjectionType::Orthographic) return "Orthographic";
    else return "Unknown";
}

ProjectionType projectionTypeFromString(const std::string & str)
{
    if (str == "Perspective") return ProjectionType::Perspective;
    else if (str == "Orthographic") return ProjectionType::Orthographic;
    else
    {
        SKY_ERROR("Unknown projection type string: {}", str);
        return ProjectionType::Perspective; // Default fallback
    }
}
}