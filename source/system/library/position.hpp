#include <iostream>
#include <SFML/Graphics.hpp>

#pragma once

#ifndef POSITION_HPP
#define POSITION_HPP

namespace position
{
    sf::Vector2f getSidePosition(sf::FloatRect anchor, sf::FloatRect object, sf::Vector2f distance, sf::Vector2i side);
}

#endif