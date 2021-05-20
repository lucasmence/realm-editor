#include <iostream>
#include <SFML/Graphics.hpp>

#pragma once

#ifndef POSITION_HPP
#define POSITION_HPP

namespace position
{
    sf::Vector2f getSidePosition(sf::FloatRect anchor, sf::FloatRect object, sf::Vector2f distance, sf::Vector2i side);
    sf::Vector2f getGridPosition(sf::Vector2f gridSize, sf::Vector2f position);
    sf::Vector2f getCenterPosition(sf::Vector2f screen, sf::FloatRect object, sf::Vector2i side);
}

#endif