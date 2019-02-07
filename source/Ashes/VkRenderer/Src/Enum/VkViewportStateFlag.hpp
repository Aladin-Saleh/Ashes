/*
This file belongs to Ashes.
See LICENSE file in root folder
*/
#pragma once

#include <AshesPrerequisites.hpp>

namespace vk_renderer
{
	/**
	*\brief
	*	Convertit un ashes::DepthStencilStateFlags en VkPipelineDepthStencilStateCreateFlags.
	*\param[in] flags
	*	Le ashes::DepthStencilStateFlags.
	*\return
	*	Le VkPipelineDepthStencilStateCreateFlags.
	*/
	VkPipelineViewportStateCreateFlags convert( ashes::ViewportStateFlags const & flags );
}