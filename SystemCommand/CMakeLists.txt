set(PLUGIN_NAME SystemCommand)

include(module) # Setup default stuff needed for the cmake framework.

find_package(Interfaces REQUIRED)

set(PLUGIN_DEFINITIONS
    )

set(PLUGIN_OPTIONS)

set(PLUGIN_LINK_OPTIONS)

set(PLUGIN_INCLUDE_DIRS
    ${PLUGINS_INCLUDE_DIR})


set(PLUGIN_SOURCES
    Module.cpp
    SystemCommand.cpp
    SystemCommandImplementation.cpp
)

set(PLUGIN_INPUT
    ${PLUGIN_SOURCES}
    ${PLUGIN_INCLUDES}
    )

set(PLUGIN_DEPENDENCIES
    ${PLUGINS_LIBRARIES}
    ${ADDITIONAL_LIBRARIES}
    ${INTERFACES_LIBRARIES})


include(default_targets) # This is a regular plugin no need to edit the defaults
target_link_libraries(${MODULE_NAME} ${INTERFACES_LIBRARIES})
write_config(${PLUGIN_NAME})
