add_executable(EchoServer EchoServer_unit.cc)
target_link_libraries(EchoServer PRIVATE LuxUtils LuxLog polaris)

add_executable(EchoClient EchoClient_unit.cc)
target_link_libraries(EchoClient PRIVATE LuxUtils LuxLog polaris)