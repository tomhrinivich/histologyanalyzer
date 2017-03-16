cmake_minimum_required (VERSION 2.8.11)
if(COMMAND CMAKE_POLICY)
  cmake_policy(SET CMP0003 NEW)
endif()

project (histologyanalyzer)
add_subdirectory (sqlite)

# Find ITK.
find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

add_executable(histologyanalyzer histologyanalyzer.cpp )

#add libraries
target_link_libraries(histologyanalyzer ${ITK_LIBRARIES} sqlite3)