#include "Descriptor/VkDescriptorSetLayout.hpp"

#include "Descriptor/VkDescriptorSetLayoutBinding.hpp"
#include "Descriptor/VkDescriptorSetPool.hpp"
#include "Core/VkDevice.hpp"

namespace vk_renderer
{
	DescriptorSetLayout::DescriptorSetLayout( Device const & device
		, renderer::DescriptorSetLayoutBindingArray && bindings )
		: renderer::DescriptorSetLayout{ device, std::move( bindings ) }
		, m_device{ device }
	{
		auto vkbindings = convert< VkDescriptorSetLayoutBinding >( m_bindings );
		VkDescriptorSetLayoutCreateInfo createInfo
		{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,  // sType
			nullptr,                                              // pNext
			0,                                                    // flags
			static_cast< uint32_t >( vkbindings.size() ),         // bindingCount
			vkbindings.data()                                     // pBindings
		};
		DEBUG_DUMP( createInfo );
		auto res = CreateDescriptorSetLayout( m_device
			, &createInfo
			, nullptr
			, &m_layout );

		if ( !checkError( res ) )
		{
			throw std::runtime_error{ "Descriptor set layout creation failed: " + getLastError() };
		}
	}

	renderer::DescriptorSetPoolPtr DescriptorSetLayout::createPool( uint32_t maxSets )const
	{
		return std::make_unique< DescriptorSetPool >( m_device
			, *this
			, maxSets );
	}
}