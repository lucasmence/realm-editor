cmake_minimum_required(VERSION 3.15)
project(realm-editor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
)

if(WIN32)
    add_definitions(-DSFML_STATIC -D_CRT_SECURE_NO_WARNINGS -DSFML_DYNAMIC)
    
    set(BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/libs/boost_1_74_0")
    set(SFML_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs/SFML-2.5.1-x64/lib")
    
    target_include_directories(${PROJECT_NAME} PRIVATE 
        "${BOOST_ROOT}"
        "libs/SFML-2.5.1-x64/include"
    )

    target_link_directories(${PROJECT_NAME} PRIVATE "${BOOST_ROOT}/stage/lib")
    
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        "${SFML_LIB_PATH}/sfml-graphics-s.lib" "${SFML_LIB_PATH}/sfml-window-s.lib"
        "${SFML_LIB_PATH}/sfml-system-s.lib" "${SFML_LIB_PATH}/sfml-audio-s.lib"
        "${SFML_LIB_PATH}/sfml-network-s.lib" "${SFML_LIB_PATH}/sfml-main.lib"
        "${SFML_LIB_PATH}/freetype.lib" "${SFML_LIB_PATH}/flac.lib"
        "${SFML_LIB_PATH}/vorbisenc.lib" "${SFML_LIB_PATH}/vorbisfile.lib"
        "${SFML_LIB_PATH}/vorbis.lib" "${SFML_LIB_PATH}/ogg.lib"
        "${SFML_LIB_PATH}/openal32.lib"
        opengl32 winmm gdi32 ws2_32
    )
else()
    find_package(Boost REQUIRED COMPONENTS filesystem system)
    find_package(SFML 2.5 COMPONENTS graphics window system audio network REQUIRED)
    find_package(OpenGL REQUIRED)

    target_link_libraries(${PROJECT_NAME} PRIVATE 
        sfml-graphics sfml-window sfml-system sfml-audio sfml-network
        Boost::filesystem
        Boost::system
        OpenGL::GL
        pthread
    )
endif()