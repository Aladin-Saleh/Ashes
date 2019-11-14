/**
*\file
*	RenderingResources.h
*\author
*	Sylvain Doremus
*/
#ifndef ___GlRenderer_Buffer_HPP___
#define ___GlRenderer_Buffer_HPP___
#pragma once

#include "renderer/Gl4Renderer/GlRendererPrerequisites.hpp"
#include "renderer/Gl4Renderer/Enum/GlBufferTarget.hpp"

namespace ashes::gl4
{
	class Buffer
	{
		friend class DeviceMemory;

	public:
		Buffer( VkDevice device
			, VkBufferCreateInfo createInfo );
		~Buffer();

		VkMemoryRequirements getMemoryRequirements()const;
		bool isMapped()const;

		inline GLuint getInternal()const
		{
			assert( m_internal != GL_INVALID_INDEX );
			return m_internal;
		}

		inline VkDeviceSize getInternalOffset()const
		{
			assert( m_internal != GL_INVALID_INDEX );
			return m_internalOffset;
		}

		inline GlBufferTarget getTarget()const
		{
			return m_target;
		}

		inline void setMemory( VkDeviceMemory memory )
		{
			m_memory = memory;
		}

		inline VkDeviceMemory getMemory()const
		{
			return m_memory;
		}

	private:
		inline void setInternal( GLuint value, VkDeviceSize internalOffset )
		{
			assert( m_internal == GL_INVALID_INDEX );
			m_internal = value;
			m_internalOffset = internalOffset;
		}

	private:
		VkDevice m_device;
		UInt32Array m_queueFamilyIndices;
		VkBufferCreateInfo m_createInfo;
		GLuint m_internal{ GL_INVALID_INDEX };
		VkDeviceSize m_internalOffset{ 0u };
		GlBufferTarget m_target;
		VkDeviceMemory m_memory{ VK_NULL_HANDLE };
		mutable GlBufferTarget m_copyTarget;
	};
}

#endif