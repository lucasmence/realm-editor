#include "manager.hpp"

Manager::Manager()
{
	this->window = std::shared_ptr<sf::RenderWindow>(new sf::RenderWindow(sf::VideoMode(1380, 860), "realm-editor", sf::Style::Default));
}

Manager::~Manager()
{

}

bool Manager::update()
{
    this->event();

    this->window->clear();
    this->window->display();

	return true;
}

bool Manager::event()
{
    sf::Event event;
    while (this->window->pollEvent(event))
    {
        switch (event.type)
        {
            case sf::Event::Closed:
            {
                this->window->close();
                break;
            }

            case sf::Event::MouseButtonPressed:
            {
                this->eventClick(event);
                break;
            }
        }            
    }

	return true;
}

bool Manager::eventClick(sf::Event& event)
{
    return false;
}

bool Manager::unloadAll()
{
	return true;
}