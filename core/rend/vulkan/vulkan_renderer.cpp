/*
    Created on: Oct 2, 2019

	Copyright 2019 flyinghead

	This file is part of Flycast.

    Flycast is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Flycast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Flycast.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "vulkan.h"
#include "vulkan_renderer.h"
#include "drawer.h"
#include "shaders.h"

class VulkanRenderer final : public BaseVulkanRenderer
{
public:
	bool Init() override
	{
		DEBUG_LOG(RENDERER, "VulkanRenderer::Init");
		BaseVulkanRenderer::Init();

		textureDrawer.Init(&samplerManager, &shaderManager, &textureCache);
		textureDrawer.SetCommandPool(&texCommandPool);

		screenDrawer.Init(&samplerManager, &shaderManager);
		screenDrawer.SetCommandPool(&texCommandPool);

		return true;
	}

	void Resize(int w, int h) override
	{
		BaseVulkanRenderer::Resize(w, h);
		screenDrawer.Init(&samplerManager, &shaderManager);
	}

	void Term() override
	{
		DEBUG_LOG(RENDERER, "VulkanRenderer::Term");
		GetContext()->WaitIdle();
		BaseVulkanRenderer::Term();
	}

	bool Render() override
	{
		Drawer *drawer;
		if (pvrrc.isRTT)
			drawer = &textureDrawer;
		else
			drawer = &screenDrawer;

		drawer->Draw(fogTexture.get(), paletteTexture.get());

		drawer->EndRenderPass();

		return !pvrrc.isRTT;
	}

	bool Present() override
	{
		return screenDrawer.PresentFrame();
	}

private:
	SamplerManager samplerManager;
	ScreenDrawer screenDrawer;
	TextureDrawer textureDrawer;
};

Renderer* rend_Vulkan()
{
	return new VulkanRenderer();
}
