cmake_minimum_required(VERSION 3.15)
project(realm-editor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(
    -DSFML_STATIC 
    -D_CRT_SECURE_NO_WARNINGS 
    -DSFML_NO_DEPRECATED_WARNINGS 
    -DSFML_DYNAMIC
)

set(SOURCES
    "source/main.cpp"
    "source/system/external/imgui/imgui-SFML.cpp"
    "source/system/external/imgui/imgui.cpp"
    "source/system/external/imgui/imgui_draw.cpp"
    "source/system/external/imgui/imgui_tables.cpp"
    "source/system/external/imgui/imgui_widgets.cpp"
    "source/system/external/RichText/RichText.cpp"
    "source/system/hud/button.cpp"
    "source/system/hud/edit.cpp"
    "source/system/hud/hud.cpp"
    "source/system/library/json.cpp"
    "source/system/library/position.cpp"
    "source/system/library/script.cpp"
    "source/system/manager.cpp"
    "source/system/map/map.cpp"
    "source/system/model/model.cpp"
    "source/system/model/texture.cpp"
    "source/system/object/viewElement.cpp"
    "source/system/palette/palette.cpp"
    "source/system/text/label.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE 
    "source/system/external/imgui"
    "source/system/external/nlohmann"
    "source/system/external/RichText"
    "source/system/hud"
    "source/system/library"
    "source/system/map"
    "source/system/model"
    "source/system/object"
    "source/system/palette"
    "source/system/text"
    "libs/boost_1_74_0"
    "libs/SFML-2.5.1-x64/include"
    "libs/Candle-master/include"
)

set(BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/libs/boost_1_74_0")
set(BOOST_LIB_DIR "${BOOST_ROOT}/stage/lib")

target_include_directories(${PROJECT_NAME} PRIVATE "${BOOST_ROOT}")

if(WIN32)
    target_link_directories(${PROJECT_NAME} PRIVATE "${BOOST_LIB_DIR}")
endif()

if(WIN32)
    set(SFML_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs/SFML-2.5.1-x64/lib")

    target_link_libraries(${PROJECT_NAME} PRIVATE 
        "${SFML_LIB_PATH}/sfml-graphics-s.lib"
        "${SFML_LIB_PATH}/sfml-window-s.lib"
        "${SFML_LIB_PATH}/sfml-system-s.lib"
        "${SFML_LIB_PATH}/sfml-audio-s.lib"
        "${SFML_LIB_PATH}/sfml-network-s.lib"
        "${SFML_LIB_PATH}/sfml-main.lib"
        # Dependências externas com caminho completo para evitar LNK1181
        "${SFML_LIB_PATH}/freetype.lib"
        "${SFML_LIB_PATH}/flac.lib"
        "${SFML_LIB_PATH}/vorbisenc.lib"
        "${SFML_LIB_PATH}/vorbisfile.lib"
        "${SFML_LIB_PATH}/vorbis.lib"
        "${SFML_LIB_PATH}/ogg.lib"
        "${SFML_LIB_PATH}/openal32.lib"
        # Bibliotecas nativas do Windows
        opengl32
        winmm
        gdi32
        ws2_32
    )
endif()