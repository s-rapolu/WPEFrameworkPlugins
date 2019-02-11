set(PLUGIN_NAME OCDM)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

message("Setting up ${PLUGIN_NAME}")

option(PLUGIN_OCDM_PLAYREADY_READ_DIR "Playready read-dir" "/root/Netflix/data/dpi/playready")
option(PLUGIN_OCDM_PLAYREADY_STORE_LOCATION "Playready store-location" "/root/Netflix/data/dpi/playready/storage/drmstore")

set(PLUGIN_SOURCES 
        OCDM.cpp
        CENCParser.cpp
        FrameworkRPC.cpp
        Module.cpp)

find_package(ocdm REQUIRED)

# Library definition section
add_library(${MODULE_NAME} SHARED ${PLUGIN_SOURCES})

if("${CMAKE_FIND_ROOT_PATH}" STREQUAL "")
   # Desktop case: not cross compiling
   target_link_libraries(${MODULE_NAME} "${CMAKE_INSTALL_PREFIX}/lib/libocdm.so")
else()
   # Cross compiling: buildroot
   target_link_libraries(${MODULE_NAME} ${PLUGINS_LIBRARIES} ${OCDM_LIBRARIES})
endif()

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

write_config(${PLUGIN_NAME})
