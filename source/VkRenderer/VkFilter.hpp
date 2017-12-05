/*
This file belongs to Renderer.
See LICENSE file in root folder.
*/
#pragma once

#include <VkLib/VkLibPrerequisites.hpp>
#include <Renderer/Filter.hpp>

namespace vk_renderer
{
	/**
	*\brief
	*	Convertit un renderer::Filter en VkFilter.
	*\param[in] filter
	*	Le renderer::Filter.
	*\return
	*	Le VkFilter.
	*/
	VkFilter convert( renderer::Filter const & filter );
}
