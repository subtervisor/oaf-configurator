cmake_minimum_required(VERSION 3.22)

project(OAFConfigurator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

set(APP_NAME "OAF configurator")
set(APP_DESC "Configures open_agb_firm's ini settings")
set(APP_AUTHOR "subtervisor")
set(APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/app/icon.png")
set(APP_ROMFS "${CMAKE_CURRENT_SOURCE_DIR}/romfs")

add_executable(OAFConfigurator
    source/main.cpp
)
target_link_libraries(OAFConfigurator PRIVATE
    m
    z
    ctru
    citro3d
    imgui
    imgui-ctr
)


target_compile_definitions(OAFConfigurator PRIVATE
	IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1
	IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS=1
)

# Generate 3DSX
ctr_generate_smdh(
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.smdh
    NAME "${APP_NAME}"
    DESCRIPTION "${APP_DESC}"
    AUTHOR "${APP_AUTHOR}"
    ICON "${APP_ICON}"
)
ctr_create_3dsx(
    ${PROJECT_NAME}
    OUTPUT "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.3dsx"
    SMDH "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.smdh"
    ROMFS "${APP_ROMFS}"
    DEPENDS create_resources
)
