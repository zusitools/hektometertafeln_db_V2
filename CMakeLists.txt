cmake_minimum_required(VERSION 3.1)
project(hektometertafeln)

set(CMAKE_CXX_STANDARD 17)

find_program(IWYU_PATH NAMES include-what-you-use)
if (IWYU_PATH)
  set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${IWYU_PATH})
endif()

set (SOURCES
  hekto_builder.cpp
  mesh.cpp
  textur.cpp
)

if (WIN32)
  set (SOURCES
    ${SOURCES}
    dll.cpp
    gui.cpp
    gui.rc
    version.rc
  )

  set_source_files_properties(gui.rc PROPERTIES LANGUAGE RC)
  set_source_files_properties(version.rc PROPERTIES LANGUAGE RC)
endif()

add_library(hektometertafeln_DB_V2 SHARED ${SOURCES})

if (WIN32)
  target_link_libraries(hektometertafeln_DB_V2 shlwapi)
endif()

set_target_properties(hektometertafeln_DB_V2 PROPERTIES PREFIX "") # no "lib..." in output name
if (WIN32 AND (CMAKE_SIZEOF_VOID_P EQUAL 8))
  set_target_properties(hektometertafeln_DB_V2 PROPERTIES SUFFIX ".64.dll")
endif()
target_compile_definitions(hektometertafeln_DB_V2 PRIVATE -DDLL_EXPORTS)
if(MINGW)
  set_target_properties(hektometertafeln_DB_V2 PROPERTIES LINK_FLAGS -Wl,--kill-at)
endif()
install(TARGETS hektometertafeln_DB_V2 DESTINATION bin)
