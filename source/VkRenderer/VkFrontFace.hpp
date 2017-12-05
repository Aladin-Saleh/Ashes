/*
This file belongs to Renderer.
See LICENSE file in root folder
*/
#pragma once

#include <VkLib/VkLibPrerequisites.hpp>
#include <Renderer/FrontFace.hpp>

namespace vk_renderer
{
	/**
	*\brief
	*	Convertit un renderer::FrontFace en VkFrontFace.
	*\param[in] value
	*	Le renderer::FrontFace.
	*\return
	*	Le VkFrontFace.
	*/
	VkFrontFace convert( renderer::FrontFace const & value );
}
