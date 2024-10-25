#include "scene_renderer.h"

#include "core/application.h"

namespace sky
{
SceneRenderer::SceneRenderer()
{
	m_device = CreateRef<gfx::Device>(*Application::getWindow());
	init();
}

SceneRenderer::~SceneRenderer() {}

void SceneRenderer::init() {}

void SceneRenderer::render() {}
} // namespace sky