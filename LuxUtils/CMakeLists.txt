# GLOB_RECURSE 允许 * 匹配嵌套的目录
# GLOB 只允许查找一级目录
# CONFIGURE_DEPENDS，每次运行 cmake --build build 时，自动检测目录是否更新；
#    如果目录有新文件了， CMake 会自动帮你运行 cmake -B build 更新 srcs
# include/*.h -->> 为了方便 vs ，Linux 可不加
file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cc include/*.h)

# 静态库
add_library(LuxUtils STATIC ${srcs})

# 相对路径
# PUBLIC 修饰符，是为了让链接该项目的其他项目也能够共享这个头文件搜索路径
target_include_directories(LuxUtils PUBLIC include)

# 依赖
target_link_libraries(LuxUtils pthread)

if (NOT BUILD_TEST)
    message("Build LuxUtils tests.")
    add_subdirectory(test)
else()
    message("Don't Build LuxUtils tests.")
endif()

