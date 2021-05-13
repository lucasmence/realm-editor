#include "manager.hpp"

Manager::Manager()
{
    this->unloadAll();

	this->window = std::make_shared<sf::RenderWindow>(sf::VideoMode(1920, 1080), "realm-editor", sf::Style::Default);

    this->font = std::shared_ptr<sf::Font>(new sf::Font);
    this->font->loadFromFile("resources/fonts/consola.ttf");

    this->hud = std::make_shared<Hud>(this);

    this->hasFocus = true;
    this->open = false;

    this->loadWindowOpening();

    /*this->addView(ObjectType::otModel, std::static_pointer_cast<ViewElement>(std::make_shared<Model>(this, sf::Vector2f(0.f, 0.f), "textures/terrain/castle-cobblestone")));
    this->addView(ObjectType::otLabel, std::static_pointer_cast<ViewElement>(std::make_shared<Label>(this, "Hello world", 20, sf::Vector2f(0.f, 20.f))));
    this->addView(ObjectType::otLabel, std::static_pointer_cast<ViewElement>(std::make_shared<Label>(this, "Hello world", 20, sf::Vector2f(0.f, 25.f), 2)));
    this->addView(ObjectType::otLabel, std::static_pointer_cast<ViewElement>(std::make_shared<Label>(this, "Hello world", 20, sf::Vector2f(0.f, 30.f))));
    this->addView(ObjectType::otModel, std::static_pointer_cast<ViewElement>(std::make_shared<Model>(this, sf::Vector2f(20.f, 20.f), "textures/terrain/castle-cobblestone")));

    std::shared_ptr<Model> model1 = std::make_shared<Model>(this, sf::Vector2f(100.f, 100.f));
    model1->loadShape(sf::Vector2f(50.f, 100.f), sf::Color(150, 150, 150, 100));
    this->addView(ObjectType::otModel, std::static_pointer_cast<ViewElement>(model1));

    std::shared_ptr<Model> model2 = std::make_shared<Model>(this, sf::Vector2f(200.f, 250.f));
    model2->loadShape(sf::Vector2f(50.f, 0.f), sf::Color(150, 255, 150, 100));
    this->addView(ObjectType::otModel, std::static_pointer_cast<ViewElement>(model2));

    std::shared_ptr<Button> button = std::make_shared<Button>(this, "Button1", sf::Vector2f(400.f, 300.f), "btn1");*/

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
    return false;
}

bool Manager::unloadAll()
{
    this->hud = nullptr;
    this->list.textures.clear();
    this->list.viewElements.clear();
	return true;
}