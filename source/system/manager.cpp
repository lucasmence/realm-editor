#include "manager.hpp"

Manager::Manager()
{
    this->unloadAll();

	this->window = std::make_shared<sf::RenderWindow>(sf::VideoMode(1380, 860), "realm-editor", sf::Style::Default);

    this->font = std::shared_ptr<sf::Font>(new sf::Font);
    this->font->loadFromFile("resources/fonts/consola.ttf");

    this->addView(ObjectType::otModel, std::static_pointer_cast<ViewElement>(std::make_shared<Model>(this, "textures/terrain/castle-cobblestone")));
    this->addView(ObjectType::otText, std::static_pointer_cast<ViewElement>(std::make_shared<Text>(this, "Hello world", 20, sf::Vector2f(50.f, 50.f))));
}

Manager::~Manager()
{
    this->unloadAll();
}

bool Manager::addView(ObjectType type, std::shared_ptr<ViewElement> element)
{
    switch (type)
    {
        case (ObjectType::otModel):
        {
            this->list.models.emplace_back(element);
            this->list.viewElements.emplace_back(element);
            break;
        }
        case (ObjectType::otText):
        {
            this->list.texts.emplace_back(element);
            this->list.viewElements.emplace_back(element);
            break;
        }
    }

    return true;
}

bool Manager::removeView(ObjectType type, std::shared_ptr<ViewElement> element)
{
    switch (type)
    {
        case (ObjectType::otModel):
        {
            this->list.models.remove(element);
            this->list.viewElements.remove(element);
            break;
        }
        case (ObjectType::otText):
        {
            this->list.texts.remove(element);
            this->list.viewElements.remove(element);
            break;
        }
    }

    return true;
}

bool Manager::update()
{
    this->event();

    this->window->clear();

    for (auto& element : this->list.viewElements)
        element->draw();

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
    this->list.textures.clear();
    this->list.models.clear();
    this->list.texts.clear();
    this->list.viewElements.clear();
	return true;
}