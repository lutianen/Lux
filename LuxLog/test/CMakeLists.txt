add_executable(LoggerTest Logger_unit.cc)
target_link_libraries(LoggerTest PRIVATE LuxLog)

add_executable(example_1 example_1.cc)
target_link_libraries(example_1 PRIVATE LuxLog)

add_executable(example_2 example_2.cc)
target_link_libraries(example_2 PRIVATE LuxLog)

add_executable(example_3 example_3.cc)
target_link_libraries(example_3 PRIVATE LuxLog)

