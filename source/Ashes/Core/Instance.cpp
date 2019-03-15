/*
This file belongs to Ashes.
See LICENSE file in root folder.
*/
#include "Ashes/Core/Instance.hpp"

#include "Ashes/Core/Connection.hpp"
#include "Ashes/Miscellaneous/QueueCreateInfo.hpp"
#include "Ashes/Utils/CallStack.hpp"

#include <cmath>

namespace ashes
{
	Instance::Instance( ClipDirection clipDirection
		, std::string const & name
		, Configuration const & configuration )
		: m_configuration{ configuration }
		, m_clipDirection{ clipDirection }
		, m_name{ name }
	{
#ifndef NDEBUG

		Debug::initialise();

#endif
	}

	Instance::~Instance()
	{
#ifndef NDEBUG

		Debug::cleanup();

#endif
	}

	PhysicalDevice & Instance::getPhysicalDevice( uint32_t gpuIndex )const
	{
		if ( gpuIndex >= m_gpus.size() )
		{
			throw std::logic_error( "Invalid GPU index" );
		}

		return *m_gpus[gpuIndex];
	}

	std::array< float, 16 > Instance::infinitePerspective( float radiansFovY
		, float aspect
		, float zNear )const
	{
		float const range = tan( radiansFovY / float( 2 ) ) * zNear;
		float const left = -range * aspect;
		float const right = range * aspect;
		float const bottom = -range;
		float const top = range;

		std::array< float, 16 > result{ 0.0f };
		result[0] = ( float( 2 ) * zNear ) / ( right - left );
		result[5] = ( float( 2 ) * zNear ) / ( top - bottom );
		result[10] = -float( 1 );
		result[11] = -float( 1 );
		result[14] = -float( 2 ) * zNear;
		return result;
	}

	DevicePtr Instance::createDevice( ConnectionPtr connection
		, uint32_t presentQueueFamilyIndex
		, uint32_t graphicsQueueFamilyIndex
		, uint32_t computeQueueFamilyIndex )const
	{
		std::vector< float > queuePriorities = { 1.0f };
		DeviceQueueCreateInfoArray queueCreateInfos;

		if ( graphicsQueueFamilyIndex != uint32_t( ~( 0u ) ) )
		{
			queueCreateInfos.push_back(
				{
					DeviceQueueCreateFlag::eNone,
					graphicsQueueFamilyIndex,
					queuePriorities,
				} );
		}

		if ( presentQueueFamilyIndex != graphicsQueueFamilyIndex )
		{
			queueCreateInfos.push_back(
				{
					DeviceQueueCreateFlag::eNone,
					presentQueueFamilyIndex,
					queuePriorities,
				} );
		}

		if ( computeQueueFamilyIndex != graphicsQueueFamilyIndex )
		{
			queueCreateInfos.push_back(
				{
					DeviceQueueCreateFlag::eNone,
					computeQueueFamilyIndex,
					queuePriorities,
				} );
		}

		return createDevice( std::move( connection )
			, queueCreateInfos
			, m_layerNames
			, m_extensionNames
			, connection->getGpu().getFeatures() );
	}
}