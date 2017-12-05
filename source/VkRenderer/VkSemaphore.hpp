/*
This file belongs to Renderer.
See LICENSE file in root folder.
*/
#pragma once

#include "VkRendererPrerequisites.hpp"

#include <VkLib/Semaphore.hpp>
#include <Renderer/Semaphore.hpp>

namespace vk_renderer
{
	/**
	*\brief
	*	Classe de Semaphore.
	*\remarks
	*	Un sémaphore est un élément de synchronisation servant pour les files.
	*/
	class Semaphore
		: public renderer::Semaphore
	{
	public:
		/**
		*\brief
		*	Constructeur.
		*\param[in] device
		*	Le LogicalDevice parent.
		*/
		explicit Semaphore( renderer::Device const & device );
		/**
		*\brief
		*	Conversion implicite vers VkSemaphore.
		*/
		inline vk::Semaphore const & getSemaphore()const
		{
			return m_semaphore;
		}

	private:
		vk::Semaphore m_semaphore;
	};
}
