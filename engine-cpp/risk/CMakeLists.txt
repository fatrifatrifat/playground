add_library(engine_risk STATIC
  src/risk_engine.cpp
)

target_include_directories(engine_risk PUBLIC include)
target_link_libraries(engine_risk PUBLIC club::contracts)
club_set_target_defaults(engine_risk)

add_library(club::engine_risk ALIAS engine_risk)
