/*
This file belongs to Renderer.
See LICENSE file in root folder
*/
#pragma once

#include <VkLib/VkLibPrerequisites.hpp>
#include <Renderer/CullModeFlag.hpp>

namespace vk_renderer
{
	/**
	*\brief
	*	Convertit un renderer::CullModeFlags en VkCullModeFlags.
	*\param[in] flags
	*	Le renderer::CullModeFlags.
	*\return
	*	Le VkCullModeFlags.
	*/
	VkCullModeFlags convert( renderer::CullModeFlags const & flags );
}
