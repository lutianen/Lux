add_executable(LuxRedisTest LuxRedis_unit.cc)
target_link_libraries(LuxRedisTest PRIVATE LuxUtils LuxRedis)
