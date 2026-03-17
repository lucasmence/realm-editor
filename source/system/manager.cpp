#include "manager.hpp"
#include "library/script.hpp"
#include "external/imgui/imgui.h"
#include "external/imgui/imgui-SFML.h"

Manager::Manager()
{
    this->unloadAll();
    this->loadConstants();

    this->appName = "realm-editor";
    this->window = std::make_shared<sf::RenderWindow>(sf::VideoMode(sf::VideoMode::getDesktopMode().width, sf::VideoMode::getDesktopMode().height), appName, sf::Style::Default);
    ImGui::SFML::Init(*this->window);

    this->font = std::shared_ptr<sf::Font>(new sf::Font);
    this->font->loadFromFile(this->constant.gamePath + "/" + this->constant.fontFilePath);
    this->icon.loadFromFile("realm-editor.png");
    this->window->setIcon(this->icon.getSize().x, this->icon.getSize().y, this->icon.getPixelsPtr());
  
    this->hud = std::make_shared<Hud>(this);
    this->palette = std::make_shared<Palette>(this);
    this->map = std::make_shared<Map>(this);
    this->canvas = std::make_shared<sf::View>();
    this->minimapView = std::make_shared<sf::View>();
    this->minimapView->setViewport(this->constant.minimapSize);
    this->minimapViewArea = sf::FloatRect(0.f, 0.f, 0.f, 0.f);   

    this->hasFocus = true;
    this->open = false;
    this->minimapViewUpdate = false;
    this->minimapVisible = false;
    this->canvasPosition = sf::Vector2f(0.f, -115.f);
    this->filePathData.active = false;

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

    for (auto& elementIndex : this->list.viewElements)
    {
        if (elementIndex->priority < element->priority)
        {
            this->list.viewElements.emplace(std::find(this->list.viewElements.begin(), this->list.viewElements.end(), elementIndex), element);
            return true;         
        }

        if (elementIndex->priority == element->priority)
        {
            if (elementIndex->autoPriority > element->autoPriority)
            {
                this->list.viewElements.emplace(std::find(this->list.viewElements.begin(), this->list.viewElements.end(), elementIndex), element);
                return true;
            }
        }
    }

    this->list.viewElements.emplace_back(element);
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

    this->imguiUpdate();

    this->display();

	return true;
}

bool Manager::imguiUpdate()
{
    ImGui::SFML::Update(*this->window, deltaClock.restart());
    if (this->updatePathImgui())
    {
        if (this->filePathData.path != "" && !this->filePathData.active)
        {
            this->map->filename = this->filePathData.path;
            std::cout << this->map->filename << std::endl;
            this->filePathData.path = "";
            switch (this->filePathData.type)
            {
            case (PathType::ptLoadMap):
            {
                this->map->loadMapAfter();
                break;
            }
            case (PathType::ptSaveMap):
            {
                this->map->saveMapAfter();
                break;
            }
            case (PathType::ptGamepath):
            {
                this->loadGamepathAfter();
                break;
            }
            }
        }

        return true;
    }

    return false;
}

bool Manager::loadGamepathAfter()
{

    return true;
}

bool Manager::display()
{
    this->minimapViewUpdate = false;

    for (auto& element : this->list.viewElements)
        element->draw();

    if (this->minimapVisible)
    {
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
    }
    else 
        this->minimapViewArea = sf::FloatRect(0.f, 0.f, 0.f, 0.f);
    
    ImGui::SFML::Render(*this->window);
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
        ImGui::SFML::ProcessEvent(event);
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
    if (this->hud->getCheckEditing())
        return false;

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
        case (sf::Keyboard::C):
        {
            this->hud->buttonsClick(this->hud->getButton("btnClear")->shape->getPosition());
            break;
        }
        case (sf::Keyboard::E):
        {
            this->hud->buttonsClick(this->hud->getButton("btnErase")->shape->getPosition());
            break;
        }
        case (sf::Keyboard::G):
        {
            this->hud->buttonsClick(this->hud->getButton("btnGridVisibilityToggle")->shape->getPosition());
            break;
        }
        case (sf::Keyboard::P):
        {
            this->hud->buttonsClick(this->hud->getButton("btnSpawnPress")->shape->getPosition());
            break;
        }
        case (sf::Keyboard::S):
        {
            this->hud->buttonsClick(this->hud->getButton("btnCenterShape")->shape->getPosition());
            break;
        }
        case (sf::Keyboard::M):
        {
            this->hud->buttonsClick(this->hud->getButton("btnMatrix")->shape->getPosition());
            break;
        }
        case (sf::Keyboard::W):
        {
            this->hud->buttonsClick(this->hud->getButton("btnWall")->shape->getPosition());
            break;
        }
        case (sf::Keyboard::A):
        {
            this->hud->buttonsClick(this->hud->getButton("btnMapAreaSize")->shape->getPosition());
            break;
        }
        case (sf::Keyboard::D):
        {
            this->hud->buttonsClick(this->hud->getButton("btnGridSpawn")->shape->getPosition());
            break;
        }
        case (sf::Keyboard::I):
        {
            this->hud->buttonsClick(this->hud->getButton("btnSelectItem")->shape->getPosition());
            break;
        }
        case (sf::Keyboard::B):
        {
            this->hud->buttonsClick(this->hud->getButton("btnUpdateMapBounds")->shape->getPosition());
            break;
        }
        case (sf::Keyboard::Tilde):
        {
            this->hud->buttonsClick(this->hud->getButton("btnDragCursor")->shape->getPosition());
            break;
        }
        case (sf::Keyboard::Period):
        {
            this->hud->buttonsClick(this->hud->getButton("btnSelectItemMove")->shape->getPosition());
            break;
        }
        case (sf::Keyboard::Q):
        {
            this->hud->buttonsClick(this->hud->getButton("btnFormShapeSquare")->shape->getPosition());
            break;
        }
        case (sf::Keyboard::R):
        {
            this->hud->buttonsClick(this->hud->getButton("btnFormShapeCircle")->shape->getPosition());
            break;
        }
        case (sf::Keyboard::O):
        {
            this->hud->buttonsClick(this->hud->getButton("btnFormShapeNone")->shape->getPosition());
            break;
        }
        case (sf::Keyboard::L):
        {
            this->calculateMapEdges();
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

bool Manager::calculateMapEdges()
{
    this->mapEdges.clear();

    sf::IntRect pixel = {0, 0, 1, 1};

    for (int x = 0; x < this->map->data.size.x; x++)
        for (int y = 0; y < this->map->data.size.y; y++)
        {
            pixel.left = x;
            pixel.top = y;


        }

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
    this->constant.gamePath = file.value("game-path", "");
    this->constant.gameVersion = file.value("game-version", "");

    this->constant.gridSize.clear();
    this->constant.brushSize.clear();

    for (int index = 0; index < file["grid-size"].size(); index++)
        this->constant.gridSize.emplace_back(file["grid-size"][index]);

    for (int index = 0; index < file["brush-size"].size(); index++)
        this->constant.brushSize.emplace_back(file["brush-size"][index]);

    this->constant.minimapSize = sf::FloatRect(file["minimap-size"][0], file["minimap-size"][1], file["minimap-size"][2], file["minimap-size"][3]);

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

std::list<FileEntry> Manager::returnFiles(std::string pathname)
{
    boost::filesystem::path path = pathname;
    boost::filesystem::directory_iterator iterator{ path };
    boost::filesystem::path parentPath(pathname);

    std::list<FileEntry> fileListResult = { FileEntry {"...", parentPath.parent_path().string(), true} };
    while (iterator != boost::filesystem::directory_iterator{})
    {
        boost::filesystem::path p = iterator->path();
        if (!(p.filename().string() == "trigger" && boost::filesystem::is_directory(p)))
            fileListResult.emplace_back(FileEntry{ p.filename().string(), boost::filesystem::absolute(p).string(), boost::filesystem::is_directory(p) });
        iterator++;
    }

    int index = 0;

    std::list<FileEntry> fileList = {};
    std::list<FileEntry> fileListTemp = fileListResult;
    while (fileListTemp.size() > 0) 
    {
        FileEntry entry = fileListTemp.back();
        if (entry.isFolder) fileList.emplace(fileList.begin(), entry); else fileList.emplace_back(entry);
        fileListTemp.pop_back();
    }

    return fileList;
}

bool Manager::choosePath(PathType type, std::string confirmButtonName, std::string dialogCaption, bool getFolder, bool cancelButtonVisible)
{
    this->palette->clearPaletteItem();
    this->filePathData = FilePathData{ type, confirmButtonName, dialogCaption, "", FileEntry{"", "", false}, getFolder, true, this->returnFiles(this->constant.gamePath), cancelButtonVisible };
    return true;
}

bool Manager::updatePathImgui()
{
    if (!this->filePathData.active)
        return false;

    std::list<FileEntry> fileList = this->filePathData.filePath;

    ImGui::SetNextWindowSize(ImVec2(530, 235), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(
        ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.5f)
    );
    ImGui::Begin(this->filePathData.dialogCaption.data());
    if (ImGui::BeginChild("##file_browser", ImVec2(400, 200), true))
    {
        for (const auto& entry : fileList)
        {
            std::string label = entry.isFolder ? "[ " + entry.name + " ]" : entry.name;
            if (entry.isFolder) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));

            if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) 
            {
                this->filePathData.currentEntry = this->filePathData.isFolder && entry.isFolder ? entry : FileEntry{ "", "", false }; 
                if (ImGui::IsMouseDoubleClicked(0)) 
                {
                    if (entry.isFolder) {
                        this->filePathData.filePath = returnFiles(entry.path);
                        this->filePathData.currentEntry = { "", "", false };
                    }
                    else 
                    {
                        this->filePathData.path = entry.path;
                        this->filePathData.active = false;
                    }
                }
            }

            if (entry.isFolder) ImGui::PopStyleColor();

        }
        ImGui::EndChild();
        ImGui::Dummy(ImVec2(200.0f, 0.0f));
        ImGui::Separator();
        ImVec2 size = ImGui::GetWindowSize();
        float btnWidth = 100.0f;
        float btnHeight = 25.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.y;

        float posX = size.x - btnWidth - 10.0f;
        float posY = size.y - (btnHeight * 2) - spacing - 10.0f;

        ImGui::SetCursorPos(ImVec2(posX, posY));
        if (ImGui::Button(this->filePathData.confirmButtonName.data(), ImVec2(btnWidth, btnHeight)))
        { 
            if (this->filePathData.currentEntry.path != "")
            {
                this->filePathData.path = this->filePathData.currentEntry.path;
                this->filePathData.active = false;
            }       
        }

        if (this->filePathData.cancelButtonVisible)
        {
            ImGui::SetCursorPos(ImVec2(posX, posY + btnHeight + spacing));
            if (ImGui::Button("Cancel", ImVec2(btnWidth, btnHeight)))
            {
                this->filePathData.active = false;
            }
        }
        
        float windowHeight = ImGui::GetWindowSize().y;
        float textHeight = ImGui::GetTextLineHeightWithSpacing();
        float margin = 10.0f;

        ImGui::SetCursorPos(ImVec2(margin, windowHeight - textHeight - margin));
        ImGui::Text(this->filePathData.currentEntry.name.data());
    }
    
    ImGui::End();

    return true;
}