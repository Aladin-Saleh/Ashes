#include "Core/GlContext.hpp"

#include "Core/GlContextLock.hpp"
#include "Core/GlSurface.hpp"

#include "ashesgl_api.hpp"

#if _WIN32
#	include <gl/GL.h>
#endif

#include <iostream>
#include <locale>

#if Ashes_LogContextLocking

#define logContextLock()\
	std::stringstream stream;\
	stream.imbue( std::locale{ "C" } );\
	stream << "Context [0x" << std::setfill( '0' ) << std::setw( 8 ) << std::hex << this << "] locked by thread [" << std::this_thread::get_id() << "]";\
	get( m_instance )->reportMessage( VK_DEBUG_REPORT_DEBUG_BIT_EXT\
		, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT\
		, uint64_t( m_instance )\
		, 0u\
		, 0u\
		, "CONTEXT"\
		, stream.str().c_str() )

#define logContextUnlock()\
	std::stringstream stream;\
	stream.imbue( std::locale{ "C" } );\
	stream << "Context [0x" << std::setfill( '0' ) << std::setw( 8 ) << std::hex << this << "] unlocked by thread [" << std::this_thread::get_id() << "]";\
	get( m_instance )->reportMessage( VK_DEBUG_REPORT_DEBUG_BIT_EXT\
		, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT\
		, uint64_t( m_instance )\
		, 0u\
		, 0u\
		, "CONTEXT"\
		, stream.str().c_str() )

#else

#define logContextLock()
#define logContextUnlock()

#endif

using ashes::operator==;
using ashes::operator!=;

namespace ashes::gl
{
	//*************************************************************************

	Context const & getContext( ContextLock const & lock )
	{
		return lock.getContext();
	}

	//*************************************************************************

	Context::Context( gl::ContextImplPtr impl )
		: m_impl{ std::move( impl ) }
		, m_instance{ m_impl->instance }
	{
		m_impl->preInitialise( MinMajor, MinMinor );
		m_impl->enable();
		loadBaseFunctions();
		m_impl->disable();
		m_impl->postInitialise();
		m_extent = m_impl->extent;
	}

	Context::~Context()
	{
	}

#if _WIN32

	ContextPtr Context::create( VkInstance instance
		, VkSurfaceKHR surface )
	{
		if ( get( surface )->isWin32() )
		{
			return create( instance
				, get( surface )->getWin32CreateInfo()
				, &get( instance )->getCurrentContext() );
		}
		else if ( get( surface )->isDisplay() )
		{
			return create( instance
				, get( surface )->getDisplayCreateInfo()
				, &get( instance )->getCurrentContext() );
		}

		return nullptr;
	}

#elif __linux__

	ContextPtr Context::create( VkInstance instance
		, VkSurfaceKHR surface )
	{
		if ( get( surface )->isXlib() )
		{
			return create( instance
				, get( surface )->getXlibCreateInfo()
				, &get( instance )->getCurrentContext() );
		}
		else if ( get( surface )->isXcb() )
		{
			return create( instance
				, get( surface )->getXcbCreateInfo()
				, &get( instance )->getCurrentContext() );
		}
		else if ( get( surface )->isWayland() )
		{
			return create( instance
				, get( surface )->getWaylandCreateInfo()
				, &get( instance )->getCurrentContext() );
		}
		else if ( get( surface )->isDisplay() )
		{
			return create( instance
				, get( surface )->getDisplayCreateInfo()
				, &get( instance )->getCurrentContext() );
		}

		return nullptr;
	}

#endif

	ContextState & Context::getState()
	{
		auto id = std::this_thread::get_id();
		auto it = m_state.find( id );

		if ( it == m_state.end() )
		{
			it = m_state.emplace( std::move( id )
				, std::make_unique< ContextState >() ).first;
			initialiseThreadState( *it->second );
		}

		return *it->second;
	}

#if VK_EXT_debug_utils

	void Context::submitDebugUtilsMessenger( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity
		, VkDebugUtilsMessageTypeFlagsEXT messageTypes
		, VkDebugUtilsMessengerCallbackDataEXT const & callbackData )const
	{
		get( m_instance )->submitDebugUtilsMessenger( messageSeverity
			, messageTypes
			, callbackData );
	}

#endif
#if VK_EXT_debug_report

	void Context::reportMessage( VkDebugReportFlagsEXT flags
		, VkDebugReportObjectTypeEXT objectType
		, uint64_t object
		, size_t location
		, int32_t messageCode
		, const char * pLayerPrefix
		, const char * pMessage )
	{
		get( m_instance )->reportMessage( flags
			, objectType
			, object
			, location
			, messageCode
			, pLayerPrefix
			, pMessage );
	}

#endif

	void Context::lock()
	{
		m_mutex.lock();
		m_enabled = true;
		m_activeThread = std::this_thread::get_id();
		m_impl->enable();
		logContextLock();
	}

	void Context::unlock()
	{
		logContextUnlock();
		m_impl->disable();
		m_enabled = false;
		m_mutex.unlock();
	}

	void Context::loadBaseFunctions()
	{
#if _WIN32
#	define GL_LIB_BASE_FUNCTION( fun )\
		m_gl##fun = PFN_gl##fun( &::gl##fun );\
		if ( !m_gl##fun )\
		{\
			throw std::runtime_error{ std::string{ "Couldn't load function " } + "gl"#fun };\
		}
#else
#	define GL_LIB_BASE_FUNCTION( fun )\
		if ( !( getFunction( "gl"#fun, m_gl##fun ) ) )\
		{\
			throw std::runtime_error{ std::string{ "Couldn't load function " } + "gl"#fun };\
		}
#endif
#define GL_LIB_FUNCTION( fun )\
		if ( !( getFunction( "gl"#fun, m_gl##fun ) ) )\
		{\
			throw std::runtime_error{ std::string{ "Couldn't load function " } + "gl"#fun };\
		}

#	define GL_LIB_FUNCTION_EXT( fun, ... )\
		if ( !( getFunction( "gl"#fun, m_gl##fun, __VA_ARGS__ ) ) )\
		{\
			std::cerr << "Couldn't load function " << "gl"#fun << std::endl;\
		}
#include "Miscellaneous/OpenGLFunctionsList.inl"
	}

	void Context::initialiseThreadState( ContextState const & state )
	{
		ContextLock lock{ *this };
		glLogCall( lock
			, glEnable
			, GL_TEXTURE_CUBE_MAP_SEAMLESS );
	}

	//*************************************************************************
}
