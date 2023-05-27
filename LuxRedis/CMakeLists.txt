file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cc include/*.h)

# 静态库
add_library(LuxRedis STATIC ${srcs})
# 相对路径
# PUBLIC 修饰符，是为了让链接该项目的其他项目也能够共享这个头文件搜索路径
target_include_directories(LuxRedis PUBLIC include)
# 依赖
target_link_libraries(LuxRedis PUBLIC LuxUtils LuxLog hiredis)

if (NOT BUILD_TEST)
    message("Build LuxRedis tests.")
    add_subdirectory(test)
else()
    message("Don't Build LuxRedis tests.")
endif()

