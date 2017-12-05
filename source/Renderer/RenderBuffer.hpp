/*
This file belongs to Renderer.
See LICENSE file in root folder.
*/
#ifndef ___Renderer_RenderBuffer_HPP___
#define ___Renderer_RenderBuffer_HPP___
#pragma once

#include "RendererPrerequisites.hpp"

#include <Utils/PixelFormat.hpp>
#include <Utils/Vec2.hpp>

namespace renderer
{
	/**
	*\brief
	*	Un wrapper autour d'une texture ne pouvant être utilisée qu'en destination de rendu.
	*/
	class RenderBuffer
	{
	protected:
		/**
		*\brief
		*	Constructeur.
		*\param[in] device
		*	Le périphérique logique.
		*/
		RenderBuffer( Device const & device
			, utils::PixelFormat format
			, IVec2 const & size );

	public:
		/**
		*\~english
		*\brief
		*	Destructor.
		*\~french
		*\brief
		*	Destructeur.
		*/
		virtual ~RenderBuffer() = default;
		/**
		*\brief
		*	Prépare une barrière mémoire de transition vers un layout d'attache couleur.
		*\return
		*	La barrière mémoire.
		*/
		virtual ImageMemoryBarrier makeColourAttachment()const = 0;
		/**
		*\brief
		*	Prépare une barrière mémoire de transition vers un layout d'attache profondeur et stencil.
		*\return
		*	La barrière mémoire.
		*/
		virtual ImageMemoryBarrier makeDepthStencilAttachment()const = 0;
		/**
		*\return
		*	Le format des pixels de la texture.
		*/
		inline utils::PixelFormat getFormat()const noexcept
		{
			return m_format;
		}
		/**
		*\return
		*	Les dimensions de la texture.
		*/
		inline utils::IVec2 const & getDimensions()const noexcept
		{
			return m_size;
		}

	private:
		Device const & m_device;
		utils::IVec2 m_size;
		utils::PixelFormat m_format{ utils::PixelFormat::eR8G8B8 };
	};
}

#endif
