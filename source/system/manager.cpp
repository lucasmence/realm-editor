#include <fstream>
#include <sstream>
#include <type_traits>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "manager.hpp"
#include "external/imgui/imgui.h"
#include "external/imgui/imgui-SFML.h"

Manager::Manager()
{
    this->unloadAll();
    this->loadConstants();

    this->appName = "realm-editor";
    this->window = std::make_shared<sf::RenderWindow>(sf::VideoMode(sf::VideoMode::getDesktopMode().width, sf::VideoMode::getDesktopMode().height), appName, sf::Style::Default);
    this->canvas = std::make_shared<sf::View>();
    this->minimapView = std::make_shared<sf::View>();
    ImGui::SFML::Init(*this->window);

    this->font = std::shared_ptr<sf::Font>(new sf::Font);
    this->font->loadFromFile(this->constant.fontFilePath);
    this->icon.loadFromFile("realm-editor.png");
    this->window->setIcon(this->icon.getSize().x, this->icon.getSize().y, this->icon.getPixelsPtr());

    // Load splash logo texture from the icon image
    this->splashLogoTexture.loadFromImage(this->icon);
    this->splashLogoSprite.setTexture(this->splashLogoTexture);
    // Scale logo to a reasonable size for the splash
    float logoMaxDim = 100.f;
    sf::Vector2u logoSize = this->splashLogoTexture.getSize();
    float logoScale = logoMaxDim / std::max(logoSize.x, logoSize.y);
    this->splashLogoSprite.setScale(logoScale, logoScale);

    this->hudLoaded = false;

    this->hasFocus = true;
    this->open = false;
    this->minimapViewUpdate = false;
    this->minimapVisible = false;
    this->canvasPosition = sf::Vector2f(0.f, -115.f);
    this->filePathData.active = false;
    this->imguiMiscData.active = false;
    this->closeSignal = false;
    this->splashActive = false;

    if (!this->loadConfigTxt())
        this->choosePath(PathType::ptGamepath, "Select", "Select game folder", true, false);
}

Manager::~Manager()
{
    this->systemClose();
}

void Manager::systemClose()
{
    this->hudLoaded = false;
    this->unloadAll();
    this->window->close();
    ImGui::SFML::Shutdown();
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

    if (this->splashActive)
    {
        // Check if 5 seconds have elapsed
        if (this->splashClock.getElapsedTime().asSeconds() >= 2.5f)
            this->splashActive = false;
    }

    if (this->splashActive)
    {
        this->renderSplash();
    }
    else
    {
        if (this->hudLoaded)
        {
            this->setCanvas();

            this->hud->update(this->getMousePosition());
        }

        this->imguiUpdate();
    }

    this->display();

    if (this->closeSignal) 
        this->systemClose();

	return true;
}

bool Manager::imguiUpdate()
{
    ImGui::SFML::Update(*this->window, deltaClock.restart());
    
    if (this->hudLoaded)
        this->hud->imguiRender();
    
    if (this->imguiUpdateDialogBox())
        return true;

    if (this->updatePathImgui())
        if (this->imguiUpdatePath())
            return true;

    return false;
}

bool Manager::imguiUpdateDialogBox()
{
    if (!this->imguiMiscData.active) return false;

    switch (this->imguiMiscData.type)
    {
        case (ImguiMiscType::imtReloadConfirmation):
        {
            ImGui::SetNextWindowSize(ImVec2(1, 1), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(
                ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f),
                ImGuiCond_Always,
                ImVec2(0.5f, 0.5f)
            );
            ImGui::Begin("##placeholder");

            ImGui::OpenPopup("Reload Map");

            if (ImGui::BeginPopupModal("Reload Map", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Are you sure?");
                ImGui::Separator();

                if (ImGui::Button("Yes", ImVec2(120, 0)))
                {
                    this->imguiDialogBoxData.result = true;
                    this->imguiMiscData.active = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();

                if (ImGui::Button("No", ImVec2(120, 0)))
                {
                    this->imguiDialogBoxData.result = false;
                    this->imguiMiscData.active = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            ImGui::End();

            if (!this->imguiMiscData.active && this->imguiDialogBoxData.result) this->map->loadMap(this->map->filename);

            break;
        }

        case (ImguiMiscType::imtExitConfirmation):
        {
            ImGui::SetNextWindowSize(ImVec2(1, 1), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(
                ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f),
                ImGuiCond_Always,
                ImVec2(0.5f, 0.5f)
            );
            ImGui::Begin("##placeholder");

            ImGui::OpenPopup("Exit");

            if (ImGui::BeginPopupModal("Exit", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Are you sure you want to exit? \nAll your non-saved data will be lost!");
                ImGui::Separator();

                if (ImGui::Button("Yes", ImVec2(120, 0)))
                {
                    this->imguiDialogBoxData.result = true;
                    this->imguiMiscData.active = false;
                    this->map->deleteMapTemp();
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();

                if (ImGui::Button("No", ImVec2(120, 0)))
                {
                    this->imguiDialogBoxData.result = false;
                    this->imguiMiscData.active = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            ImGui::End();

            if (!this->imguiMiscData.active && this->imguiDialogBoxData.result) this->closeSignal = true;

            break;
        }
    }

    return true;
}

bool Manager::imguiUpdatePath()
{
    if (this->filePathData.path != "" && !this->filePathData.active)
    {
        if (this->hudLoaded) 
            this->map->filename = this->filePathData.path;

        boost::filesystem::path mapFolder(this->filePathData.currentEntry.path);
        this->constant.mapFolder = mapFolder.parent_path().string();
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
                this->constant.gamePath = this->filePathData.currentEntry.path;
                this->loadGamepathAfter();
                break;
            }
        }

        this->saveConfigTxt();
    }

    return true;
}

bool Manager::loadGamepathAfter()
{
    this->hud = std::make_shared<Hud>(this);
    this->palette = std::make_shared<Palette>(this);
    this->map = std::make_shared<Map>(this);
    this->minimapView->setViewport(this->constant.minimapSize);
    this->minimapViewArea = sf::FloatRect(0.f, 0.f, 0.f, 0.f);

    this->map->newMap();

    // Check if a recovery temp map exists BEFORE loading it
    boost::filesystem::path tempPath = boost::filesystem::path(this->constant.gamePath) / "temp" / "temp.json";
    bool tempMapExists = boost::filesystem::exists(tempPath);

    this->map->loadMapTemp();

    this->hudLoaded = true;

    // Show splash only if no recovery map is being loaded
    this->splashActive = !tempMapExists;
    if (this->splashActive)
        this->splashClock.restart();

    return true;
}

bool Manager::display()
{
    this->minimapViewUpdate = false;

    if (!this->splashActive && !this->filePathData.active && this->hudLoaded)
    {
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
    } 
    
    if (!this->splashActive)
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
        if (this->splashActive)
        {
            // During splash, close the window directly without confirmation
            if (event.type == sf::Event::Closed)
            {
                this->closeSignal = true;
                return true;
            }
            continue;
        }

        ImGui::SFML::ProcessEvent(event);
        if (this->hudLoaded)
            switch (event.type)
            {
                case sf::Event::Closed:
                {
                    this->imguiTrigger(ImguiMiscData{ true, ImguiMiscType::imtExitConfirmation });
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
    // Only block clicks on interactive ImGui widgets (buttons, inputs, etc.),
    // not on transparent window backgrounds (the tool panel covers the map area)
    if (ImGui::IsAnyItemHovered())
        return true;

    sf::Vector2f cursor = this->getMousePosition();
    
    this->hud->updateClick(cursor, sf::Mouse::isButtonPressed(sf::Mouse::Right));

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
            this->hud->manager->palette->clearPaletteItem();
            break;
        }
        case (sf::Keyboard::E):
        {
            this->hud->manager->palette->erasePaletteItem();
            break;
        }
        case (sf::Keyboard::G):
        {
            this->hud->toggleGridVisibility();
            break;
        }
        case (sf::Keyboard::P):
        {
            this->hud->spawnPress = !this->hud->spawnPress;
            break;
        }
        case (sf::Keyboard::S):
        {
            this->hud->centerShape = !this->hud->centerShape;
            break;
        }
        case (sf::Keyboard::M):
        {
            this->hud->matrixActivated = !this->hud->matrixActivated;
            break;
        }
        case (sf::Keyboard::W):
        {
            this->hud->wallActivated = !this->hud->wallActivated;
            break;
        }
        case (sf::Keyboard::A):
        {
            this->hud->shapeMapArea->visible = !this->hud->shapeMapArea->visible;
            break;
        }
        case (sf::Keyboard::D):
        {
            this->hud->gridSpawn = !this->hud->gridSpawn;
            break;
        }
        case (sf::Keyboard::I):
        {
            this->hud->manager->palette->clearPaletteItem();
            this->hud->itemSelect = true;
            break;
        }
        case (sf::Keyboard::B):
        {
            this->hud->updateMapBounds();
            break;
        }
        case (sf::Keyboard::Tilde):
        {
            this->hud->dragCursor = !this->hud->dragCursor;
            if (this->hud->dragCursor)
                this->hud->manager->palette->clearPaletteItem();
            break;
        }
        case (sf::Keyboard::Period):
        {
            if (this->hud->itemSelected)
                this->hud->itemSelectedMove = !this->hud->itemSelectedMove;
            else
                this->hud->itemSelectedMove = false;
            break;
        }
        case (sf::Keyboard::Q):
        {
            this->hud->formShapeClick("square");
            break;
        }
        case (sf::Keyboard::R):
        {
            this->hud->formShapeClick("circle");
            break;
        }
        case (sf::Keyboard::O):
        {
            this->hud->formShapeClick("none");
            break;
        }
        case (sf::Keyboard::L):
        {
            this->calculateMapEdges();
            break;
        }
        case (sf::Keyboard::Z):
        {
            if (event.key.control)
                this->hud->undoAction();
            break;
        }
        case (sf::Keyboard::Y):
        {
            if (event.key.control)
                this->hud->redoAction();
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

std::shared_ptr<Texture> Manager::getTexture(std::string filename, std::string jsonPath)
{
    for (auto& texture : this->list.textures)
        if (texture->filename == filename)
            return texture; 

    std::shared_ptr<Texture> texture = std::make_shared<Texture>(filename, jsonPath);
    this->list.textures.emplace_back(texture);
    return texture;
}

bool Manager::loadConstants()
{

    json file = Json::loadFromFile("data/options/realm-editor.json");
    this->constant.fontFilePath = file.value("font-file-path", "");
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
    if (this->hudLoaded) this->palette->clearPaletteItem();
    this->filePathData = FilePathData{ type, confirmButtonName, dialogCaption, "", "", FileEntry{"", "", false}, getFolder, true, this->returnFiles(this->constant.mapFolder != "" ? this->constant.mapFolder : boost::filesystem::current_path().string()), cancelButtonVisible, false};
    return true;
}

bool Manager::updatePathImgui()
{
    if (!this->filePathData.active)
        return false;

    std::list<FileEntry> fileList = this->filePathData.filePath;

    ImGui::SetNextWindowSize(ImVec2(530, 275), ImGuiCond_FirstUseEver);
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
                this->filePathData.currentEntry = (this->filePathData.isFolder && entry.isFolder) || (!this->filePathData.isFolder && !entry.isFolder) ? entry : FileEntry{"", "", false};
                if (ImGui::IsMouseDoubleClicked(0)) 
                {
                    if (entry.isFolder) {
                        this->filePathData.filePath = returnFiles(entry.path);
                        this->filePathData.currentEntry = { "", "", false };
                        if (this->filePathData.type == PathType::ptSaveMap)
                        {
                            this->filePathData.path = entry.path;
                            this->filePathData.currentEntry = entry;
                        }           
                    }
                    else 
                    {
                        this->filePathData.path = entry.path;
                        this->filePathData.active = false;
                        if (this->filePathData.type == PathType::ptSaveMap)
                        {
                            this->filePathData.path = entry.path;
                            this->filePathData.currentEntry = entry;
                        }
                    }
                }
            }

            if (entry.isFolder) ImGui::PopStyleColor();

        }
        ImGui::EndChild();

        if (this->filePathData.type == PathType::ptSaveMap)
        {
            static char buffer[256] = "";
            if (ImGui::InputText("##edit", buffer, IM_ARRAYSIZE(buffer)))
            {
                this->filePathData.file = buffer;
            }
        }
        ImVec2 size = ImGui::GetWindowSize();
        float btnWidth = 100.0f;
        float btnHeight = 25.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.y;

        float posX = size.x - btnWidth - 10.0f;
        float posY = size.y - (btnHeight * 2) - spacing - 10.0f;

        ImGui::SetCursorPos(ImVec2(posX, posY));

        if (ImGui::Button(this->filePathData.confirmButtonName.data(), ImVec2(btnWidth, btnHeight)))
        { 
            this->filePathData.path = this->filePathData.currentEntry.path;
            if (this->filePathData.path != "")
            {
                if (this->filePathData.type == PathType::ptSaveMap)
                {
                    this->filePathData.overwriteDialog = (boost::filesystem::exists(boost::filesystem::path{ this->filePathData.path + "\\" + this->filePathData.file + ".json" }));
                }
                if (this->filePathData.overwriteDialog) 
                {
                    ImGui::OpenPopup("Overwrite");
                }
                else
                {     
                    this->filePathData.active = false;
                }   
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
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
        ImGui::Text(this->filePathData.currentEntry.name.c_str());
        ImGui::PopStyleColor();
    }

    if (ImGui::BeginPopupModal("Overwrite", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Are you sure?");
        ImGui::Separator();

        if (ImGui::Button("Yes", ImVec2(120, 0)))
        {
            this->filePathData.active = false;
            this->filePathData.overwriteDialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("No", ImVec2(120, 0)))
        {
            this->filePathData.path = "";
            this->filePathData.overwriteDialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    
    ImGui::End();

    return true;
}

bool Manager::loadConfigTxt()
{
    std::fstream file("config.txt", std::ios::in | std::ios::out | std::ios::app);

    if (!file.is_open()) return false;

    std::string localGamepath, localMapFolder;

    std::getline(file, localGamepath);
    std::getline(file, localMapFolder);

    this->constant.gamePath = localGamepath;
    this->constant.mapFolder = localMapFolder;

    if (this->constant.gamePath != "")
        this->loadGamepathAfter();

    return this->hudLoaded;
}

bool Manager::saveConfigTxt()
{
    if (!this->hudLoaded) return false;

    std::fstream file("config.txt", std::ios::in | std::ios::out | std::ios::trunc);

    file.clear();
    file << this->constant.gamePath;
    file << "\n" << this->constant.mapFolder;

    file.close();
    return true;
}


std::list<std::string> Manager::loadFileLists(std::string directory, std::string subDirectory)
{
    return this->loadFileFromDirectory(directory, "", subDirectory);
}

std::list<std::string> Manager::loadFileFromDirectory(std::string directory, std::string base, std::string subDirectory)
{
    boost::filesystem::path path = directory;
    if (base == "")
        path = this->constant.gamePath + "/data/" + directory;
    else
        base += "\\";

    std::list<std::string> listFiles = {};

    for (auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(path), {}))
        if (boost::filesystem::is_directory(entry))
        {
            std::ostringstream stringStreamDirectory, stringStreamName;
            stringStreamDirectory << entry;
            stringStreamName << entry.path().filename();
            std::string directory = this->getString(stringStreamDirectory.str()), name = this->getString(stringStreamName.str());
            boost::algorithm::replace_all(directory, "\\", "/");
            std::list<std::string> subListFiles = this->loadFileFromDirectory(directory, name);
            if (subListFiles.size() > 0)
                listFiles.insert(listFiles.end(), subListFiles.begin(), subListFiles.end());
        }
        else if (entry.path().extension().string() == ".json")
            listFiles.emplace_back(subDirectory + base + entry.path().stem().string());

    return listFiles;
}

std::string Manager::getString(std::string value)
{
    boost::erase_all(value, "\"");
    return value;
}

bool Manager::imguiTrigger(ImguiMiscData data)
{
    this->imguiMiscData = data;

    switch (this->imguiMiscData.type)
    {
        case (ImguiMiscType::imtReloadConfirmation, ImguiMiscType::imtExitConfirmation):
        {
            this->imguiDialogBoxData.result = false;
            break;
        }
    }

    return true;
}

void Manager::renderSplash()
{
    // Use the default window view (not the canvas) for splash rendering
    this->window->setView(this->window->getDefaultView());

    sf::Vector2u winSize = this->window->getSize();
    float w = static_cast<float>(winSize.x);
    float h = static_cast<float>(winSize.y);

    // Dark gradient-like background
    sf::RectangleShape background(sf::Vector2f(w, h));
    background.setFillColor(sf::Color(18, 18, 28));
    this->window->draw(background);

    // Decorative accent line at the top
    sf::RectangleShape accentLine(sf::Vector2f(w, 3.f));
    accentLine.setFillColor(sf::Color(70, 130, 200));
    this->window->draw(accentLine);

    // Centered card/panel
    float cardWidth = 420.f;
    float cardHeight = 320.f;
    sf::RectangleShape card(sf::Vector2f(cardWidth, cardHeight));
    card.setPosition(w / 2.f - cardWidth / 2.f, h / 2.f - cardHeight / 2.f);
    card.setFillColor(sf::Color(24, 24, 36));
    card.setOutlineThickness(1.f);
    card.setOutlineColor(sf::Color(50, 50, 70));
    this->window->draw(card);

    // Logo / Icon
    sf::Vector2u logoSize = this->splashLogoTexture.getSize();
    if (logoSize.x > 0 && logoSize.y > 0)
    {
        this->splashLogoSprite.setPosition(
            w / 2.f - this->splashLogoSprite.getGlobalBounds().width / 2.f,
            h / 2.f - 120.f);
        this->window->draw(this->splashLogoSprite);
    }

    // Title text
    sf::Text titleText;
    titleText.setFont(*this->font);
    titleText.setString("realm-editor");
    titleText.setCharacterSize(32);
    titleText.setStyle(sf::Text::Bold);
    titleText.setFillColor(sf::Color(220, 220, 245));
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setOrigin(titleBounds.left + titleBounds.width / 2.f, titleBounds.top + titleBounds.height / 2.f);
    titleText.setPosition(w / 2.f, h / 2.f - 25.f);
    this->window->draw(titleText);

    // Version text
    sf::Text versionText;
    versionText.setFont(*this->font);
    versionText.setString("v1.0.0");
    versionText.setCharacterSize(16);
    versionText.setFillColor(sf::Color(130, 130, 170));
    sf::FloatRect verBounds = versionText.getLocalBounds();
    versionText.setOrigin(verBounds.left + verBounds.width / 2.f, verBounds.top + verBounds.height / 2.f);
    versionText.setPosition(w / 2.f, h / 2.f + 10.f);
    this->window->draw(versionText);

    // Divider line
    sf::RectangleShape divider(sf::Vector2f(200.f, 1.f));
    divider.setFillColor(sf::Color(60, 60, 85));
    divider.setPosition(w / 2.f - 100.f, h / 2.f + 40.f);
    this->window->draw(divider);

    // Development credit
    sf::Text creditText;
    creditText.setFont(*this->font);
    creditText.setString("Developed by Mence");
    creditText.setCharacterSize(14);
    creditText.setFillColor(sf::Color(110, 110, 150));
    sf::FloatRect creditBounds = creditText.getLocalBounds();
    creditText.setOrigin(creditBounds.left + creditBounds.width / 2.f, creditBounds.top + creditBounds.height / 2.f);
    creditText.setPosition(w / 2.f, h / 2.f + 65.f);
    this->window->draw(creditText);

    // Loading / progress hint
    float elapsed = this->splashClock.getElapsedTime().asSeconds();
    float progress = std::min(elapsed / 2.5f, 1.f);

    sf::Text loadingText;
    loadingText.setFont(*this->font);
    loadingText.setString("Loading...");
    loadingText.setCharacterSize(12);
    loadingText.setFillColor(sf::Color(90, 90, 130));
    sf::FloatRect loadBounds = loadingText.getLocalBounds();
    loadingText.setOrigin(loadBounds.left + loadBounds.width / 2.f, loadBounds.top + loadBounds.height / 2.f);
    loadingText.setPosition(w / 2.f, h / 2.f + 105.f);
    this->window->draw(loadingText);

    // Progress bar background
    sf::RectangleShape progressBg(sf::Vector2f(200.f, 4.f));
    progressBg.setFillColor(sf::Color(40, 40, 55));
    progressBg.setPosition(w / 2.f - 100.f, h / 2.f + 120.f);
    this->window->draw(progressBg);

    // Progress bar fill
    sf::RectangleShape progressFill(sf::Vector2f(200.f * progress, 4.f));
    progressFill.setFillColor(sf::Color(70, 130, 200));
    progressFill.setPosition(w / 2.f - 100.f, h / 2.f + 120.f);
    this->window->draw(progressFill);
}