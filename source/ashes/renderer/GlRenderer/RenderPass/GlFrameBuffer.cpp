/*
This file belongs to Ashes.
See LICENSE file in root folder.
*/
#include "RenderPass/GlFrameBuffer.hpp"

#include "Command/GlQueue.hpp"
#include "Core/GlDevice.hpp"
#include "RenderPass/GlRenderPass.hpp"
#include "Image/GlImage.hpp"
#include "Image/GlImageView.hpp"

#include "ashesgl_api.hpp"

#include <iostream>

namespace ashes::gl
{
	enum GlFramebufferStatus
		: GLenum
	{
		GL_FRAMEBUFFER_STATUS_UNDEFINED = 0x8219,
		GL_FRAMEBUFFER_STATUS_COMPLETE = 0x8CD5,
		GL_FRAMEBUFFER_STATUS_INCOMPLETE_ATTACHMENT = 0x8CD6,
		GL_FRAMEBUFFER_STATUS_INCOMPLETE_MISSING_ATTACHMENT = 0x8CD7,
		GL_FRAMEBUFFER_STATUS_INCOMPLETE_DRAW_BUFFER = 0x8CDB,
		GL_FRAMEBUFFER_STATUS_INCOMPLETE_READ_BUFFER = 0x8CDC,
		GL_FRAMEBUFFER_STATUS_UNSUPPORTED = 0x8CDD,
		GL_FRAMEBUFFER_STATUS_INCOMPLETE_MULTISAMPLE = 0x8D56,
		GL_FRAMEBUFFER_STATUS_INCOMPLETE_LAYER_TARGETS = 0x8DA8,
	};

	namespace gl3
	{
		void bindAttach( FboAttachment const & attachment
			, uint32_t index
			, CmdList & list )
		{
			if ( attachment.baseArrayLayer
				|| ( attachment.imgLayerCount > 1 && attachment.viewLayerCount <= 1 ) )
			{
				list.push_back( makeCmd< OpType::eFramebufferTextureLayer >( GL_FRAMEBUFFER
					, GlAttachmentPoint( attachment.point + index )
					, attachment.object
					, attachment.mipLevel
					, attachment.baseArrayLayer ) );
			}
			else if ( attachment.viewLayerCount
				&& ( attachment.target < GL_TEXTURE_CUBE_POSITIVE_X || attachment.target > GL_TEXTURE_CUBE_NEGATIVE_Z ) )
			{
				list.push_back( makeCmd< OpType::eFramebufferTexture >( GL_FRAMEBUFFER
					, GlAttachmentPoint( attachment.point + index )
					, attachment.object
					, attachment.mipLevel ) );
			}
			else
			{
				list.push_back( makeCmd< OpType::eFramebufferTexture2D >( GL_FRAMEBUFFER
					, GlAttachmentPoint( attachment.point + index )
					, attachment.target
					, attachment.object
					, attachment.mipLevel ) );
			}
		}

		FboAttachment initialiseAttachment( VkImageView view
			, uint32_t index
			, bool & multisampled )
		{
			auto image = get( view )->getImage();
			multisampled = get( image )->getSamples() > VK_SAMPLE_COUNT_1_BIT;

			auto layersCount = get( image )->getArrayLayers();
			bool isCube = checkFlag( get( image )->getCreateFlags()
				, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT );

			FboAttachment attachment{};
			attachment.point = getAttachmentPoint( view );
			attachment.type = getAttachmentType( view );
			attachment.originalObject = get( image )->getInternal();
			attachment.originalMipLevel = get( view )->getSubresourceRange().baseMipLevel;
			attachment.object = get( view )->getInternal();
			attachment.mipLevel = attachment.originalMipLevel;
			attachment.imgLayerCount = std::max( get( image )->getArrayLayers(), get( image )->getDimensions().depth );
			attachment.viewLayerCount = ( ( get( view )->getType() == VK_IMAGE_VIEW_TYPE_3D )
				? attachment.imgLayerCount
				: get( view )->getSubresourceRange().layerCount );
			attachment.baseArrayLayer = ( ( ( !isCube && attachment.imgLayerCount > 1u ) || ( isCube && attachment.imgLayerCount > 6u ) )
				? get( view )->getSubresourceRange().baseArrayLayer
				: 0u );
			attachment.target = ( get( image )->getType() == VK_IMAGE_TYPE_3D
				? GL_TEXTURE_3D
				: ( multisampled
					? ( attachment.viewLayerCount > 1u
						? GL_TEXTURE_2D_MULTISAMPLE_ARRAY
						: GL_TEXTURE_2D_MULTISAMPLE )
					: ( isCube
						? get( view )->getTextureType()
						: ( attachment.viewLayerCount > 1u
							? GL_TEXTURE_2D_ARRAY
							: GL_TEXTURE_2D ) ) ) );
			attachment.index = index;
			return attachment;
		}
	}

	namespace gl4
	{
		void bindAttach( FboAttachment const & attachment
			, uint32_t index
			, CmdList & list )
		{
			if ( attachment.viewLayerCount > 1u )
			{
				list.push_back( makeCmd< OpType::eFramebufferTexture >( GL_FRAMEBUFFER
					, GlAttachmentPoint( attachment.point + index )
					, attachment.object
					, attachment.mipLevel ) );
			}
			else
			{
				list.push_back( makeCmd< OpType::eFramebufferTexture2D >( GL_FRAMEBUFFER
					, GlAttachmentPoint( attachment.point + index )
					, attachment.target
					, attachment.object
					, attachment.mipLevel ) );
			}
		}

		FboAttachment initialiseAttachment( VkImageView view
			, uint32_t index
			, bool & multisampled )
		{
			auto image = get( view )->getImage();
			multisampled = get( image )->getSamples() > VK_SAMPLE_COUNT_1_BIT;

			FboAttachment attachment{};
			attachment.point = getAttachmentPoint( view );
			attachment.type = getAttachmentType( view );
			attachment.originalObject = get( image )->getInternal();
			attachment.originalMipLevel = get( view )->getSubresourceRange().baseMipLevel;
			attachment.object = get( view )->getInternal();
			attachment.mipLevel = attachment.originalMipLevel;
			attachment.imgLayerCount = std::max( get( image )->getArrayLayers(), get( image )->getDimensions().depth );
			attachment.viewLayerCount = ( ( get( view )->getType() == VK_IMAGE_VIEW_TYPE_3D )
				? attachment.imgLayerCount
				: get( view )->getSubresourceRange().layerCount );
			attachment.target = ( get( image )->getType() == VK_IMAGE_TYPE_3D
				? GL_TEXTURE_3D
				: ( attachment.viewLayerCount > 1u
					? ( multisampled
						? GL_TEXTURE_2D_MULTISAMPLE_ARRAY
						: GL_TEXTURE_2D_ARRAY )
					: ( multisampled
						? GL_TEXTURE_2D_MULTISAMPLE
						: GL_TEXTURE_2D ) ) );
			attachment.index = index;

			if ( get( view )->getSubresourceRange().baseMipLevel )
			{
				if ( get( image )->getArrayLayers() == 1u )
				{
					attachment.object = get( image )->getInternal();
				}
				else
				{
					attachment.mipLevel = 0u;
				}
			}

			return attachment;
		}
	}

	void bindAttach( VkDevice device
		, FboAttachment const & attachment
		, CmdList & list )
	{
		if ( hasTextureViews( device ) )
		{
			gl4::bindAttach( attachment
				, attachment.index
				, list );
		}
		else
		{
			gl3::bindAttach( attachment
				, attachment.index
				, list );
		}
	}

	FboAttachment initialiseAttachment( VkDevice device
		, VkImageView view
		, uint32_t index
		, bool & multisampled )
	{
		if ( hasTextureViews( device ) )
		{
			return gl4::initialiseAttachment( view
				, index
				, multisampled );
		}

		return gl3::initialiseAttachment( view
			, index
			, multisampled );
	}

	bool isSRGBFormat( VkFormat format )
	{
		return format == VK_FORMAT_R8G8_SRGB
			|| format == VK_FORMAT_R8G8B8_SRGB
			|| format == VK_FORMAT_B8G8R8_SRGB
			|| format == VK_FORMAT_R8G8B8A8_SRGB
			|| format == VK_FORMAT_B8G8R8A8_SRGB
			|| format == VK_FORMAT_A8B8G8R8_SRGB_PACK32;
	}

	GlAttachmentPoint getAttachmentPoint( VkImageAspectFlags aspectMask )
	{
		if ( checkFlag( aspectMask, VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT )
			&& checkFlag( aspectMask, VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT ) )
		{
			return GL_ATTACHMENT_POINT_DEPTH_STENCIL;
		}

		if ( checkFlag( aspectMask, VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT ) )
		{
			return GL_ATTACHMENT_POINT_STENCIL;
		}

		if ( checkFlag( aspectMask, VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT ) )
		{
			return GL_ATTACHMENT_POINT_DEPTH;
		}

		return GL_ATTACHMENT_POINT_COLOR0;
	}

	GlAttachmentPoint getAttachmentPoint( VkImageView texture )
	{
		return getAttachmentPoint( get( texture )->getSubresourceRange().aspectMask );
	}

	GlAttachmentType getAttachmentType( VkImageAspectFlags aspectMask )
	{
		if ( checkFlag( aspectMask, VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT )
			&& checkFlag( aspectMask, VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT ) )
		{
			return GL_ATTACHMENT_TYPE_DEPTH_STENCIL;
		}

		if ( checkFlag( aspectMask, VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT ) )
		{
			return GL_ATTACHMENT_TYPE_STENCIL;
		}

		if ( checkFlag( aspectMask, VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT ) )
		{
			return GL_ATTACHMENT_TYPE_DEPTH;
		}

		return GL_ATTACHMENT_TYPE_COLOR;
	}

	GlAttachmentType getAttachmentType( VkImageView texture )
	{
		return getAttachmentType( get( texture )->getFormat() );
	}

	void checkCompleteness( GLenum status )
	{
		switch ( status )
		{
		case 0:
			std::cerr << "An error has occured." << std::endl;
			assert( false );
			break;

		case GL_FRAMEBUFFER_STATUS_COMPLETE:
			break;

		case GL_FRAMEBUFFER_STATUS_UNDEFINED:
			std::cerr << "The specified framebuffer is the default read or draw framebuffer, but the default framebuffer does not exist." << std::endl;
			assert( false );
			break;

		case GL_FRAMEBUFFER_STATUS_INCOMPLETE_ATTACHMENT:
			std::cerr << "At least one of the framebuffer attachment points are framebuffer incomplete." << std::endl;
			assert( false );
			break;

		case GL_FRAMEBUFFER_STATUS_INCOMPLETE_MISSING_ATTACHMENT:
			std::cerr << "The framebuffer does not have at least one image attached to it." << std::endl;
			assert( false );
			break;

		case GL_FRAMEBUFFER_STATUS_INCOMPLETE_DRAW_BUFFER:
			std::cerr << "The value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for any color attachment point(s) named by GL_DRAW_BUFFERi." << std::endl;
			assert( false );
			break;

		case GL_FRAMEBUFFER_STATUS_INCOMPLETE_READ_BUFFER:
			std::cerr << "GL_READ_BUFFER is not GL_NONE and the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for the color attachment point named by GL_READ_BUFFER." << std::endl;
			assert( false );
			break;

		case GL_FRAMEBUFFER_STATUS_UNSUPPORTED:
			std::cerr << "The combination of internal formats of the attached images violates an implementation-dependent set of restrictions." << std::endl;
			assert( false );
			break;

		case GL_FRAMEBUFFER_STATUS_INCOMPLETE_MULTISAMPLE:
			std::cerr << "One of the following:" << std::endl;
			std::cerr << "  - The value of GL_RENDERBUFFER_SAMPLES is not the same for all attached renderbuffers;" << std::endl;
			std::cerr << "  - The value of GL_TEXTURE_SAMPLES is the not same for all attached textures;" << std::endl;
			std::cerr << "  - The attached images are a mix of renderbuffers and textures, the value of GL_RENDERBUFFER_SAMPLES does not match the value of GL_TEXTURE_SAMPLES;" << std::endl;
			std::cerr << "  - The value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not the same for all attached textures;" << std::endl;
			std::cerr << "  - The attached images are a mix of renderbuffers and textures, the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not GL_TRUE for all attached textures." << std::endl;
			assert( false );
			break;

		case GL_FRAMEBUFFER_STATUS_INCOMPLETE_LAYER_TARGETS:
			std::cerr << "At least one framebuffer attachment is layered, and any populated attachment is not layered, or all populated color attachments are not from textures of the same target." << std::endl;
			assert( false );
			break;
		}
	}

	Framebuffer::Framebuffer( VkDevice device
		, VkFramebufferCreateInfo createInfo )
		: m_device{ device }
		, m_flags{ createInfo.flags }
		, m_renderPass{ createInfo.renderPass }
		, m_attachments{ makeVector( createInfo.pAttachments, createInfo.attachmentCount ) }
		, m_dimensions{ createInfo.width, createInfo.height }
		, m_layers{ createInfo.layers }
	{
		doInitialiseAttaches();
		doBindAttaches();
		doCreateFramebuffer();
	}

	Framebuffer::Framebuffer( VkDevice device
		, GLuint name )
		: m_device{ device }
	{
		m_internal = name;
	}

	Framebuffer::~Framebuffer()
	{
		if ( m_internal != GL_INVALID_INDEX )
		{
			auto context = get( m_device )->getContext();
			glLogCall( context
				, glDeleteFramebuffers
				, 1
				, &m_internal );
		}
	}

	UInt32Array Framebuffer::getDrawBuffers( ArrayView < VkAttachmentReference const > const & references )const
	{
		assert( getInternal() != GL_INVALID_INDEX );
		UInt32Array drawBuffers;
		auto & attaches = getAllAttaches();

		for ( auto & reference : references )
		{
			auto & attach = attaches[reference.attachment];
			drawBuffers.push_back( attach.point + attach.index );
		}

		if ( m_drawBuffers != drawBuffers )
		{
			m_drawBuffers = drawBuffers;
		}

		return m_drawBuffers;
	}

	UInt32Array Framebuffer::getDrawBuffers( ArrayView < VkAttachmentReference > const & references )const
	{
		m_drawBuffers.clear();

		assert( getInternal() != GL_INVALID_INDEX );
		auto & attachments = getAttachments();
		auto & attaches = getAllAttaches();

		for ( auto & reference : references )
		{
			auto fboAttach = attachments[reference.attachment];
			auto fboView = get( fboAttach );

			if ( !isDepthOrStencilFormat( fboView->getFormat() ) )
			{
				auto & attach = attaches[reference.attachment];
				auto fboImage = get( fboView->getImage() );

				if ( fboImage->hasInternal() )
				{
					m_drawBuffers.push_back( attach.point + attach.index );
				}
				else if ( attaches.size() == 1 )
				{
					m_drawBuffers.push_back( GL_ATTACHMENT_POINT_BACK );
				}
			}
		}

		return m_drawBuffers;
	}

	bool Framebuffer::hasOnlySwapchainImage()const
	{
		return m_attachments.end() == std::find_if( m_attachments.begin()
			, m_attachments.end()
			, []( VkImageView view )
			{
				return !get( get( view )->getImage() )->isSwapchainImage();
			} );
	}

	bool Framebuffer::hasSwapchainImage()const
	{
		return m_attachments.end() != std::find_if( m_attachments.begin()
			, m_attachments.end()
			, []( VkImageView view )
			{
				return get( get( view )->getImage() )->isSwapchainImage();
			} );
	}

	void Framebuffer::doInitialiseAttaches()
	{
		auto renderPass = get( m_renderPass );
		auto itPassAttach = renderPass->begin();
		uint32_t index = 0u;
		uint32_t msIndex = 0u;
		auto fboAttachIt = m_attachments.begin();

		for ( auto & passAttach : *renderPass )
		{
			assert( fboAttachIt != m_attachments.end() );
			auto attach = renderPass->getAttachment( passAttach );
			doInitialiseAttach( *fboAttachIt
				, ( ashes::isDepthOrStencilFormat( get( *fboAttachIt )->getFormat() )
					? 0u
					: ( get( get( *fboAttachIt )->getImage() )->getSamples() > VK_SAMPLE_COUNT_1_BIT
						? msIndex++
						: index++ ) ) );
			++fboAttachIt;
		}
	}

	void Framebuffer::doBindAttaches()
	{
		if ( !m_multisampled )
		{
			if ( m_depthStencilAttach )
			{
				bindAttach( m_device, *m_depthStencilAttach, m_bindAttaches );
			}

			for ( auto & attachment : m_colourAttaches )
			{
				bindAttach( m_device, attachment, m_bindAttaches );
			}
		}
		else
		{
			if ( m_depthStencilMsAttach )
			{
				bindAttach( m_device, *m_depthStencilMsAttach, m_bindAttaches );
			}

			for ( auto & attachment : m_colourMsAttaches )
			{
				bindAttach( m_device, attachment, m_bindAttaches );
			}
		}
	}

	void Framebuffer::doCreateFramebuffer()
	{
		auto context = get( m_device )->getContext();
		glLogCall( context
			, glGenFramebuffers
			, 1
			, &m_internal );
		glLogCall( context
			, glBindFramebuffer
			, GL_FRAMEBUFFER
			, m_internal );
		applyList( context, m_bindAttaches );
		checkCompleteness( context->glCheckFramebufferStatus( GL_FRAMEBUFFER ) );
		glLogCall( context
			, glBindFramebuffer
			, GL_FRAMEBUFFER
			, 0 );
	}

	void Framebuffer::doInitialiseAttach( VkImageView view
		, uint32_t index )
	{
		bool multisampled{ false };
		auto attachment = initialiseAttachment( m_device, view, index, multisampled );
		m_multisampled = m_multisampled || multisampled;

		if ( attachment.point == GL_ATTACHMENT_POINT_DEPTH_STENCIL
			|| attachment.point == GL_ATTACHMENT_POINT_DEPTH
			|| attachment.point == GL_ATTACHMENT_POINT_STENCIL )
		{
			assert( attachment.index == 0u );

			if ( multisampled )
			{
				m_depthStencilMsAttach = attachment;
			}
			else
			{
				m_depthStencilAttach = attachment;
			}
		}
		else
		{
			if ( multisampled )
			{
				m_colourMsAttaches.push_back( attachment );
			}
			else
			{
				m_colourAttaches.push_back( attachment );
			}

			m_allColourAttaches.push_back( attachment );
			m_srgb |= isSRGBFormat( get( view )->getFormat() );
		}

		m_allAttaches.push_back( attachment );
	}
}
