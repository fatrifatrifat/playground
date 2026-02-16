add_library(engine_core STATIC
  src/engine.cpp
)

target_include_directories(engine_core PUBLIC include)
target_link_libraries(engine_core PUBLIC club::contracts)
club_set_target_defaults(engine_core)

add_library(club::engine_core ALIAS engine_core)
