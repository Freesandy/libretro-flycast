/*
    Created on: Nov 7, 2019

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
#include "../vulkan.h"
#include "../vulkan_renderer.h"
#include "oit_drawer.h"
#include "oit_shaders.h"
#include "oit_buffer.h"

class OITVulkanRenderer final : public BaseVulkanRenderer
{
public:
	bool Init() override
	{
		DEBUG_LOG(RENDERER, "OITVulkanRenderer::Init");
		try {
			BaseVulkanRenderer::Init();

			oitBuffers.Init(0, 0);
			textureDrawer.Init(&samplerManager, &oitShaderManager, &textureCache, &oitBuffers);
			textureDrawer.SetCommandPool(&texCommandPool);

			screenDrawer.Init(&samplerManager, &oitShaderManager, &oitBuffers);
			screenDrawer.SetCommandPool(&texCommandPool);

			return true;
		}
		catch (const vk::SystemError& err)
		{
			ERROR_LOG(RENDERER, "Vulkan error: %s", err.what());
		}
		return false;
	}

	void Resize(int w, int h) override
	{
		NOTICE_LOG(RENDERER, "OIT Resize %d x %d", w, h);
		BaseVulkanRenderer::Resize(w, h);
		screenDrawer.Init(&samplerManager, &oitShaderManager, &oitBuffers);
	}

	void Term() override
	{
		DEBUG_LOG(RENDERER, "VulkanRenderer::Term");
		GetContext()->WaitIdle();
		screenDrawer.Term();
		textureDrawer.Term();
		oitBuffers.Term();
		BaseVulkanRenderer::Term();
	}

	bool Render() override
	{
		OITDrawer *drawer;
		if (pvrrc.isRTT)
			drawer = &textureDrawer;
		else
			drawer = &screenDrawer;

		drawer->Draw(fogTexture.get(), paletteTexture.get());

		drawer->EndFrame();

		return !pvrrc.isRTT;
	}

	bool Present() override
	{
		return screenDrawer.PresentFrame();
	}

private:
	OITBuffers oitBuffers;
	SamplerManager samplerManager;
	OITShaderManager oitShaderManager;
	OITScreenDrawer screenDrawer;
	OITTextureDrawer textureDrawer;
};

Renderer* rend_OITVulkan()
{
	return new OITVulkanRenderer();
}
