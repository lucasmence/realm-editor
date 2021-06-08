#include "manager.hpp"
#include "library/script.hpp"

Manager::Manager()
{
    this->unloadAll();
    this->loadConstants();

    this->appName = "realm-editor";
	this->window = std::make_shared<sf::RenderWindow>(sf::VideoMode(1920, 1080), appName, sf::Style::Default);

    this->font = std::shared_ptr<sf::Font>(new sf::Font);
    this->font->loadFromFile(this->constant.fontFilePath);
    this->icon.loadFromFile("realm-editor.png");
    this->window->setIcon(this->icon.getSize().x, this->icon.getSize().y, this->icon.getPixelsPtr());

    this->hud = std::make_shared<Hud>(this);
    this->palette = std::make_shared<Palette>(this);
    this->map = std::make_shared<Map>(this);
    this->canvas = std::make_shared<sf::View>();
    this->minimapView = std::make_shared<sf::View>();
    this->minimapView->setViewport(sf::FloatRect(0.742f, 0.90f, 0.10f, 0.10f));
    this->minimapViewArea = sf::FloatRect(0.f, 0.f, 0.f, 0.f);   

    this->hasFocus = true;
    this->open = false;
    this->minimapViewUpdate = false;
    this->canvasPosition = sf::Vector2f(0.f, -115.f);

    this->loadWindowOpening();
    this->map->newMap();
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
    script::maximizeWindow("realm-editor");
    
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
    element->initialization();
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

    this->setCanvas();
    
    this->hud->update(this->getMousePosition());

    this->display();

	return true;
}

bool Manager::display()
{
    this->minimapViewUpdate = false;

    for (auto& element : this->list.viewElements)
        element->draw();

    this->minimapView->reset(sf::FloatRect(this->canvasPosition.x, this->canvasPosition.y, this->window->getSize().x, this->window->getSize().y));
    this->minimapView->zoom(2.f);
    this->window->setView(*this->minimapView);
    this->minimapViewUpdate = true;
    for (auto& element : this->list.viewElements)
        element->draw();

    this->minimapViewUpdate = false;

    this->minimapViewArea = sf::FloatRect(this->canvasPosition.x + this->canvas->getSize().x * this->minimapView->getViewport().left,
                                          this->canvasPosition.y + this->canvas->getSize().y * this->minimapView->getViewport().top,
                                          this->minimapView->getSize().x * this->minimapView->getViewport().width,
                                          this->minimapView->getSize().y * this->minimapView->getViewport().height);
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
    this->window->setView(*this->canvas);
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

            case sf::Event::KeyPressed:
            {
                this->eventKey(event);
                break;
            }

            case sf::Event::TextEntered:
            {
                this->eventType(event);
                break;
            }

            case sf::Event::MouseButtonReleased:
            {
                this->eventMouseReleased(event);
                break;
            }

            case sf::Event::MouseMoved:
            {
                this->eventMouseMoved(event);
                break;
            }
        }
    }

	return true;
}

bool Manager::eventClick(sf::Event& event)
{
    sf::Vector2f cursor = this->getMousePosition();
    
    this->hud->updateClick(cursor, sf::Mouse::isButtonPressed(sf::Mouse::Right));
    this->palette->selectPaletteItem(cursor);

    return true;
}

bool Manager::eventKey(sf::Event& event)
{
    switch (event.key.code)
    {
        case (sf::Keyboard::Left):
        {
            this->moveCanvas(sf::Vector2f(this->canvasPosition.x - 64.f, this->canvasPosition.y));
            break;
        }
        case (sf::Keyboard::Right):
        {
            this->moveCanvas(sf::Vector2f(this->canvasPosition.x + 64.f, this->canvasPosition.y));
            break;
        }
        case (sf::Keyboard::Up):
        {
            this->moveCanvas(sf::Vector2f(this->canvasPosition.x, this->canvasPosition.y - 64.f));
            break;
        }
        case (sf::Keyboard::Down):
        {
            this->moveCanvas(sf::Vector2f(this->canvasPosition.x, this->canvasPosition.y + 64.f));
            break;
        }
        case (sf::Keyboard::Add):
        {
            this->hud->zoomMap(-1);
            break;
        }
        case (sf::Keyboard::Subtract):
        {
            this->hud->zoomMap(1);
            break;
        }
        case (sf::Keyboard::Space):
        {
            this->resetView();
            break;
        }
        case (sf::Keyboard::Delete):
        {
            this->hud->deleteSelectedItem();
            break;
        }
    }

    return true;
}

bool Manager::eventMouseReleased(sf::Event& event)
{
    sf::Vector2f cursor = this->getMousePosition();
    this->hud->updateMouseReleased(cursor);
    return true;
}

bool Manager::eventMouseMoved(sf::Event& event)
{
    return true;
}

bool Manager::eventType(sf::Event& event)
{
    this->hud->updateEdit(static_cast<char>(event.text.unicode));
    return true;
}

bool Manager::resetView()
{
    if (this->hud->zoom != 1.f)
        return this->hud->zoomMapReset();
    
    this->canvasPosition = sf::Vector2f(0.f, -115.f);
    this->moveCanvas(this->canvasPosition);
    return true;
}

bool Manager::moveCanvas(sf::Vector2f position)
{
    this->canvas->move(position.x - this->canvasPosition.x, position.y - this->canvasPosition.y);
    this->canvasPosition.x = position.x;
    this->canvasPosition.y = position.y;
    this->window->setView(*this->canvas);
    return true;
}

bool Manager::setCanvasCenter(sf::Vector2f position)
{
    this->canvas->move(position.x, position.y);
    this->canvasPosition.x = position.x + this->canvasPosition.x;
    this->canvasPosition.y = position.y + this->canvasPosition.y;
    this->window->setView(*this->canvas);

    return true;
}

bool Manager::setCanvas()
{
    this->canvas->reset(sf::FloatRect(this->canvasPosition.x, this->canvasPosition.y, this->window->getSize().x, this->window->getSize().y));
    this->canvas->zoom(this->hud->zoom);
    this->window->setView(*this->canvas);
    return true;
}

std::string Manager::setTitle(std::string value)
{
    std::string title = this->appName + " - " + value;
    this->window->setTitle(title);
    return title;
}

std::shared_ptr<Texture> Manager::getTexture(std::string filename)
{
    for (auto& texture : this->list.textures)
        if (texture->filename == filename)
            return texture; 

    std::shared_ptr<Texture> texture = std::make_shared<Texture>(filename);
    this->list.textures.emplace_back(texture);
    return texture;
}

bool Manager::loadConstants()
{

    json file = Json::loadFromFile("data/options/realm-editor.json");
    this->constant.fontFilePath = file.value("font-file-path", "");

    this->constant.gridSize.clear();
    this->constant.brushSize.clear();

    for (int index = 0; index < file["grid-size"].size(); index++)
        this->constant.gridSize.emplace_back(file["grid-size"][index]);

    for (int index = 0; index < file["brush-size"].size(); index++)
        this->constant.brushSize.emplace_back(file["brush-size"][index]);

    file.clear();

    return true;
}

bool Manager::unloadAll()
{
    this->hud = nullptr;
    this->map = nullptr;
    this->list.textures.clear();
    this->list.viewElements.clear();
	return true;
}