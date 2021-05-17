#include "manager.hpp"

Manager::Manager()
{
    this->unloadAll();

	this->window = std::make_shared<sf::RenderWindow>(sf::VideoMode(1920, 1080), "realm-editor", sf::Style::Default);

    this->font = std::shared_ptr<sf::Font>(new sf::Font);
    this->font->loadFromFile("resources/fonts/consola.ttf");

    this->hud = std::make_shared<Hud>(this);
    this->palette = std::make_shared<Palette>(this);

    this->hasFocus = true;
    this->open = false;

    this->loadWindowOpening();
}

Manager::~Manager()
{
    this->unloadAll();
}

bool Manager::loadWindowOpening()
{
    if (this->open)
        return false;

    this->open = true;
    std::system("windowMode -title realm-editor -mode maximized");
    return true;
}

bool Manager::addViewElement(std::shared_ptr<ViewElement> element)
{
    this->list.viewElements.remove(element);

    for (auto &elementIndex : this->list.viewElements)
        if (elementIndex->priority < element->priority)
        {
            std::list<std::shared_ptr<ViewElement>>::iterator iterator = std::find(this->list.viewElements.begin(), this->list.viewElements.end(), elementIndex);
            this->list.viewElements.emplace(iterator, element);
            return true;
        }
   
    this->list.viewElements.emplace_back(element);
    return true;
}

bool Manager::addView(std::shared_ptr<ViewElement> element)
{
    this->addViewElement(element);
    return true;
}

bool Manager::removeView(std::shared_ptr<ViewElement> element)
{
    this->list.viewElements.remove(element);

    return true;
}

bool Manager::update()
{
    this->event();

    this->window->clear();

    this->hud->update(this->getMousePosition());

    for (auto& element : this->list.viewElements)
        element->draw();

    this->window->display();

	return true;
}

sf::Vector2f Manager::getMousePosition()
{
    sf::Vector2i pixelPosition = sf::Mouse::getPosition(*this->window);
    return window->mapPixelToCoords(pixelPosition);
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

            case  sf::Event::GainedFocus:
            {
                this->hasFocus = true;
                break;
            }

            case sf::Event::LostFocus:
            {
                this->hasFocus = false;
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
    this->hud->buttonsClick(this->getMousePosition());
    return false;
}

bool Manager::unloadAll()
{
    this->hud = nullptr;
    this->list.textures.clear();
    this->list.viewElements.clear();
	return true;
}