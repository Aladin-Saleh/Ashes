/*
This file belongs to Ashes.
See LICENSE file in root folder.
*/
#include "ashespp/Pipeline/ComputePipeline.hpp"

#include "ashespp/Core/Device.hpp"

namespace ashes
{
	ComputePipeline::ComputePipeline( Device const & device
		, ComputePipelineCreateInfo createInfo )
		: ComputePipeline{ device, "ComputePipeline", std::move( createInfo ) }
	{
	}

	ComputePipeline::ComputePipeline( Device const & device
		, std::string const & debugName
		, ComputePipelineCreateInfo createInfo )
		: m_device{ device }
		, m_createInfo{ std::move( createInfo ) }
	{
		DEBUG_DUMP( m_createInfo );
		DEBUG_WRITE( "pipeline.log" );
		auto res = m_device.vkCreateComputePipelines( m_device
			, VK_NULL_HANDLE
			, 1
			, &static_cast< VkComputePipelineCreateInfo const & >( m_createInfo )
			, m_device.getAllocationCallbacks()
			, &m_internal );
		checkError( res, "ComputePipeline creation" );
		registerObject( m_device, debugName, *this );
	}

	ComputePipeline::~ComputePipeline()
	{
		unregisterObject( m_device, *this );
		m_device.vkDestroyPipeline( m_device
			, m_internal
			, m_device.getAllocationCallbacks() );
	}
}
