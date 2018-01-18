﻿/**
*\file
*	VertexBuffer.h
*\author
*	Sylvain Doremus
*/
#ifndef ___VkRenderer_DescriptorSet_HPP___
#define ___VkRenderer_DescriptorSet_HPP___
#pragma once

#include "GlRendererPrerequisites.hpp"

#include <Renderer/Descriptor/DescriptorSet.hpp>

#include <vector>

namespace gl_renderer
{
	/**
	*\brief
	*	Set de descripteurs.
	*/
	class DescriptorSet
		: public renderer::DescriptorSet
	{
	public:
		/**
		*\brief
		*	Constructeur.
		*\param[in] pool
		*	Le pool de sets de descripteurs.
		*/
		DescriptorSet( renderer::DescriptorSetPool const & pool );
		/**
		*\copydoc		renderer::DescriptorSet::createBinding
		*/
		renderer::CombinedTextureSamplerBinding const & createBinding( renderer::DescriptorSetLayoutBinding const & layoutBinding
			, renderer::TextureView const & view
			, renderer::Sampler const & sampler )override;
		/**
		*\copydoc		renderer::DescriptorSet::createBinding
		*/
		renderer::SamplerBinding const & createBinding( renderer::DescriptorSetLayoutBinding const & layoutBinding
			, renderer::Sampler const & sampler )override;
		/**
		*\copydoc		renderer::DescriptorSet::createBinding
		*/
		renderer::SampledTextureBinding const & createBinding( renderer::DescriptorSetLayoutBinding const & layoutBinding
			, renderer::TextureView const & view
			, renderer::ImageLayout layout )override;
		/**
		*\copydoc		renderer::DescriptorSet::createBinding
		*/
		renderer::StorageTextureBinding const & createBinding( renderer::DescriptorSetLayoutBinding const & layoutBinding
			, renderer::TextureView const & view )override;
		/**
		*\copydoc		renderer::DescriptorSet::createBinding
		*/
		renderer::UniformBufferBinding const & createBinding( renderer::DescriptorSetLayoutBinding const & layoutBinding
			, renderer::UniformBufferBase const & uniformBuffer
			, uint32_t offset )override;
		/**
		*\copydoc		renderer::DescriptorSet::createBinding
		*/
		renderer::StorageBufferBinding const & createBinding( renderer::DescriptorSetLayoutBinding const & layoutBinding
			, renderer::BufferBase const & storageBuffer
			, uint32_t offset )override;
		/**
		*\copydoc		renderer::DescriptorSet::createBinding
		*/
		renderer::UniformTexelBufferBinding const & createBinding( renderer::DescriptorSetLayoutBinding const & layoutBinding
			, renderer::UniformBufferBase const & uniformBuffer
			, renderer::BufferView const & view )override;
		/**
		*\copydoc		renderer::DescriptorSet::createBinding
		*/
		renderer::StorageTexelBufferBinding const & createBinding( renderer::DescriptorSetLayoutBinding const & layoutBinding
			, renderer::BufferBase const & storageBuffer
			, renderer::BufferView const & view )override;
		/**
		*\copydoc		renderer::DescriptorSet::createBinding
		*/
		void update()const override;
		/**
		*\brief
		*	Le tableau d'attaches de type sampler + texture.
		*/
		inline std::vector< renderer::CombinedTextureSamplerBinding > const & getCombinedTextureSamplers()const
		{
			return m_combinedTextureSamplers;
		}
		/**
		*\brief
		*	Le tableau d'attaches de type sampler.
		*/
		inline std::vector< renderer::SamplerBinding > const & getSamplers()const
		{
			return m_samplers;
		}
		/**
		*\brief
		*	Le tableau d'attaches de type texture échantillonnée.
		*/
		inline std::vector< renderer::SampledTextureBinding > const & getSampledTextures()const
		{
			return m_sampledTextures;
		}
		/**
		*\brief
		*	Le tableau d'attaches de type texture de stockage.
		*/
		inline std::vector< renderer::StorageTextureBinding > const & getStorageTextures()const
		{
			return m_storageTextures;
		}
		/**
		*\brief
		*	Le tableau d'attaches de type tampon uniforme.
		*/
		inline std::vector< renderer::UniformBufferBinding > const & getUniformBuffers()const
		{
			return m_uniformBuffers;
		}
		/**
		*\brief
		*	Le tableau d'attaches de type tampon de stockage.
		*/
		inline std::vector< renderer::StorageBufferBinding > const & getStorageBuffers()const
		{
			return m_storageBuffers;
		}
		/**
		*\brief
		*	Le tableau d'attaches de type tampon uniforme de texels.
		*/
		inline std::vector< renderer::UniformTexelBufferBinding > const & getUniformTexelBuffers()const
		{
			return m_uniformTexelBuffers;
		}
		/**
		*\brief
		*	Le tableau d'attaches de type tampon de stockage de texels.
		*/
		inline std::vector< renderer::StorageTexelBufferBinding > const & getStorageTexelBuffers()const
		{
			return m_storageTexelBuffers;
		}

	private:
		std::vector< renderer::CombinedTextureSamplerBinding > m_combinedTextureSamplers;
		std::vector< renderer::SamplerBinding > m_samplers;
		std::vector< renderer::SampledTextureBinding > m_sampledTextures;
		std::vector< renderer::StorageTextureBinding > m_storageTextures;
		std::vector< renderer::UniformBufferBinding > m_uniformBuffers;
		std::vector< renderer::StorageBufferBinding > m_storageBuffers;
		std::vector< renderer::UniformTexelBufferBinding > m_uniformTexelBuffers;
		std::vector< renderer::StorageTexelBufferBinding > m_storageTexelBuffers;
	};
}

#endif
