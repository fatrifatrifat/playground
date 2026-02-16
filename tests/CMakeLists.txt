add_executable(smoke_test smoke_test.cpp)
target_link_libraries(smoke_test PRIVATE club::contracts)
club_set_target_defaults(smoke_test)

add_test(NAME smoke_test COMMAND smoke_test)
