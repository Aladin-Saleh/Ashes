/*
This file belongs to Renderer.
See LICENSE file in root folder.
*/
#ifndef ___Renderer_Connection_HPP___
#define ___Renderer_Connection_HPP___
#pragma once

#include "WindowHandle.hpp"

namespace renderer
{
	/**
	*\brief
	*	Encapsulation de la connexion à une application dépendant de la plateforme.
	*/
	class Connection
	{
	protected:
		Connection( Connection const & ) = delete;
		Connection & operator=( Connection const & ) = delete;
		Connection( Connection && rhs ) = default;
		Connection & operator=( Connection && rhs ) = default;
		/**
		*\brief
		*	Constructeur.
		*\param[in] renderer
		*	L'instance de Renderer.
		*\param[in] deviceIndex
		*	L'indice du périphérique physique.
		*\param[in] handle
		*	Le descripteur de la fenêtre.
		*/
		Connection( Renderer const & renderer
			, uint32_t deviceIndex
			, WindowHandle && handle );

	public:
		/**
		*\~english
		*\brief
		*	Destructor.
		*\~french
		*\brief
		*	Destructeur.
		*/
		virtual ~Connection() = default;

	protected:
		WindowHandle m_handle;
	};
}

#endif
