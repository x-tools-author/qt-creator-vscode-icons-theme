cmake_minimum_required(VERSION 3.20)

project(VSCodeIconsTheme LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(QT_NO_PRIVATE_MODULE_WARNING ON)

# --------------------------------------------------------------------------------------------------
# * Download VSCode Icon Theme
# * https://codeload.github.com/vscode-icons/vscode-icons/zip/refs/tags/v12.17.0
set(VSCODE_ICONS_VERSION "12.17.0")
set(VSCODE_ICONS_URL "https://codeload.github.com/vscode-icons/vscode-icons/zip/refs/tags")
set(VSCODE_ICONS_URL "${VSCODE_ICONS_URL}/v${VSCODE_ICONS_VERSION}")
set(VSCODE_ICONS_FILE_NAME "vscode-icons")
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/3rd/${VSCODE_ICONS_FILE_NAME}.zip")
  message(STATUS "Downloading VSCode Icons v${VSCODE_ICONS_VERSION}...")
  file(
    DOWNLOAD "${VSCODE_ICONS_URL}" "${CMAKE_SOURCE_DIR}/3rd/${VSCODE_ICONS_FILE_NAME}.zip"
    SHOW_PROGRESS
    STATUS download_status)
  list(GET download_status 0 download_result)
  if(NOT download_result EQUAL 0)
    message(FATAL_ERROR "Failed to download VSCode Icons: ${download_status}")
    file(REMOVE "${CMAKE_SOURCE_DIR}/3rd/${VSCODE_ICONS_FILE_NAME}.zip")
  endif()
  message(STATUS "Extracting VSCode Icons...")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xfz "${CMAKE_SOURCE_DIR}/3rd/${VSCODE_ICONS_FILE_NAME}.zip"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/3rd")
  # Rename extracted folder
  file(RENAME "${CMAKE_SOURCE_DIR}/3rd/vscode-icons-${VSCODE_ICONS_VERSION}"
       "${CMAKE_SOURCE_DIR}/3rd/${VSCODE_ICONS_FILE_NAME}")
endif()

# --------------------------------------------------------------------------------------------------
find_package(QtCreator REQUIRED COMPONENTS Core)
find_package(
  Qt6
  COMPONENTS Gui GuiPrivate Svg
  REQUIRED)

add_qtc_plugin(
  VSCodeIconsTheme
  LONG_DESCRIPTION_MD
  README.md
  PLUGIN_DEPENDS
  QtCreator::Core
  DEPENDS
  Qt::Gui
  Qt::GuiPrivate
  Qt::Svg
  QtCreator::ExtensionSystem
  QtCreator::Utils
  SOURCES
  src/plugin.cpp
  resources.qrc
  README.md)

# --------------------------------------------------------------------------------------------------
if(WIN32)
  target_link_libraries(VSCodeIconsTheme PUBLIC dwmapi)
endif()
