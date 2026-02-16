add_executable(paper_trader
  main.cpp
)

target_link_libraries(paper_trader
  PRIVATE
    club::engine_core
    club::engine_risk
    club::contracts
)

club_set_target_defaults(paper_trader)
