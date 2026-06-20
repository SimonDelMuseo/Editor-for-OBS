cmake_minimum_required(VERSION 3.22)
project(E33-Editor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt: metaobjeto, UI y recursos automáticos
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Flags obligatorios para Qt6 + MSVC
add_compile_options(/permissive- /Zc:__cplusplus)

# Rutas
set(OBS_PATH "E:/OBS-PLUGIN/obs-studio-32.1")
set(OBS_BUILD "${OBS_PATH}/build")
set(QT6_PATH "C:/Qt/6.8.3/msvc2022_64")

# Para que find_package encuentre Qt6
set(CMAKE_PREFIX_PATH "${QT6_PATH}")

# Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# INCLUDES
include_directories(
    "${OBS_PATH}/libobs"
    "${OBS_PATH}/frontend/api"

    "${OBS_BUILD}/libobs"
    "${OBS_BUILD}/frontend/api"
    "${OBS_BUILD}/config"

    "${QT6_PATH}/include"
    "${QT6_PATH}/include/QtCore"
    "${QT6_PATH}/include/QtGui"
    "${QT6_PATH}/include/QtWidgets"

    .
)

# DIRECTORIOS DE LIBRERÍAS
link_directories(
    "${OBS_BUILD}/libobs/Release"
    "${OBS_BUILD}/frontend/api/Release"
)

# ============================================================
# LIBRERÍA DEL PLUGIN
# ============================================================
add_library(E33-Editor SHARED
    plugin.cpp
    plugin.hpp

    editor-dock.cpp
    editor-dock.hpp

    editor-widget.cpp
    editor-widget.hpp

    editor-controller.cpp
    editor-controller.hpp

    editor-model.cpp
    editor-model.hpp

    # SOURCE TAB
    src/source/source-tab.cpp
    src/source/source-tab.hpp

    # Contenido Source
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

    # PROPERTIES TAB
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

    # TRANSFORM TAB
    src/transform/transform-tab.cpp
    src/transform/transform-tab.hpp

    # Contenido Transform
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

    # SOURCE FILTERS TAB
    src/filters/filters-tab.cpp
    src/filters/filters-tab.hpp

    # SCENE FILTERS TAB
    src/scene-filters/scene-filters-tab.cpp
    src/scene-filters/scene-filters-tab.hpp

    # Contenido FILTERS TABS
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

    # MISC TAB
    src/misc/misc-tab.cpp
    src/misc/misc-tab.hpp

    # WIDGETS
    src/widgets/alignment-grid.cpp
    src/widgets/alignment-grid.hpp

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
)

# FLAGS DEL TARGET
target_compile_options(E33-Editor PRIVATE
    /permissive-
    /Zc:__cplusplus
)

# ENLACE
target_link_libraries(E33-Editor
    obs.lib
    obs-frontend-api
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
