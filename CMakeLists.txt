cmake_minimum_required(VERSION 3.22)
project(E33-Editor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if (MSVC)
    add_compile_options(/permissive- /Zc:__cplusplus)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE
        "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif()

set(OBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../obs-studio-33" CACHE PATH
    "Codigo fuente de OBS Studio")
set(OBS_BUILD "${OBS_PATH}/build_x64" CACHE PATH
    "Directorio build de OBS Studio")
set(QT6_PATH "C:/Qt/6.11.1/msvc2022_64" CACHE PATH
    "Instalacion de Qt6 (carpeta msvc2022_64)")

set(CMAKE_PREFIX_PATH "${QT6_PATH}")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_library(libobs SHARED IMPORTED)
set_target_properties(libobs PROPERTIES
    IMPORTED_LOCATION "${OBS_BUILD}/libobs/Release/obs.dll"
    IMPORTED_IMPLIB   "${OBS_BUILD}/libobs/Release/obs.lib"
    INTERFACE_INCLUDE_DIRECTORIES
        "${OBS_PATH}/libobs;${OBS_BUILD}/libobs")

add_library(obs-frontend-api SHARED IMPORTED)
set_target_properties(obs-frontend-api PROPERTIES
    IMPORTED_LOCATION "${OBS_BUILD}/frontend/api/Release/obs-frontend-api.dll"
    IMPORTED_IMPLIB   "${OBS_BUILD}/frontend/api/Release/obs-frontend-api.lib"
    INTERFACE_INCLUDE_DIRECTORIES
        "${OBS_PATH}/frontend/api;${OBS_BUILD}/frontend/api;${OBS_BUILD}/config")

add_library(E33-Editor SHARED
    plugin.cpp
    plugin.hpp

    resources.qrc

    editor-dock.cpp
    editor-dock.hpp

    editor-widget.cpp
    editor-widget.hpp

    editor-controller.cpp
    editor-controller.hpp

    editor-model.cpp
    editor-model.hpp

    editor-ui-helpers.cpp
    editor-ui-helpers.hpp

    src/source/source-tab.cpp
    src/source/source-tab.hpp

    src/source/copy-paste.cpp
    src/source/copy-paste.hpp

    src/source/source-color.cpp
    src/source/source-color.hpp

    src/source/transitions.cpp
    src/source/transitions.hpp

    src/source/image-settings.cpp
    src/source/image-settings.hpp

    src/source/video-settings.cpp
    src/source/video-settings.hpp

    src/source/audio-settings.cpp
    src/source/audio-settings.hpp

    src/properties/double-slider.cpp
    src/properties/double-slider.hpp

    src/properties/IconLabel.cpp
    src/properties/IconLabel.hpp
    
    src/properties/plain-text-edit.cpp
    src/properties/plain-text-edit.hpp

    src/properties/properties-tab.cpp
    src/properties/properties-tab.hpp

    src/properties/properties-view.cpp
    src/properties/properties-view.hpp
    src/properties/properties-view.moc.hpp

    src/properties/slider-ignorewheel.cpp
    src/properties/slider-ignorewheel.hpp

    src/properties/spinbox-ignorewheel.cpp
    src/properties/spinbox-ignorewheel.hpp

    src/properties/vertical-scroll-area.cpp
    src/properties/vertical-scroll-area.hpp

    src/transform/transform-tab.cpp
    src/transform/transform-tab.hpp

    src/transform/alignment.cpp
    src/transform/alignment.hpp

    src/transform/copypaste.cpp
    src/transform/copypaste.hpp

    src/transform/edit-bounds.cpp
    src/transform/edit-bounds.hpp

    src/transform/edit-crop.cpp
    src/transform/edit-crop.hpp
    
    src/transform/edit-position.cpp
    src/transform/edit-position.hpp

    src/transform/edit.cpp
    src/transform/edit.hpp

    src/transform/quick.cpp
    src/transform/quick.hpp

    src/filters/filters-tab.cpp
    src/filters/filters-tab.hpp

    src/scene-filters/scene-filters-tab.cpp
    src/scene-filters/scene-filters-tab.hpp

    src/notes/notes-tab-scene.cpp
    src/notes/notes-tab-scene.hpp
    src/notes/notes-tab-source.cpp
    src/notes/notes-tab-source.hpp

    src/notes/menu-context.cpp
    src/notes/menu-context.hpp

    src/notes/edit-note-bar.cpp
    src/notes/edit-note-bar.hpp

    src/filters-files/filters-copy-paste.cpp
    src/filters-files/filters-copy-paste.hpp
    
    src/filters-files/filters-model.cpp
    src/filters-files/filters-model.hpp

    src/filters-files/filters-proper-renderer.cpp
    src/filters-files/filters-proper-renderer.hpp

    src/filters-files/filters-renderer.cpp
    src/filters-files/filters-renderer.hpp

    src/filters-files/filters-state.cpp
    src/filters-files/filters-state.hpp

    src/filters-files/filters-window.cpp
    src/filters-files/filters-window.hpp
    
    src/widgets/alignment-grid.cpp
    src/widgets/alignment-grid.hpp

    src/widgets/e33-spinbox.cpp
    src/widgets/e33-spinbox.hpp

    src/widgets/box-filters.cpp
    src/widgets/box-filters.hpp

    src/widgets/box.cpp
    src/widgets/box.hpp

    src/widgets/filters-header.cpp
    src/widgets/filters-header.hpp

    src/widgets/separator.cpp
    src/widgets/separator.hpp

    src/widgets/header.cpp
    src/widgets/header.hpp

    src/widgets/information.cpp
    src/widgets/information.hpp    
    src/widgets/preview-source.cpp
    src/widgets/preview-source.hpp

    src/widgets/sign.cpp
    src/widgets/sign.hpp
)

target_include_directories(E33-Editor PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(E33-Editor PRIVATE
    libobs
    obs-frontend-api
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

set(OBS_INSTALL_PATH "C:/Program Files/obs-studio" CACHE PATH
    "Carpeta de instalacion de OBS Studio")
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${OBS_INSTALL_PATH}" CACHE PATH
        "Prefijo de instalacion" FORCE)
endif()

install(TARGETS E33-Editor
    RUNTIME DESTINATION "obs-plugins/64bit"
    LIBRARY DESTINATION "obs-plugins/64bit"
    ARCHIVE DESTINATION "lib")

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data/obs-plugins/E33-Editor"
        DESTINATION "data/obs-plugins")
