#include "manager.hpp"

Manager::Manager()
{
    this->unloadAll();

	this->window = std::make_shared<sf::RenderWindow>(sf::VideoMode(1380, 860), "realm-editor", sf::Style::Default);

    this->font = std::shared_ptr<sf::Font>(new sf::Font);
    this->font->loadFromFile("resources/fonts/consola.ttf");

    this->addView(ObjectType::otModel, std::static_pointer_cast<ViewElement>(std::make_shared<Model>(this, sf::Vector2f(0.f, 0.f), "textures/terrain/castle-cobblestone")));
    this->addView(ObjectType::otText, std::static_pointer_cast<ViewElement>(std::make_shared<Text>(this, "Hello world", 20, sf::Vector2f(0.f, 20.f))));
    this->addView(ObjectType::otText, std::static_pointer_cast<ViewElement>(std::make_shared<Text>(this, "Hello world", 20, sf::Vector2f(0.f, 25.f), 2)));
    this->addView(ObjectType::otText, std::static_pointer_cast<ViewElement>(std::make_shared<Text>(this, "Hello world", 20, sf::Vector2f(0.f, 30.f))));
    this->addView(ObjectType::otModel, std::static_pointer_cast<ViewElement>(std::make_shared<Model>(this, sf::Vector2f(20.f, 20.f), "textures/terrain/castle-cobblestone")));
}

Manager::~Manager()
{
    this->unloadAll();
}

bool Manager::addViewElement(std::shared_ptr<ViewElement> element)
{
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

bool Manager::addView(ObjectType type, std::shared_ptr<ViewElement> element)
{
    switch (type)
    {
        case (ObjectType::otModel):
        {
            this->list.models.emplace_back(element);
            this->addViewElement(element);
            break;
        }
        case (ObjectType::otText):
        {
            this->list.texts.emplace_back(element);
            this->addViewElement(element);
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