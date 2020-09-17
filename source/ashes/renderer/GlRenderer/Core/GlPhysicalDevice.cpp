/*
This file belongs to Ashes.
See LICENSE file in root folder.
*/
#include "Core/GlPhysicalDevice.hpp"

#include "Core/GlInstance.hpp"
#include "Core/GlDevice.hpp"
#include "Miscellaneous/GlScreenHelpers.hpp"

#include "ashesgl_api.hpp"

#include <ashes/common/Hash.hpp>

#if defined( __linux__ )
#	include <X11/Xlib.h>
#elif defined( _WIN32 )
#	include <gl/GL.h>
#endif

#include <algorithm>
#include <cstring>
#include <iterator>
#include <stdexcept>

#ifdef max
#	undef max
#endif

namespace ashes::gl
{
	namespace
	{
		static GLenum constexpr GL_FULL_SUPPORT = 0x82B7;
		static GLenum constexpr GL_CAVEAT_SUPPORT = 0x82B8;
		static GLenum constexpr GL_IMAGE_CLASS_4_X_32 = 0x82B9;
		static GLenum constexpr GL_IMAGE_CLASS_2_X_32 = 0x82BA;
		static GLenum constexpr GL_IMAGE_CLASS_1_X_32 = 0x82BB;
		static GLenum constexpr GL_IMAGE_CLASS_4_X_16 = 0x82BC;
		static GLenum constexpr GL_IMAGE_CLASS_2_X_16 = 0x82BD;
		static GLenum constexpr GL_IMAGE_CLASS_1_X_16 = 0x82BE;
		static GLenum constexpr GL_IMAGE_CLASS_4_X_8 = 0x82BF;
		static GLenum constexpr GL_IMAGE_CLASS_2_X_8 = 0x82C0;
		static GLenum constexpr GL_IMAGE_CLASS_1_X_8 = 0x82C1;
		static GLenum constexpr GL_IMAGE_CLASS_11_11_10 = 0x82C2;
		static GLenum constexpr GL_IMAGE_CLASS_10_10_10_2 = 0x82C3;
		static GLenum constexpr GL_VIEW_CLASS_128_BITS = 0x82C4;
		static GLenum constexpr GL_VIEW_CLASS_96_BITS = 0x82C5;
		static GLenum constexpr GL_VIEW_CLASS_64_BITS = 0x82C6;
		static GLenum constexpr GL_VIEW_CLASS_48_BITS = 0x82C7;
		static GLenum constexpr GL_VIEW_CLASS_32_BITS = 0x82C8;
		static GLenum constexpr GL_VIEW_CLASS_24_BITS = 0x82C9;
		static GLenum constexpr GL_VIEW_CLASS_16_BITS = 0x82CA;
		static GLenum constexpr GL_VIEW_CLASS_8_BITS = 0x82CB;
		static GLenum constexpr GL_VIEW_CLASS_S3TC_DXT1_RGB = 0x82CC;
		static GLenum constexpr GL_VIEW_CLASS_S3TC_DXT1_RGBA = 0x82CD;
		static GLenum constexpr GL_VIEW_CLASS_S3TC_DXT3_RGBA = 0x82CE;
		static GLenum constexpr GL_VIEW_CLASS_S3TC_DXT5_RGBA = 0x82CF;
		static GLenum constexpr GL_VIEW_CLASS_RGTC1_RED = 0x82D0;
		static GLenum constexpr GL_VIEW_CLASS_RGTC2_RG = 0x82D1;
		static GLenum constexpr GL_VIEW_CLASS_BPTC_UNORM = 0x82D2;
		static GLenum constexpr GL_VIEW_CLASS_BPTC_FLOAT = 0x82D3;

		size_t makeKey( VkFormat format
			, VkImageType type
			, VkImageTiling tiling
			, VkImageUsageFlags usage
			, VkImageCreateFlags flags )
		{
			auto hash = std::hash< VkFormat >{}( format );
			hashCombine( hash, type );
			hashCombine( hash, tiling );
			hashCombine( hash, usage );
			hashCombine( hash, flags );
			return hash;
		}

		uint32_t doGetVendorID( std::string vendorName )
		{
			uint32_t result = 0u;
			std::transform( vendorName.begin()
				, vendorName.end()
				, vendorName.begin()
				, ::tolower );

			if ( vendorName.find( "nvidia" ) != std::string::npos )
			{
				result = 0x10DE;
			}
			else if ( vendorName.find( "intel" ) != std::string::npos )
			{
				result = 0x8086;
			}
			else if ( vendorName.find( "amd" ) != std::string::npos )
			{
				result = 0x1002;
			}
			else if ( vendorName.find( "arm" ) != std::string::npos )
			{
				result = 0x13B5;
			}

			return result;
		}
	}

	PhysicalDevice::PhysicalDevice( VkInstance instance )
		: m_instance{ instance }
	{
		doInitialise();
	}

	std::vector< VkLayerProperties > PhysicalDevice::enumerateLayerProperties()const
	{
		return {};
	}

	uint32_t PhysicalDevice::getMemoryTypeBits( VkMemoryPropertyFlags properties )const
	{
		uint32_t result{};
		auto & memoryProperties = getMemoryProperties();

		for ( auto i = 0u; i < memoryProperties.memoryTypeCount; ++i )
		{
			if ( ( memoryProperties.memoryTypes[i].propertyFlags & properties ) == properties )
			{
				result |= ( 0x1 << i );
			}
		}

		return result;
	}

	uint32_t PhysicalDevice::getMemoryTypeBits( VkMemoryPropertyFlags properties1
		, VkMemoryPropertyFlags properties2 )const
	{
		uint32_t result = getMemoryTypeBits( properties1 );
		result |= getMemoryTypeBits( properties2 );
		return result;
	}

	std::vector < VkExtensionProperties > PhysicalDevice::enumerateExtensionProperties( const char * layerName )const
	{
		static std::vector< VkExtensionProperties > const extensions
		{
#if VK_KHR_swapchain
			VkExtensionProperties{ VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_SWAPCHAIN_SPEC_VERSION },
#endif
#if VK_EXT_debug_report
			VkExtensionProperties{ VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_SPEC_VERSION },
#endif
#if VK_EXT_debug_marker
			VkExtensionProperties{ VK_EXT_DEBUG_MARKER_EXTENSION_NAME, VK_EXT_DEBUG_MARKER_SPEC_VERSION },
#endif
#if VK_EXT_debug_utils
			VkExtensionProperties{ VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_SPEC_VERSION },
#endif
#if VK_EXT_inline_uniform_block
			VkExtensionProperties{ VK_EXT_INLINE_UNIFORM_BLOCK_EXTENSION_NAME, VK_EXT_INLINE_UNIFORM_BLOCK_SPEC_VERSION },
#endif
#if VK_KHR_maintenance1
			VkExtensionProperties{ VK_KHR_MAINTENANCE1_EXTENSION_NAME, VK_KHR_MAINTENANCE1_SPEC_VERSION },
#endif
		};
		return extensions;
	}

	VkPhysicalDeviceProperties const & PhysicalDevice::getProperties()const
	{
		return m_properties;
	}

	VkPhysicalDeviceMemoryProperties const & PhysicalDevice::getMemoryProperties()const
	{
		return Instance::getMemoryProperties();
	}

	VkPhysicalDeviceFeatures const & PhysicalDevice::getFeatures()const
	{
		return m_features;
	}

	std::vector< VkQueueFamilyProperties > const & PhysicalDevice::getQueueFamilyProperties()const
	{
		return m_queueProperties;
	}

	VkFormatProperties const & PhysicalDevice::getFormatProperties( VkFormat fmt )const
	{
		return m_formatProperties[fmt];
	}

	VkResult PhysicalDevice::getImageFormatProperties( VkFormat format
		, VkImageType type
		, VkImageTiling tiling
		, VkImageUsageFlags usage
		, VkImageCreateFlags flags
		, VkImageFormatProperties & imageProperties )const
	{
		auto pair = m_imageFormatProperties.insert( { makeKey( format
				, type
				, VkImageTiling{}
				, VkImageUsageFlags{}
				, VkImageCreateFlags{} )
			, {} } );

		if ( pair.second )
		{
			ContextLock context{ get( m_instance )->getCurrentContext() };

			if ( context->m_glGetInternalformativ )
			{
				auto internal = getInternalFormat( format );
				auto gltype = convert( get( this ), type, 1u, 0u );
				GLint value;
				glLogCall( context, glGetInternalformativ, gltype, internal, GL_FORMAT_PROPERTY_SUPPORTED, 1, &value );

				if ( value == GL_FALSE )
				{
					return VK_ERROR_FORMAT_NOT_SUPPORTED;
				}

				glLogCall( context, glGetInternalformativ, gltype, internal, GL_FORMAT_PROPERTY_MAX_WIDTH, 1, &value );
				pair.first->second.maxExtent.width = uint32_t( value );
				pair.first->second.maxExtent.height = 1u;
				pair.first->second.maxExtent.depth = 1u;

				if ( type == VK_IMAGE_TYPE_2D
					|| type == VK_IMAGE_TYPE_3D )
				{
					glLogCall( context, glGetInternalformativ, GL_TEXTURE_2D, internal, GL_FORMAT_PROPERTY_MAX_HEIGHT, 1, &value );
					pair.first->second.maxExtent.height = uint32_t( value );

					if ( type == VK_IMAGE_TYPE_3D )
					{
						glLogCall( context, glGetInternalformativ, GL_TEXTURE_3D, internal, GL_FORMAT_PROPERTY_MAX_DEPTH, 1, &value );
						pair.first->second.maxExtent.depth = uint32_t( value );
					}
				}

				glLogCall( context, glGetInternalformativ, gltype, internal, GL_FORMAT_PROPERTY_MAX_SAMPLES, 1, &value );
				pair.first->second.sampleCounts = VkSampleCountFlagBits( value );

				glLogCall( context, glGetInternalformativ, gltype, internal, GL_FORMAT_PROPERTY_TEXEL_SIZE, 1, &value );
				VkDeviceSize texelSize = VkDeviceSize( value );

				gltype = convert( get( this ), type, 2u, 0u );
				glLogCall( context, glGetInternalformativ, gltype, internal, GL_FORMAT_PROPERTY_MAX_LAYERS, 1, &value );
				pair.first->second.maxArrayLayers = uint32_t( value );
			}
		}

		imageProperties = pair.first->second;
		return imageProperties.maxExtent.width == 0u
			? VK_ERROR_FORMAT_NOT_SUPPORTED
			: VK_SUCCESS;
	}

	VkResult PhysicalDevice::getSparseImageFormatProperties( VkFormat format
		, VkImageType type
		, VkSampleCountFlagBits samples
		, VkImageUsageFlags usage
		, VkImageTiling tiling
		, std::vector< VkSparseImageFormatProperties > & sparseImageFormatProperties )const
	{
		return VK_ERROR_FORMAT_NOT_SUPPORTED;
	}

#if VK_VERSION_1_1

	VkPhysicalDeviceFeatures2 const & PhysicalDevice::getFeatures2()const
	{
		return m_features2;
	}

	VkPhysicalDeviceProperties2 const & PhysicalDevice::getProperties2()const
	{
		return m_properties2;
	}

	VkFormatProperties2 const & PhysicalDevice::getFormatProperties2( VkFormat format )const
	{
		return m_formatProperties2[format];
	}

	VkResult PhysicalDevice::getImageFormatProperties2( VkPhysicalDeviceImageFormatInfo2 const & imageFormatInfo
		, VkImageFormatProperties2 & imageFormatProperties )const
	{
		imageFormatProperties.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;
		imageFormatProperties.pNext = nullptr;
		return getImageFormatProperties( imageFormatInfo.format
			, imageFormatInfo.type
			, imageFormatInfo.tiling
			, imageFormatInfo.usage
			, imageFormatInfo.flags
			, imageFormatProperties.imageFormatProperties );
	}

	std::vector< VkQueueFamilyProperties2 > PhysicalDevice::getQueueFamilyProperties2()const
	{
		return m_queueProperties2;
	}

	VkPhysicalDeviceMemoryProperties2 const & PhysicalDevice::getMemoryProperties2()const
	{
		return Instance::getMemoryProperties2();
	}

	VkResult PhysicalDevice::getSparseImageFormatProperties2( VkPhysicalDeviceSparseImageFormatInfo2 const & formatInfo
		, std::vector< VkSparseImageFormatProperties2 > & sparseImageFormatProperties )const
	{
		std::vector< VkSparseImageFormatProperties > props;
		auto result = getSparseImageFormatProperties( formatInfo.format
			, formatInfo.type
			, formatInfo.samples
			, formatInfo.usage
			, formatInfo.tiling
			, props );

		if ( result != VK_ERROR_FORMAT_NOT_SUPPORTED )
		{
			for ( auto & prop : props )
			{
				sparseImageFormatProperties.push_back(
					{
						VK_STRUCTURE_TYPE_SPARSE_IMAGE_FORMAT_PROPERTIES_2,
						nullptr,
						prop,
					} );
			}
		}

		return result;
	}

#elif VK_KHR_get_physical_device_properties2

	VkPhysicalDeviceFeatures2KHR const & PhysicalDevice::getFeatures2()const
	{
		return m_features2;
	}

	VkPhysicalDeviceProperties2KHR const & PhysicalDevice::getProperties2()const
	{
		return m_properties2;
	}

	VkFormatProperties2KHR const & PhysicalDevice::getFormatProperties2( VkFormat format )const
	{
		return m_formatProperties2[format];
	}

	VkResult PhysicalDevice::getImageFormatProperties2( VkPhysicalDeviceImageFormatInfo2KHR const & imageFormatInfo
		, VkImageFormatProperties2KHR & imageFormatProperties )const
	{
		imageFormatProperties.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2_KHR;
		imageFormatProperties.pNext = nullptr;
		return getImageFormatProperties( imageFormatInfo.format
			, imageFormatInfo.type
			, imageFormatInfo.tiling
			, imageFormatInfo.usage
			, imageFormatInfo.flags
			, imageFormatProperties.imageFormatProperties );
	}

	std::vector< VkQueueFamilyProperties2KHR > PhysicalDevice::getQueueFamilyProperties2()const
	{
		return m_queueProperties2;
	}

	VkPhysicalDeviceMemoryProperties2KHR const & PhysicalDevice::getMemoryProperties2()const
	{
		return Instance::getMemoryProperties2();
	}

	VkResult PhysicalDevice::getSparseImageFormatProperties2( VkPhysicalDeviceSparseImageFormatInfo2KHR const & formatInfo
		, std::vector< VkSparseImageFormatProperties2KHR > & sparseImageFormatProperties )const
	{
		std::vector< VkSparseImageFormatProperties > props;
		auto result = getSparseImageFormatProperties( formatInfo.format
			, formatInfo.type
			, formatInfo.samples
			, formatInfo.usage
			, formatInfo.tiling
			, props );

		if ( result != VK_ERROR_FORMAT_NOT_SUPPORTED )
		{
			for ( auto & prop : props )
			{
				sparseImageFormatProperties.push_back(
					{
						VK_STRUCTURE_TYPE_SPARSE_IMAGE_FORMAT_PROPERTIES_2_KHR,
						nullptr,
						prop,
					} );
			}
		}

		return result;
	}

#endif
#ifdef VK_KHR_display

	std::vector< VkDisplayPropertiesKHR > const & PhysicalDevice::getDisplayProperties()const
	{
		return m_displays;
	}

	std::vector< VkDisplayPlanePropertiesKHR > PhysicalDevice::getDisplayPlaneProperties()const
	{
		return m_displayPlanes;
	}

	std::vector< VkDisplayKHR > PhysicalDevice::getDisplayPlaneSupportedDisplays( uint32_t planeIndex )const
	{
		std::vector< VkDisplayKHR > result
		{
			m_displayPlanes[planeIndex].currentDisplay
		};
		return result;
	}

#endif

	bool PhysicalDevice::find( VkExtensionProperties const & name )const
	{
		return get( m_instance )->getExtensions().find( name );
	}

	bool PhysicalDevice::findAny( VkExtensionPropertiesArray const & names )const
	{
		return get( m_instance )->getExtensions().findAny( names );
	}

	bool PhysicalDevice::findAll( VkExtensionPropertiesArray const & names )const
	{
		return get( m_instance )->getExtensions().findAll( names );
	}

	int PhysicalDevice::getMajor()const
	{
		return get( m_instance )->getExtensions().getMajor();
	}

	int PhysicalDevice::getMinor()const
	{
		return get( m_instance )->getExtensions().getMinor();
	}

	void PhysicalDevice::doInitialise()
	{
		m_glFeatures.has420PackExtensions = find( ARB_shading_language_420pack );
		m_glFeatures.hasCopyImage = find( ARB_copy_image );
		m_glFeatures.hasProgramPipelines = find( ARB_separate_shader_objects );
		m_glFeatures.hasTextureStorage = findAll( { ARB_texture_storage, ARB_texture_storage_multisample } );
		m_glFeatures.hasTextureViews = find( ARB_texture_view );
		m_glFeatures.hasViewportArrays = find( ARB_viewport_array );
		m_glFeatures.hasProgramInterfaceQuery = find( ARB_program_interface_query );

		ContextLock context{ get( m_instance )->getCurrentContext() };
		doInitialiseProperties( context );
		doInitialiseFeatures( context );
		doInitialiseQueueProperties( context );
		doInitialiseFormatProperties( context );
		doInitialiseDisplayProperties( context );
		doInitialiseProperties2( context );
	}

	void PhysicalDevice::doInitialiseFeatures( ContextLock & context )
	{
		m_features.robustBufferAccess = find( KHR_robustness );
		m_features.fullDrawIndexUint32 = false;
		m_features.imageCubeArray = find( ARB_texture_cube_map_array );
		m_features.independentBlend = findAny( { ARB_draw_buffers_blend, EXT_draw_buffers2 } );
		m_features.geometryShader = find( ARB_geometry_shader4 );
		m_features.tessellationShader = find( ARB_tessellation_shader );
		m_features.sampleRateShading = find( ARB_sample_shading );
		m_features.dualSrcBlend = find( ARB_blend_func_extended );
		m_features.logicOp = true;
		m_features.multiDrawIndirect = findAll( { ARB_multi_draw_indirect, ARB_draw_indirect } );
		m_features.drawIndirectFirstInstance = findAll( { ARB_base_instance, ARB_draw_instanced } );
		m_features.depthClamp = find( ARB_depth_clamp );
		m_features.depthBiasClamp = find( ARB_polygon_offset_clamp );
		m_features.fillModeNonSolid = true;
		m_features.depthBounds = true;
		GLint range[2];
		glLogCall( context, glGetIntegerv
			, GL_VALUE_NAME_ALIASED_LINE_WIDTH_RANGE
			, range );
		m_features.wideLines = ( range[1] > 1 );
		glLogCall( context, glGetIntegerv
			, GL_VALUE_NAME_SMOOTH_LINE_WIDTH_RANGE
			, range );
		m_features.wideLines &= ( range[1] > 1 );
		m_features.largePoints = true;
		m_features.alphaToOne = true;
		m_features.multiViewport = find( ARB_viewport_array );
		m_features.samplerAnisotropy = find( ARB_texture_filter_anisotropic );
		m_features.textureCompressionETC2 = findAll( { ARB_ES3_compatibility, ARB_ES2_compatibility, ARB_invalidate_subdata, ARB_texture_storage } );
		m_features.textureCompressionASTC_LDR = find( KHR_texture_compression_astc_ldr );
		m_features.textureCompressionBC = findAll( { ARB_texture_compression, EXT_texture_compression_s3tc, EXT_texture_sRGB } );
		m_features.occlusionQueryPrecise = true;
		m_features.pipelineStatisticsQuery = find( ARB_pipeline_statistics_query );
		m_features.vertexPipelineStoresAndAtomics = find( ARB_shader_atomic_counters );
		m_features.fragmentStoresAndAtomics = m_features.vertexPipelineStoresAndAtomics;
		m_features.shaderTessellationAndGeometryPointSize = m_features.tessellationShader && m_features.geometryShader;
		m_features.shaderImageGatherExtended = findAll( { ARB_texture_gather, ARB_gpu_shader5 } );
		m_features.shaderStorageImageExtendedFormats = find( ARB_shader_image_load_store );
		m_features.shaderStorageImageMultisample = find( ARB_shader_image_load_store );
		m_features.shaderStorageImageReadWithoutFormat = find( EXT_shader_image_load_formatted );
		m_features.shaderStorageImageWriteWithoutFormat = find( ARB_shader_image_load_store );
		m_features.shaderUniformBufferArrayDynamicIndexing = find( ARB_gpu_shader5 );
		m_features.shaderSampledImageArrayDynamicIndexing = find( ARB_gpu_shader5 );
		m_features.shaderStorageBufferArrayDynamicIndexing = find( ARB_shader_storage_buffer_object );
		m_features.shaderStorageImageArrayDynamicIndexing = find( ARB_shader_image_load_store );
		m_features.shaderClipDistance = true;
		m_features.shaderCullDistance = find( ARB_cull_distance );
		m_features.shaderFloat64 = find( ARB_gpu_shader_fp64 );
		m_features.shaderInt64 = find( ARB_gpu_shader_int64 );
		m_features.shaderInt16 = false;
		m_features.shaderResourceResidency = false;// find( ARB_sparse_texture2 );
		m_features.shaderResourceMinLod = false;// find( ARB_sparse_texture_clamp );
		m_features.sparseBinding = false;// findAll( { ARB_sparse_buffer, ARB_sparse_texture2 } );
		m_features.sparseResidencyBuffer = false;// find( ARB_sparse_buffer );
		m_features.sparseResidencyImage2D = false;// find( ARB_sparse_texture2 );
		m_features.sparseResidencyImage3D = false;// find( ARB_sparse_texture2 );
		m_features.sparseResidency2Samples = false;// find( ARB_sparse_texture2 );
		m_features.sparseResidency4Samples = false;// find( ARB_sparse_texture2 );
		m_features.sparseResidency8Samples = false;// find( ARB_sparse_texture2 );
		m_features.sparseResidency16Samples = false;// find( ARB_sparse_texture2 );
		m_features.sparseResidencyAliased = false;
		m_features.variableMultisampleRate = true;
		m_features.inheritedQueries = true;
	}

	void PhysicalDevice::doInitialiseProperties( ContextLock & context )
	{
		auto & extensions = get( m_instance )->getExtensions();
		m_properties.apiVersion = ( extensions.getMajor() << 22 ) | ( extensions.getMinor() << 12 );
		m_properties.deviceID = 0u;
		strncpy( m_properties.deviceName, ( char const * )context->glGetString( GL_RENDERER ), VK_MAX_PHYSICAL_DEVICE_NAME_SIZE );
		std::memset( m_properties.pipelineCacheUUID, 0u, sizeof( m_properties.pipelineCacheUUID ) );
		m_properties.vendorID = doGetVendorID( ( char const * )context->glGetString( GL_VENDOR ) );
		m_properties.deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
		m_properties.driverVersion = 0;

		context.getValue( GL_VALUE_NAME_MAX_TEXTURE_SIZE, m_properties.limits.maxImageDimension1D );
		context.getValue( GL_VALUE_NAME_MAX_TEXTURE_SIZE, m_properties.limits.maxImageDimension2D );
		context.getValue( GL_VALUE_NAME_MAX_3D_TEXTURE_SIZE, m_properties.limits.maxImageDimension3D );
		context.getValue( GL_VALUE_NAME_MAX_CUBE_MAP_TEXTURE_SIZE, m_properties.limits.maxImageDimensionCube );
		context.getValue( GL_VALUE_NAME_MAX_ARRAY_TEXTURE_LAYERS, m_properties.limits.maxImageArrayLayers );
		context.getValue( GL_VALUE_NAME_MAX_TEXTURE_BUFFER_SIZE, m_properties.limits.maxTexelBufferElements );
		context.getValue( GL_VALUE_NAME_MAX_UNIFORM_BLOCK_SIZE, m_properties.limits.maxUniformBufferRange );
		m_properties.limits.maxStorageBufferRange = NonAvailable< uint32_t >;
		context.getValue( GL_VALUE_NAME_MAX_UNIFORM_BLOCK_SIZE, m_properties.limits.maxPushConstantsSize );
		m_properties.limits.maxMemoryAllocationCount = NonAvailable< uint32_t >;
		m_properties.limits.maxSamplerAllocationCount = NonAvailable< uint32_t >;
		m_properties.limits.bufferImageGranularity = NonAvailable< uint64_t >;
		m_properties.limits.sparseAddressSpaceSize = NonAvailable< uint64_t >;
		m_properties.limits.maxBoundDescriptorSets = NonAvailable< uint32_t >;
		context.getValue( GL_VALUE_NAME_MAX_COMBINED_TEXTURE_IMAGE_UNITS, m_properties.limits.maxPerStageDescriptorSamplers );
		context.getValue( GL_VALUE_NAME_MAX_COMBINED_UNIFORM_BLOCKS, m_properties.limits.maxPerStageDescriptorUniformBuffers );
		context.getValue( GL_VALUE_NAME_MAX_COMBINED_SHADER_STORAGE_BLOCKS, m_properties.limits.maxPerStageDescriptorStorageBuffers );
		context.getValue( GL_VALUE_NAME_MAX_COMBINED_TEXTURE_IMAGE_UNITS, m_properties.limits.maxPerStageDescriptorSampledImages );
		context.getValue( GL_VALUE_NAME_MAX_COMBINED_IMAGE_UNIFORMS, m_properties.limits.maxPerStageDescriptorStorageImages );
		m_properties.limits.maxPerStageDescriptorInputAttachments = NonAvailable< uint32_t >;
		m_properties.limits.maxPerStageResources = NonAvailable< uint32_t >;
		context.getValue( GL_VALUE_NAME_MAX_COMBINED_TEXTURE_IMAGE_UNITS, m_properties.limits.maxDescriptorSetSamplers );
		context.getValue( GL_VALUE_NAME_MAX_COMBINED_UNIFORM_BLOCKS, m_properties.limits.maxDescriptorSetUniformBuffers );
		context.getValue( GL_VALUE_NAME_MAX_COMBINED_UNIFORM_BLOCKS, m_properties.limits.maxDescriptorSetUniformBuffersDynamic );
		context.getValue( GL_VALUE_NAME_MAX_COMBINED_SHADER_STORAGE_BLOCKS, m_properties.limits.maxDescriptorSetStorageBuffers );
		context.getValue( GL_VALUE_NAME_MAX_COMBINED_SHADER_STORAGE_BLOCKS, m_properties.limits.maxDescriptorSetStorageBuffersDynamic );
		context.getValue( GL_VALUE_NAME_MAX_COMBINED_TEXTURE_IMAGE_UNITS, m_properties.limits.maxDescriptorSetSampledImages );
		context.getValue( GL_VALUE_NAME_MAX_COMBINED_IMAGE_UNIFORMS, m_properties.limits.maxDescriptorSetStorageImages );
		m_properties.limits.maxDescriptorSetInputAttachments = NonAvailable< uint32_t >;
		context.getValue( GL_VALUE_NAME_MAX_VERTEX_ATTRIBS, m_properties.limits.maxVertexInputAttributes );
		context.getValue( GL_VALUE_NAME_MAX_VERTEX_ATTRIB_BINDINGS, m_properties.limits.maxVertexInputBindings );
		context.getValue( GL_VALUE_NAME_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET, m_properties.limits.maxVertexInputAttributeOffset );
		context.getValue( GL_VALUE_NAME_MAX_VERTEX_ATTRIB_STRIDE, m_properties.limits.maxVertexInputBindingStride );
		context.getValue( GL_VALUE_NAME_MAX_VERTEX_OUTPUT_COMPONENTS, m_properties.limits.maxVertexOutputComponents );
		context.getValue( GL_VALUE_NAME_MAX_TESS_GEN_LEVEL, m_properties.limits.maxTessellationGenerationLevel );
		context.getValue( GL_VALUE_NAME_MAX_PATCH_VERTICES, m_properties.limits.maxTessellationPatchSize );
		context.getValue( GL_VALUE_NAME_MAX_TESS_EVALUATION_INPUT_COMPONENTS, m_properties.limits.maxTessellationControlPerVertexInputComponents );
		context.getValue( GL_VALUE_NAME_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS, m_properties.limits.maxTessellationControlPerVertexOutputComponents );
		context.getValue( GL_VALUE_NAME_MAX_TESS_PATCH_COMPONENTS, m_properties.limits.maxTessellationControlPerPatchOutputComponents );
		context.getValue( GL_VALUE_NAME_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS, m_properties.limits.maxTessellationControlTotalOutputComponents );
		context.getValue( GL_VALUE_NAME_MAX_TESS_EVALUATION_INPUT_COMPONENTS, m_properties.limits.maxTessellationEvaluationInputComponents );
		context.getValue( GL_VALUE_NAME_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS, m_properties.limits.maxTessellationEvaluationOutputComponents );
		context.getValue( GL_VALUE_NAME_MAX_GEOMETRY_SHADER_INVOCATIONS, m_properties.limits.maxGeometryShaderInvocations );
		context.getValue( GL_VALUE_NAME_MAX_GEOMETRY_INPUT_COMPONENTS, m_properties.limits.maxGeometryInputComponents );
		context.getValue( GL_VALUE_NAME_MAX_GEOMETRY_OUTPUT_COMPONENTS, m_properties.limits.maxGeometryOutputComponents );
		context.getValue( GL_VALUE_NAME_MAX_GEOMETRY_OUTPUT_VERTICES, m_properties.limits.maxGeometryOutputVertices );
		context.getValue( GL_VALUE_NAME_MAX_GEOMETRY_OUTPUT_COMPONENTS, m_properties.limits.maxGeometryTotalOutputComponents );
		context.getValue( GL_VALUE_NAME_MAX_FRAGMENT_INPUT_COMPONENTS, m_properties.limits.maxFragmentInputComponents );
		context.getValue( GL_VALUE_NAME_MAX_DRAW_BUFFERS, m_properties.limits.maxFragmentOutputAttachments );
		context.getValue( GL_VALUE_NAME_MAX_DUAL_SOURCE_DRAW_BUFFERS, m_properties.limits.maxFragmentDualSrcAttachments );
		context.getValue( GL_VALUE_NAME_MAX_COLOR_ATTACHMENTS, m_properties.limits.maxFragmentCombinedOutputResources );
		context.getValue( GL_VALUE_NAME_MAX_COMPUTE_SHARED_MEMORY_SIZE, m_properties.limits.maxComputeSharedMemorySize );
		context.getValuesI( GL_VALUE_NAME_MAX_COMPUTE_WORK_GROUP_COUNT, m_properties.limits.maxComputeWorkGroupCount );
		context.getValue( GL_VALUE_NAME_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, m_properties.limits.maxComputeWorkGroupInvocations );
		context.getValuesI( GL_VALUE_NAME_MAX_COMPUTE_WORK_GROUP_SIZE, m_properties.limits.maxComputeWorkGroupSize );
		m_properties.limits.subPixelPrecisionBits = NonAvailable< uint32_t >;
		m_properties.limits.subTexelPrecisionBits = NonAvailable< uint32_t >;
		m_properties.limits.mipmapPrecisionBits = NonAvailable< uint32_t >;
		context.getValue( GL_VALUE_NAME_MAX_ELEMENTS_INDICES, m_properties.limits.maxDrawIndexedIndexValue );
		m_properties.limits.maxDrawIndirectCount = NonAvailable< uint32_t >;
		context.getValue( GL_VALUE_NAME_MAX_TEXTURE_LOD_BIAS, m_properties.limits.maxSamplerLodBias );
		context.getValue( GL_VALUE_NAME_MAX_TEXTURE_MAX_ANISOTROPY, m_properties.limits.maxSamplerAnisotropy );
		context.getValue( GL_VALUE_NAME_MAX_VIEWPORTS, m_properties.limits.maxViewports );
		context.getValues( GL_VALUE_NAME_MAX_VIEWPORT_DIMS, m_properties.limits.maxViewportDimensions );
		context.getValues( GL_VALUE_NAME_MAX_VIEWPORT_DIMS, m_properties.limits.viewportBoundsRange );
		m_properties.limits.viewportSubPixelBits = NonAvailable< uint32_t >;
		m_properties.limits.minMemoryMapAlignment = NonAvailable< size_t >;
		m_properties.limits.minTexelBufferOffsetAlignment = NonAvailable< uint64_t >;
		context.getValue( GL_VALUE_NAME_UNIFORM_BUFFER_OFFSET_ALIGNMENT, m_properties.limits.minUniformBufferOffsetAlignment );
		m_properties.limits.minStorageBufferOffsetAlignment = NonAvailable< uint64_t >;
		context.getValue( GL_VALUE_NAME_MIN_PROGRAM_TEXEL_OFFSET, m_properties.limits.minTexelOffset );
		context.getValue( GL_VALUE_NAME_MAX_PROGRAM_TEXEL_OFFSET, m_properties.limits.maxTexelOffset );
		context.getValue( GL_VALUE_NAME_MIN_PROGRAM_TEXTURE_GATHER_OFFSET, m_properties.limits.minTexelGatherOffset );
		context.getValue( GL_VALUE_NAME_MAX_PROGRAM_TEXTURE_GATHER_OFFSET, m_properties.limits.maxTexelGatherOffset );
		m_properties.limits.minInterpolationOffset = NonAvailable< float >;
		m_properties.limits.maxInterpolationOffset = NonAvailable< float >;
		m_properties.limits.subPixelInterpolationOffsetBits = NonAvailable< uint32_t >;
		context.getValue( GL_VALUE_NAME_MAX_FRAMEBUFFER_WIDTH, m_properties.limits.maxFramebufferWidth );
		context.getValue( GL_VALUE_NAME_MAX_FRAMEBUFFER_HEIGHT, m_properties.limits.maxFramebufferHeight );
		context.getValue( GL_VALUE_NAME_MAX_FRAMEBUFFER_LAYERS, m_properties.limits.maxFramebufferLayers );
		context.getValue( GL_VALUE_NAME_MAX_FRAMEBUFFER_SAMPLES, *reinterpret_cast< uint32_t * >( &m_properties.limits.framebufferColorSampleCounts ) );
		context.getValue( GL_VALUE_NAME_MAX_FRAMEBUFFER_SAMPLES, *reinterpret_cast< uint32_t * >( &m_properties.limits.framebufferDepthSampleCounts ) );
		context.getValue( GL_VALUE_NAME_MAX_FRAMEBUFFER_SAMPLES, *reinterpret_cast< uint32_t * >( &m_properties.limits.framebufferStencilSampleCounts ) );
		context.getValue( GL_VALUE_NAME_MAX_FRAMEBUFFER_SAMPLES, *reinterpret_cast< uint32_t * >( &m_properties.limits.framebufferNoAttachmentsSampleCounts ) );
		context.getValue( GL_VALUE_NAME_MAX_COLOR_ATTACHMENTS, m_properties.limits.maxColorAttachments );
		context.getValue( GL_VALUE_NAME_MAX_COLOR_TEXTURE_SAMPLES, *reinterpret_cast< uint32_t * >( &m_properties.limits.sampledImageColorSampleCounts ) );
		context.getValue( GL_VALUE_NAME_MAX_COLOR_TEXTURE_SAMPLES, *reinterpret_cast< uint32_t * >( &m_properties.limits.sampledImageIntegerSampleCounts ) );
		context.getValue( GL_VALUE_NAME_MAX_COLOR_TEXTURE_SAMPLES, *reinterpret_cast< uint32_t * >( &m_properties.limits.sampledImageDepthSampleCounts ) );
		context.getValue( GL_VALUE_NAME_MAX_COLOR_TEXTURE_SAMPLES, *reinterpret_cast< uint32_t * >( &m_properties.limits.sampledImageStencilSampleCounts ) );
		context.getValue( GL_VALUE_NAME_MAX_COLOR_TEXTURE_SAMPLES, *reinterpret_cast< uint32_t * >( &m_properties.limits.storageImageSampleCounts ) );
		context.getValue( GL_VALUE_NAME_MAX_SAMPLE_MASK_WORDS, m_properties.limits.maxSampleMaskWords );
		m_properties.limits.timestampComputeAndGraphics = true;
		m_properties.limits.timestampPeriod = 1;
		context.getValue( GL_VALUE_NAME_MAX_CLIP_DISTANCES, m_properties.limits.maxClipDistances );
		context.getValue( GL_VALUE_NAME_MAX_CULL_DISTANCES, m_properties.limits.maxCullDistances );
		context.getValue( GL_VALUE_NAME_MAX_COMBINED_CLIP_AND_CULL_DISTANCES, m_properties.limits.maxCombinedClipAndCullDistances );
		m_properties.limits.discreteQueuePriorities = 2u;
		context.getValues( GL_VALUE_NAME_POINT_SIZE_RANGE, m_properties.limits.pointSizeRange );
		context.getValues( GL_VALUE_NAME_ALIASED_LINE_WIDTH_RANGE, m_properties.limits.lineWidthRange );
		context.getValue( GL_VALUE_NAME_POINT_SIZE_GRANULARITY, m_properties.limits.pointSizeGranularity );
		context.getValue( GL_VALUE_NAME_LINE_WIDTH_GRANULARITY, m_properties.limits.lineWidthGranularity );
		m_properties.limits.strictLines = true;
		m_properties.limits.standardSampleLocations = false;
		m_properties.limits.optimalBufferCopyOffsetAlignment = NonAvailable< uint64_t >;
		m_properties.limits.optimalBufferCopyRowPitchAlignment = NonAvailable< uint64_t >;
		m_properties.limits.nonCoherentAtomSize = 64ull;

		m_properties.sparseProperties.residencyAlignedMipSize = false;
		m_properties.sparseProperties.residencyNonResidentStrict = false;
		m_properties.sparseProperties.residencyStandard2DBlockShape = false;
		m_properties.sparseProperties.residencyStandard2DMultisampleBlockShape = false;
		m_properties.sparseProperties.residencyStandard3DBlockShape = false;
	}

	void PhysicalDevice::doInitialiseQueueProperties( ContextLock & context )
	{
		m_queueProperties.push_back(
			{
				0xFF,
				1u,
				64u,
				{
					1u,
					1u,
					1u,
				}
			} );
	}

	void PhysicalDevice::doInitialiseFormatProperties( ContextLock & context )
	{
		if ( !find( ARB_internalformat_query2 ) )
		{
			for ( VkFormat fmt = VK_FORMAT_BEGIN_RANGE; fmt < VK_FORMAT_END_RANGE; fmt = VkFormat( fmt + 1 ) )
			{
				m_formatProperties[fmt].optimalTilingFeatures = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT
					| VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT
					| VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT
					| VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT
					| VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT
					| VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT
					| VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT
					| ( ashes::isDepthOrStencilFormat( fmt ) ? VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT : 0 )
					| VK_FORMAT_FEATURE_BLIT_SRC_BIT
					| VK_FORMAT_FEATURE_BLIT_DST_BIT
					| VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT
					| VK_FORMAT_FEATURE_TRANSFER_SRC_BIT
					| VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
				m_formatProperties[fmt].linearTilingFeatures = m_formatProperties[fmt].optimalTilingFeatures;
			}
		}
		else
		{
			assert( context->m_glGetInternalformativ );

			for ( VkFormat fmt = VK_FORMAT_BEGIN_RANGE; fmt < VK_FORMAT_END_RANGE; fmt = VkFormat( fmt + 1 ) )
			{
				if ( isSupportedInternal( fmt ) )
				{
					auto internal = getInternalFormat( fmt );
					GLint value;
					glLogCall( context, glGetInternalformativ
						, GL_TEXTURE_2D
						, internal
						, GL_FORMAT_PROPERTY_SUPPORTED
						, 1
						, &value );

					if ( value == GL_TRUE )
					{
#if defined( VK_KHR_maintenance ) || defined( VK_API_VERSION_1_1 )
						m_formatProperties[fmt].optimalTilingFeatures |= VK_FORMAT_FEATURE_BLIT_SRC_BIT;
						m_formatProperties[fmt].optimalTilingFeatures |= VK_FORMAT_FEATURE_BLIT_DST_BIT;
#endif
						glLogCall( context, glGetInternalformativ
							, GL_TEXTURE_2D
							, internal
							, GL_FORMAT_PROPERTY_RENDERABLE
							, 1
							, &value );

						if ( value == GL_FULL_SUPPORT )
						{
							if ( isDepthOrStencilFormat( fmt ) )
							{
								m_formatProperties[fmt].optimalTilingFeatures |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
							}
							else
							{
								m_formatProperties[fmt].optimalTilingFeatures |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
							}
						}

						glLogCall( context, glGetInternalformativ
							, GL_TEXTURE_2D
							, internal
							, GL_FORMAT_PROPERTY_BLEND
							, 1
							, &value );

						if ( value == GL_FULL_SUPPORT )
						{
							m_formatProperties[fmt].optimalTilingFeatures |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
							m_formatProperties[fmt].optimalTilingFeatures |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT;
						}

						glLogCall( context, glGetInternalformativ
							, GL_TEXTURE_2D
							, internal
							, GL_FORMAT_PROPERTY_FRAGMENT_TEXTURE
							, 1
							, &value );

						if ( value == GL_FULL_SUPPORT )
						{
							m_formatProperties[fmt].optimalTilingFeatures |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
						}

						glLogCall( context, glGetInternalformativ
							, GL_TEXTURE_2D
							, internal
							, GL_FORMAT_PROPERTY_FILTER
							, 1
							, &value );

						if ( value == GL_FULL_SUPPORT )
						{
							m_formatProperties[fmt].optimalTilingFeatures |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
						}

						glLogCall( context, glGetInternalformativ
							, GL_TEXTURE_2D
							, internal
							, GL_FORMAT_PROPERTY_SHADER_IMAGE_LOAD
							, 1
							, &value );

						if ( value == GL_FULL_SUPPORT )
						{
							m_formatProperties[fmt].optimalTilingFeatures |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
						}

						glLogCall( context, glGetInternalformativ
							, GL_TEXTURE_2D
							, internal
							, GL_FORMAT_PROPERTY_SHADER_IMAGE_ATOMIC
							, 1
							, &value );

						if ( value == GL_FULL_SUPPORT )
						{
							m_formatProperties[fmt].optimalTilingFeatures |= VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT;
						}

#if defined( VK_KHR_maintenance ) || defined( VK_API_VERSION_1_1 )
						glLogCall( context, glGetInternalformativ
							, GL_TEXTURE_2D
							, internal
							, GL_FORMAT_PROPERTY_READ_PIXELS
							, 1
							, &value );

						if ( value == GL_FULL_SUPPORT )
						{
							m_formatProperties[fmt].optimalTilingFeatures |= VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
							m_formatProperties[fmt].optimalTilingFeatures |= VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
						}
#endif
					}
				}

				m_formatProperties[fmt].linearTilingFeatures = m_formatProperties[fmt].optimalTilingFeatures;
			}
		}
	}

	void PhysicalDevice::doInitialiseDisplayProperties( ContextLock & context )
	{
#ifdef VK_KHR_display

		auto count = getScreenCount();

		for ( uint32_t i = 0u; i < count; ++i )
		{
			std::string name;
			VkDisplayPropertiesKHR displayProps{};
			std::vector< VkDisplayModeParametersKHR > displayModesParams;
			getScreenDesc( i, name, displayProps, displayModesParams );
			m_displayNames.emplace_back( std::move( name ) );
			displayProps.displayName = m_displayNames.back().c_str();
			allocate( displayProps.display
				, nullptr
				, displayProps
				, VK_FORMAT_R8G8B8A8_UNORM
				, i
				, displayModesParams );
			m_displays.push_back( displayProps );
		}

		for ( auto & display : m_displays )
		{
			m_displayPlanes.push_back( { display.display, 0u } );
		}

#endif
	}

	void PhysicalDevice::doInitialiseProperties2( ContextLock & context )
	{
#if VK_VERSION_1_1

		m_features2.pNext = nullptr;
		m_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		m_features2.features = m_features;

		m_properties2.pNext = nullptr;
		m_properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		m_properties2.properties = m_properties;

		for ( auto & queueProperty : m_queueProperties )
		{
			m_queueProperties2.push_back(
				{
					VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2,
					nullptr,
					queueProperty,
				} );
		}

		for ( auto & formatProperty : m_formatProperties )
		{
			m_formatProperties2.emplace( formatProperty.first
				, VkFormatProperties2
				{
					VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2,
					nullptr,
					formatProperty.second,
				} );
		}

#elif VK_KHR_get_physical_device_properties2

		m_features2.pNext = nullptr;
		m_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
		m_features2.features = m_features;

		m_properties2.pNext = nullptr;
		m_properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
		m_properties2.properties = m_properties;

		for ( auto & queueProperty : m_queueProperties )
		{
			m_queueProperties2.push_back(
				{
					VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2_KHR,
					nullptr,
					queueProperty,
				} );
		}

		for ( auto & formatProperty : m_formatProperties )
		{
			m_formatProperties2.emplace( formatProperty.first
				, VkFormatProperties2
				{
					VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2_KHR,
					nullptr,
					formatProperty.second,
				} );
		}

#endif
	}

	bool has420PackExtensions( VkPhysicalDevice physicalDevice )
	{
		return get( physicalDevice )->getGlFeatures().has420PackExtensions;
	}

	bool hasCopyImage( VkPhysicalDevice physicalDevice )
	{
		return get( physicalDevice )->getGlFeatures().hasCopyImage;
	}

	bool hasProgramPipelines( VkPhysicalDevice physicalDevice )
	{
		return get( physicalDevice )->getGlFeatures().hasProgramPipelines;
	}

	bool hasSamplerAnisotropy( VkPhysicalDevice physicalDevice )
	{
		return get( physicalDevice )->getFeatures().samplerAnisotropy;
	}

	bool hasTextureStorage( VkPhysicalDevice physicalDevice )
	{
		return get( physicalDevice )->getGlFeatures().hasTextureStorage;
	}

	bool hasTextureViews( VkPhysicalDevice physicalDevice )
	{
		return get( physicalDevice )->getGlFeatures().hasTextureViews;
	}

	bool hasViewportArrays( VkPhysicalDevice physicalDevice )
	{
		return get( physicalDevice )->getGlFeatures().hasViewportArrays;
	}

	bool hasProgramInterfaceQuery( VkPhysicalDevice physicalDevice )
	{
		return get( physicalDevice )->getGlFeatures().hasProgramInterfaceQuery;
	}
}
