#include "Buffer/GlBufferView.hpp"

#include "Buffer/GlBuffer.hpp"
#include "Core/GlDevice.hpp"

#include "ashesgl_api.hpp"

namespace ashes::gl
{
	BufferView::BufferView( VkDevice device
		, VkBufferViewCreateInfo createInfo )
		: m_device{ device }
		, m_format{ createInfo.format }
		, m_offset{ createInfo.offset }
		, m_range{ createInfo.range }
	{
		auto context = get( m_device )->getContext();
		glLogCall( context
			, glGenTextures
			, 1
			, &m_internal );
		glLogCall( context
			, glBindTexture
			, GL_TEXTURE_BUFFER
			, m_internal );
		auto offset = get( createInfo.buffer )->getInternalOffset() + m_offset;

		if ( get( getInstance( device ) )->getFeatures().hasTexBufferRange )
		{
			glLogCall( context
				, glTexBufferRange
				, GL_TEXTURE_BUFFER
				, getInternalFormat( m_format )
				, get( createInfo.buffer )->getInternal()
				, offset
				, m_range );
		}
		else
		{
			if ( ( offset > 0 || m_range != get( get( createInfo.buffer )->getMemory() )->getSize() ) )
			{
				reportError( get( this )
					, VK_ERROR_VALIDATION_FAILED_EXT
					, "OpenGL"
					, "Texture buffer range is not supported" );
			}

			glLogCall( context
				, glTexBuffer
				, GL_TEXTURE_BUFFER
				, getInternalFormat( createInfo.format )
				, get( createInfo.buffer )->getInternal() );
		}

		glLogCall( context
			, glBindTexture
			, GL_TEXTURE_BUFFER
			, 0u );
	}

	BufferView::~BufferView()
	{
		auto context = get( m_device )->getContext();
		glLogCall( context
			, glDeleteTextures
			, 1
			, &m_internal );
	}
}
