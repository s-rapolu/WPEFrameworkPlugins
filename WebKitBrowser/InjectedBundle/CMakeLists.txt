set(PLUGIN_NAME InjectedBundle)
set(MODULE_NAME WPE${PLUGIN_NAME})

message("Setting up ${PLUGIN_NAME}")

set(PLUGIN_SOURCES
    main.cpp
)

# Library definition section
add_library(${MODULE_NAME} SHARED ${PLUGIN_SOURCES})
target_include_directories(${MODULE_NAME} PRIVATE ${WPE_WEBKIT_INCLUDE_DIRS} ${WPE_WEBKIT_INCLUDE_DIRS}/WPE)
target_link_libraries(${MODULE_NAME} ${PLUGINS_LIBRARIES})

# Library installation section
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}/WebKitBrowser)
