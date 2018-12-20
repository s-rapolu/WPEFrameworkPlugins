set(PLUGIN_NAME CrashDummy)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION})

message("Setting up ${PLUGIN_NAME}")

add_library(${PLUGIN_NAME} SHARED
        Module.cpp
        CrashDummy.cpp)


set_target_properties(${PLUGIN_NAME}
        PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED YES
            FRAMEWORK FALSE
            OUTPUT_NAME ${MODULE_NAME})

install(TARGETS ${PLUGIN_NAME}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/wpeframework/plugins
        COMPONENT ${MODULE_NAME})

write_config(${PLUGIN_NAME})
