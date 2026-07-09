#include <SFML/Graphics.hpp>
#include <memory>
#include <list>
#include <vector>
#include <string>
#include "../palette/palette.hpp"
#include "../map/map.hpp"

#pragma once

#ifndef HUD_HPP
#define HUD_HPP

class Manager;

struct GameTick
{
	int tickValue;
	int tickMax;
};

enum class EditType {etString, etInteger, etBoolean};

struct ImguiEditValue
{
	std::string string;
	int integer;
	bool boolean;
	bool active;
};

enum class HistoryActionType {hatSpawn, hatDelete};

struct HistoryEntry
{
	HistoryActionType type;
	std::list<MapObjectUnit> objects;
	std::string description;
};

class Hud 
{
	public:
		Manager* manager;

		sf::Vector2f hoverShapeSize;
		sf::Vector2f mousePressPosition;
		int gridSize;
		int brushSize;
		int rotation;
		int priority;
		int formShapeSize;
		float scale;
		float zoom;
		bool mousePressed;
		bool spawnPress;
		bool centerShape;
		bool mouseRightButton;
		bool matrixActivated;
		bool matrixTriggered;
		bool matrixPosSpawn;
		bool wallActivated;
		bool gridSpawn;
		bool itemSelect;
		bool itemSelected;
		bool itemSelectedMove;
		bool dragCursor;
		bool gridVisible;
		bool removeBgVisible;
		std::string weatherName;
		int weatherChance;
		std::string particles;
		std::string mapName;
		std::string mapMusic;
		std::string mapVersion;
		std::string bgTexture;
		GameTick mapTempTick;
		std::vector<int> gridSizeList;
		std::vector<int> brushSizeList;

		std::list<std::shared_ptr<Model>> models;
		std::list<std::shared_ptr<Model>> grid;
		std::shared_ptr<Model> shapeHover;
		std::shared_ptr<Model> shapeMapArea;
		std::shared_ptr<Model> shapeMatrix;
		std::shared_ptr<Model> shapeItemSelected;
		std::shared_ptr<Model> itemModelSelected;
		std::shared_ptr<Model> shapeMinimap;

		bool unloadLists();
		bool loadLists();
		bool loadModels();
		bool loadGrid();
		bool update(sf::Vector2f cursor);
		bool updateClick(sf::Vector2f cursor, bool rightButton);
		bool updateMouseReleased(sf::Vector2f cursor);
		bool updateMousePressed(sf::Vector2f cursor);
		bool updateItemSelectedMove(sf::Vector2f cursor);
		bool updateCursor(sf::Vector2f cursor);
		bool updateMapTemp();
		bool spawnClick(sf::Vector2f cursor);
		std::list<MapObjectField> getExtraEditValuesByType();
		bool matrixActivate(sf::Vector2f cursor);
		bool matrixDeactivate(sf::Vector2f cursor);
		bool matrixGenerate(sf::Vector2f cursor);
		bool showMessage(std::string text, float time = 3.f);
		bool changeGridSize(int order);
		bool changeBrushSize(int order);
		bool updateHoverShapeSize();
		bool updateHoverMapSize();
		bool updateHoverGeneral();
		bool updateShapeMatrix(sf::Vector2f cursor);
		bool updateMapBounds();
		bool toggleGridVisibility();
		bool removeBackground();
		bool formShapeClick(const std::string& shapeName);
		bool selectItem(sf::Vector2f cursor);
		bool deleteSelectedItem();
		bool selectedItemUpdate();
		bool checkMapClick(sf::Vector2f cursor);
		bool updateDragCursor(sf::Vector2f cursor);
		bool zoomMap(int value);
		bool zoomMapReset();
		bool help();
		bool getCheckEditing();
		std::shared_ptr<Model> spawnItem(sf::Vector2f tilesetPosition, std::string paletteTypeField, std::string texture, int priorityValue,
										 sf::Vector2f tilesetOrigin, int x, int y, MapObjectType objectType, std::list<MapObjectField> fields);

		bool getPaletteType(PaletteType &paletteType, MapObjectType type);
		bool updateExtraEditsValue(std::vector<std::string> caption, std::vector<EditType> type, std::vector<std::string> value, std::vector<int> maxValue, std::vector<std::string> origin);
		bool setExtraEditsValue(std::vector<std::string> value);
		bool setExtraEditValue(std::string value, int index);
		bool resetExtraEditsValue();
		std::vector<ImguiEditValue> getExtraEditsValue();
		bool setEditValue(std::string editName, std::string value);
		Hud(Manager* manager);
		~Hud();

		bool updateBitmask(MapObjectUnit object);
		bool updateBitmaskList(std::vector<MapObjectUnit> list);
		std::vector<MapObjectUnit> updateBitmaskRemove(MapObjectUnit object);

		// History (undo/redo)
		static const int historyMaxSize = 100;
		std::vector<HistoryEntry> undoStack;
		std::vector<HistoryEntry> redoStack;
		std::list<MapObjectUnit> historySpawnBuffer;
		bool matrixSpawnInProgress;
		bool recordHistory(HistoryActionType type, std::string description);
		bool undoAction();
		bool redoAction();
		bool clearHistory();

		// IMGUI rendering
		bool imguiRender();
		void imguiRenderMenuBar();
		void imguiRenderToolPanel();
		void imguiRenderPalettePanel();
		void imguiRenderPropertiesPanel();
		void imguiRenderEditsPanel();
		void imguiRenderNotification();
		void imguiRenderPaletteItems();
		void imguiRenderPaletteSelector();
		void imguiRenderPreferencesWindow();

		// Preferences window flag
		bool showPreferencesWindow;

		// Notification message
		std::string notificationText;
		float notificationTimer;
		float notificationDuration;

		// IMGUI edit state buffers
		char imguiMapName[128];
		char imguiMapMusic[128];
		char imguiMapVersion[128];
		char imguiWeatherName[128];
		char imguiParticles[128];
		char imguiExtraFields[7][128];

	private:
		// Track toggle button states for ImGui rendering
		bool gettingExtraValues;
		std::vector<std::string> extraFieldCaptions;
		std::vector<EditType> extraFieldTypes;
		std::vector<int> extraFieldMaxValues;
		std::vector<std::string> extraFieldOrigins;
		std::string formShapeSelected;
};

#endif