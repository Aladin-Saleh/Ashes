/*
This file belongs to Ashes.
See LICENSE file in root folder
*/
#include "GlX11EglContext.hpp"

#if __linux__
#	if ASHES_USE_XLIB_EGL

#	include <unistd.h>
#	include <iostream>
#	include <sstream>
#	include <cstdlib>
#	include <cstring>
#	include <cstdio>

namespace ashes::gl
{
	X11EglContext::X11EglContext( VkInstance instance
		, VkXlibSurfaceCreateInfoKHR createInfo
		, ContextImpl const * mainContext )
		: ContextImpl{ instance }
		, createInfo{ std::move( createInfo ) }
		, m_mainContext{ static_cast< X11EglContext const * >( mainContext ) }
	{
	}

	X11EglContext::~X11EglContext()
	{
	}

	void X11EglContext::preInitialise( int reqMajor, int reqMinor )
	{
		m_context = std::make_unique< ContextEgl >( createInfo.dpy
			, createInfo.window
			, reqMajor
			, reqMinor
			, ( m_mainContext
				? m_mainContext->m_context->getContext()
				: EGL_NO_CONTEXT ) );
	}

	void X11EglContext::postInitialise()
	{
	}

	void X11EglContext::enable()const
	{
		m_context->enable();
	}

	void X11EglContext::disable()const
	{
		m_context->disable();
	}

	void X11EglContext::swapBuffers()const
	{
		m_context->swap();
	}
}

#	endif
#endif
