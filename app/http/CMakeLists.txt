file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cc include/*.h)
add_executable(httpServer ${srcs})
target_include_directories(httpServer PUBLIC include)
target_link_libraries(httpServer LuxUtils LuxLog polaris LuxMySQL mysqlclient)

