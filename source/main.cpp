#include <SFML/Graphics.hpp>
#include <iostream>
#include <stdlib.h>
#include <vector>

//#include "system/manager.hpp"

int main()
{
    sf::RenderWindow *window = new sf::RenderWindow(sf::VideoMode(800, 600), "Lamina", sf::Style::Default);

    /*Manager *manager = new Manager;
    manager->window = window;
    manager->setView();*/

    while (window->isOpen())
    {
        /*sf::Event event;

        while (window->pollEvent(event))
            manager->event(event);

        manager->update();*/

        // Process events
        sf::Event event;
        while (window->pollEvent(event))
        {
            // Close window: exit
            if (event.type == sf::Event::Closed)
                window->close();
        }
        // Clear screen
        window->clear();
        // Draw the sprite
        window->display();
    }

    //delete manager;

    return 0;
}
