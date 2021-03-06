/*
This file belongs to Ashes.
See LICENSE file in root folder.
*/
#include "Command/Commands/GlWaitEventsCommand.hpp"

#include "Sync/GlEvent.hpp"

#include <algorithm>
#include <chrono>
#include <thread>

#include "ashesgl_api.hpp"

namespace ashes::gl
{
	void apply( ContextLock const & context
		, CmdWaitEvents const & cmd )
	{
		auto count = 0u;

		do
		{
			count = uint32_t( std::count_if( cmd.events.begin()
				, cmd.events.end()
				, []( VkEvent event )
				{
					return get( event )->getStatus() == VK_EVENT_SET
						|| get( event )->getStatus() == VK_EVENT_RESET;
				} ) );
			std::this_thread::sleep_for( std::chrono::nanoseconds{ 10 } );
		}
		while ( count != cmd.events.size() );
	}

	void buildWaitEventsCommand( VkEventArray events
		, VkPipelineStageFlags srcStageMask
		, VkPipelineStageFlags dstStageMask
		, ArrayView< VkMemoryBarrier const > memoryBarriers
		, ArrayView< VkBufferMemoryBarrier const > bufferMemoryBarriers
		, ArrayView< VkImageMemoryBarrier const > imageMemoryBarriers
		, CmdList & list )
	{
		glLogCommand( list, "WaitEventsCommand" );
		list.push_back( makeCmd< OpType::eWaitEvents >( std::move( events ) ) );
	}
}
