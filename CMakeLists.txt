cmake_minimum_required(VERSION 3.11)
project(lvgl_test)

option(WERROR "Treat warnings as errors for LVGL targets" OFF)
option(ASAN "Enable Sanitizer Options" OFF)

# Set policy to allow to run the target_link_libraries cmd on targets that are build
# in another directory.
# Currently, the linking is not handled by env_support/cmake/os.cmake
# This means that if a driver is enabled and it requires linking an external library
# it needs to be handled in the top-level project.

cmake_policy(SET CMP0079 NEW)

# Uncomment if the program needs debugging
#set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 99) # LVGL officially supports C99 and above
set(CMAKE_CXX_STANDARD 17) #C17
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")

find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(ASAN)
    message(STATUS "Enabling Sanitizer")
    add_compile_options(-fsanitize=address,undefined,null)
    add_link_options(-fsanitize=address,undefined,null,leak)
endif()

set(CONFIG "default" CACHE STRING "Config name to use. Leave empty for default `lv_conf.defaults`")

set(LV_CONF_DEFAULTS_PATH "${CMAKE_SOURCE_DIR}/lv_conf.defaults")

# If CONFIG is not default, update LV_CONF_DEFAULTS_PATH to the selected config
if(NOT CONFIG STREQUAL "default")
    set(LV_CONF_DEFAULTS_PATH "${CMAKE_SOURCE_DIR}/configs/${CONFIG}.defaults")
endif()

if(NOT EXISTS "${LV_CONF_DEFAULTS_PATH}")
    message(FATAL_ERROR "Config file not found: ${LV_CONF_DEFAULTS_PATH}")
endif()
message(STATUS "Using config: ${LV_CONF_DEFAULTS_PATH}")

set(LV_BUILD_SET_CONFIG_OPTS ON CACHE BOOL
    "create CMAKE variables from lv_conf_internal.h" FORCE)

set(LV_BUILD_CONF_PATH
    "${CMAKE_BINARY_DIR}/lv_conf.h"
    CACHE PATH "path to lv_conf.h" FORCE)

set(LVGL_TEMPLATE_PATH "${CMAKE_SOURCE_DIR}/lvgl/lv_conf_template.h")
set(GENERATE_SCRIPT_PATH "${CMAKE_SOURCE_DIR}/lvgl/scripts/generate_lv_conf.py")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${LVGL_TEMPLATE_PATH})
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${LV_CONF_DEFAULTS_PATH})
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${GENERATE_SCRIPT_PATH})

execute_process(
  COMMAND
    ${Python3_EXECUTABLE} ${GENERATE_SCRIPT_PATH} --template
    ${LVGL_TEMPLATE_PATH} --defaults ${LV_CONF_DEFAULTS_PATH} --config
    ${LV_BUILD_CONF_PATH}
  RESULT_VARIABLE config_result
  OUTPUT_VARIABLE config_output
  ERROR_VARIABLE config_output)

if(NOT config_result EQUAL 0)
  message(FATAL_ERROR "Failed to generate lv_conf.h: ${config_output}")
endif()

message(STATUS "${LV_BUILD_CONF_PATH} generated successfully")

add_subdirectory(lvgl)

# if (CONFIG_LV_USE_EVDEV)
#     message("Including EVDEV support")
#     find_package(PkgConfig REQUIRED)
#     pkg_check_modules(EVDEV REQUIRED libevdev)

#     list(APPEND PKG_CONFIG_LIB ${EVDEV_LIBRARIES})
#     list(APPEND PKG_CONFIG_INC ${EVDEV_INCLUDE_DIRS})
#     list(APPEND LV_LINUX_BACKEND_SRC src/lib/indev_backends/evdev.c)

# endif()

# if (CONFIG_LV_USE_LIBINPUT)
#     message("Including libinput support")

#     find_package(PkgConfig REQUIRED)
#     pkg_check_modules(LIBINPUT REQUIRED libinput)

#     list(APPEND PKG_CONFIG_LIB ${LIBINPUT_LIBRARIES})
#     list(APPEND PKG_CONFIG_INC ${LIBINPUT_INCLUDE_DIRS})

# endif()

# if (CONFIG_LV_USE_FREETYPE)
#     message("Including Freetype support")

#     find_package(PkgConfig REQUIRED)
#     pkg_check_modules(LIBFREETYPE REQUIRED freetype2)

#     list(APPEND PKG_CONFIG_LIB ${LIBFREETYPE_LIBRARIES})
#     list(APPEND PKG_CONFIG_INC ${LIBFREETYPE_INCLUDE_DIRS})

# endif()


target_include_directories(lvgl PUBLIC ${PKG_CONFIG_INC})

# Library type is determined by BUILD_SHARED_LIBS (shared if ON, static if OFF)
add_library(lvgl_linux ${LV_LINUX_SRC} ${LV_LINUX_BACKEND_SRC})
target_include_directories(lvgl_linux PUBLIC ${PKG_CONFIG_INC})

# If LVGL is configured to use LV_CONF_PATH or Kconfig
# Set the exactly the same definitions on the lvgl_linux target
set_target_properties(lvgl_linux PROPERTIES COMPILE_DEFINITIONS "${LVGL_COMPILER_DEFINES}")
target_include_directories(lvgl_linux PUBLIC
    ${LV_LINUX_INC} ${CMAKE_CURRENT_SOURCE_DIR}
    ${LVGL_CONF_INC_DIR})

# Link LVGL with external dependencies - Modern CMake/CMP0079 allows this
target_link_libraries(lvgl PUBLIC ${PKG_CONFIG_LIB} m pthread rt dl)

add_executable(lvgl_test src/main.c)

# Repeat lvgl_linux to resolve circular dependency with lvgl
target_link_libraries(lvgl_test lvgl_linux lvgl lvgl_linux)

if(WERROR)
    target_compile_options(lvgl_test PRIVATE -Werror)
    target_compile_options(lvgl PRIVATE -Werror)
    target_compile_options(lvgl_linux PRIVATE -Werror)
endif()

add_custom_target(run COMMAND ${EXECUTABLE_OUTPUT_PATH}/lvgl_test DEPENDS lvgl_test)
