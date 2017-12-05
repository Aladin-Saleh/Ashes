/*
This file belongs to Renderer.
See LICENSE file in root folder
*/
#pragma once

#include <VkLib/VkLibPrerequisites.hpp>
#include <Renderer/ImageTiling.hpp>

namespace vk_renderer
{
	/**
	*\brief
	*	Convertit un renderer::ImageTiling en VkImageTiling.
	*\param[in] value
	*	Le renderer::ImageTiling.
	*\return
	*	Le VkImageTiling.
	*/
	VkImageTiling convert( renderer::ImageTiling const & value );
}
