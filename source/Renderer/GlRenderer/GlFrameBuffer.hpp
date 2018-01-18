/*
This file belongs to Renderer.
See LICENSE file in root folder
*/
#pragma once

#include "GlRendererPrerequisites.hpp"

#include <Renderer/RenderPass/FrameBuffer.hpp>

namespace gl_renderer
{
	/**
	*\brief
	*	Classe encapsulant le concept de Framebuffer.
	*\remarks
	*	Contient les tampon de profondeur et de couleur.
	*/
	class FrameBuffer
		: public renderer::FrameBuffer
	{
	public:
		/**
		*\brief
		*	Crée un FrameBuffer compatible avec la passe de rendu donnée.
		*\remarks
		*	Si la compatibilité entre les textures voulues et les formats de la passe de rendu
		*	n'est pas possible, une std::runtime_error est lancée.
		*\param[in] dimensions
		*	Les dimensions du tampon d'images.
		*/
		FrameBuffer( renderer::RenderPass const & renderPass
			, renderer::IVec2 const & dimensions );
		/**
		*\brief
		*	Crée un FrameBuffer compatible avec la passe de rendu donnée.
		*\remarks
		*	Si la compatibilité entre les textures voulues et les formats de la passe de rendu
		*	n'est pas possible, une std::runtime_error est lancée.
		*\param[in] dimensions
		*	Les dimensions du tampon d'images.
		*\param[in] textures
		*	Les textures voulues pour le tampon d'images à créer.
		*/
		FrameBuffer( renderer::RenderPass const & renderPass
			, renderer::IVec2 const & dimensions
			, renderer::TextureCRefArray const & textures );
		/**
		*\brief
		*	Destructeur
		*/
		~FrameBuffer();
		/**
		*\brief
		*	Copie des données dans la RAM.
		*\remarks
		*	Pour utiliser cette fonction, il faut que le tampon soit activé.
		*\param[in] xoffset, yoffset
		*	Le décalage à partir duquel les données seront copiées, par rapport
		*	au d�but du stockage de la texture, en VRAM.
		*\param[in] width, height
		*	Les dimensions des données à copier.
		*\param[in] format
		*	Le format voulu pour les données.
		*\param[out] data
		*	Reçoit les données copiées.
		*/
		void download( renderer::Queue const & queue
			, uint32_t index
			, uint32_t xoffset
			, uint32_t yoffset
			, uint32_t width
			, uint32_t height
			, renderer::PixelFormat format
			, uint8_t * data )const noexcept;
		/**
		*\return
		*	Le Framebuffer OpenGL.
		*/
		inline GLuint getFrameBuffer()const
		{
			assert( m_frameBuffer != GL_INVALID_INDEX );
			return m_frameBuffer;
		}

	private:
		GLuint m_frameBuffer{ GL_INVALID_INDEX };
	};
}
