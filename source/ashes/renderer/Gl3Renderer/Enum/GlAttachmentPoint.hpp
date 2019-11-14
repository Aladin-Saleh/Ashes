/*
This file belongs to Ashes.
See LICENSE file in root folder
*/
#pragma once

namespace ashes::gl3
{
	enum GlAttachmentPoint
		: GLenum
	{
		GL_ATTACHMENT_POINT_FRONT = 0x0404,
		GL_ATTACHMENT_POINT_FRONT_LEFT = 0x0400,
		GL_ATTACHMENT_POINT_FRONT_RIGHT = 0x0401,
		GL_ATTACHMENT_POINT_BACK = 0x0405,
		GL_ATTACHMENT_POINT_BACK_LEFT = 0x0402,
		GL_ATTACHMENT_POINT_BACK_RIGHT = 0x0403,
		GL_ATTACHMENT_POINT_DEPTH_STENCIL = 0x821A,
		GL_ATTACHMENT_POINT_COLOR0 = 0x8CE0,
		GL_ATTACHMENT_POINT_COLOR1 = 0x8CE1,
		GL_ATTACHMENT_POINT_COLOR2 = 0x8CE2,
		GL_ATTACHMENT_POINT_COLOR3 = 0x8CE3,
		GL_ATTACHMENT_POINT_COLOR4 = 0x8CE4,
		GL_ATTACHMENT_POINT_COLOR5 = 0x8CE5,
		GL_ATTACHMENT_POINT_COLOR6 = 0x8CE6,
		GL_ATTACHMENT_POINT_COLOR7 = 0x8CE7,
		GL_ATTACHMENT_POINT_COLOR8 = 0x8CE8,
		GL_ATTACHMENT_POINT_COLOR9 = 0x8CE9,
		GL_ATTACHMENT_POINT_COLOR10 = 0x8CEA,
		GL_ATTACHMENT_POINT_COLOR11 = 0x8CEB,
		GL_ATTACHMENT_POINT_COLOR12 = 0x8CEC,
		GL_ATTACHMENT_POINT_COLOR13 = 0x8CED,
		GL_ATTACHMENT_POINT_COLOR14 = 0x8CEE,
		GL_ATTACHMENT_POINT_COLOR15 = 0x8CEF,
		GL_ATTACHMENT_POINT_DEPTH = 0x8D00,
		GL_ATTACHMENT_POINT_STENCIL = 0x8D20,
	};
	std::string getName( GlAttachmentPoint value );
}