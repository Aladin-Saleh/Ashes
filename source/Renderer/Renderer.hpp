/*
This file belongs to Renderer.
See LICENSE file in root folder.
*/
#ifndef ___Renderer_HPP___
#define ___Renderer_HPP___
#pragma once

#include "WindowHandle.hpp"

namespace renderer
{
	class Renderer
	{
	protected:
		/**
		*\brief
		*	Constructeur, initialise l'instance de Vulkan.
		*/
		Renderer() = default;

	public:
		/**
		*\brief
		*	Destructeur.
		*/
		virtual ~Renderer() = default;
		/**
		*\brief
		*	Crée le périphérique logique.
		*\param[in] connection
		*	La connection avec la fenêtre.
		*/
		virtual DevicePtr createDevice( ConnectionPtr && connection )const = 0;
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
		virtual ConnectionPtr createConnection( uint32_t deviceIndex
			, WindowHandle && handle )const = 0;
	};
}

#endif
