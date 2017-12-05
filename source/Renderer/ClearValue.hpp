/*
This file belongs to Renderer.
See LICENSE file in root folder.
*/
#ifndef ___Renderer_ClearValue_HPP___
#define ___Renderer_ClearValue_HPP___
#pragma once

#include "RendererPrerequisites.hpp"

#include <Utils/Vec4.hpp>

namespace renderer
{
	/**
	*\brief
	*	Valeur de vidage profondeur/stencil.
	*/
	struct DepthStencilClearValue
	{
		float depth;
		uint32_t stencil;
	};
	/**
	*\brief
	*	Valeur de vidage, couleur ou profondeur/stencil.
	*/
	struct ClearValue
	{
		ClearValue( utils::RgbaColour const & colour );
		ClearValue( DepthStencilClearValue const & depthStencil );

		bool isColour()const
		{
			return m_isColour;
		}

		utils::RgbaColour const & colour()const
		{
			assert( m_isColour );
			return m_colour;
		}

		DepthStencilClearValue const & depthStencil()const
		{
			assert( !m_isColour );
			return m_depthStencil;
		}

	private:
		bool m_isColour{ false };
		utils::RgbaColour m_colour;
		DepthStencilClearValue m_depthStencil;
	};
}

#endif
