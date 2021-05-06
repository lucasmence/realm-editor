#include <list>
#include <memory>
#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#pragma once

#ifndef MANAGER_HPP
#define MANAGER_HPP

struct ManagerList
{

};

class Manager
{
	public:
		std::shared_ptr<sf::RenderWindow> window;

		Manager();
		~Manager();
		bool unloadAll();

		bool update();
		bool event();
		bool eventClick(sf::Event& event);
};

#endif