/*
This file belongs to Renderer.
See LICENSE file in root folder.
*/
#include "ColourBlendState.hpp"

namespace renderer
{
	namespace
	{
		ColourBlendState doCreateDefault()
		{
			ColourBlendState result;
			result.addAttachment( ColourBlendStateAttachment{} );
			return result;
		}
	}

	ColourBlendState::ColourBlendState( bool logicOpEnable
		, LogicOp logicOp
		, utils::Vec4 const & blendConstants )
		: m_logicOpEnable{ logicOpEnable }
		, m_logicOp{ logicOp }
		, m_blendConstants{ blendConstants }
	{
	}

	ColourBlendState const & ColourBlendState::createDefault()
	{
		static ColourBlendState const default = doCreateDefault();
		return default;
	}

	void ColourBlendState::addAttachment( ColourBlendStateAttachment const & attachment )
	{
		m_attachs.push_back( attachment );
	}
}
