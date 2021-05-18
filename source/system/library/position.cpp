#include <stdlib.h>
#include "position.hpp"

namespace position
{
    sf::Vector2f getSidePosition(sf::FloatRect anchor, sf::FloatRect object, sf::Vector2f distance, sf::Vector2i side)
    {
		sf::Vector2f positionSide(anchor.left, anchor.top);

		switch (side.x)
		{
			case (0):
			{
				positionSide.x += distance.x;
				break;
			}
			case (1):
			{
				positionSide.x += anchor.width + distance.x;
				break;
			}
			case (-1):
			{
				positionSide.x -= (object.width + distance.x);
				break;
			}
		}

		switch (side.y)
		{
			case (0):
			{
				positionSide.y += distance.y;
				break;
			}
			case (1):
			{
				positionSide.y += anchor.height + distance.y;
				break;
			}
			case (-1):
			{
				positionSide.y -= (object.height + distance.y);
				break;
			}
		}

		return positionSide;
    }

	sf::Vector2f getGridPosition(sf::Vector2f gridSize, sf::Vector2f position)
	{
		return sf::Vector2f(div((int)position.x, gridSize.x).quot * gridSize.x, div((int)position.y, gridSize.y).quot * gridSize.y);
	}
}