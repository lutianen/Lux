add_executable(AtomicTest Atomic_unit.cc)
target_link_libraries(AtomicTest PUBLIC LuxUtils)

add_executable(ExceptionTest Exception_unit.cc)
target_link_libraries(ExceptionTest PUBLIC LuxUtils)

add_executable(FileUtilTest FileUtil_unit.cc)
target_link_libraries(FileUtilTest PUBLIC LuxUtils)

add_executable(MutexTest Mutex_unit.cc)
target_link_libraries(MutexTest PUBLIC LuxUtils)

add_executable(ProcessInfoTest ProcessInfo_unit.cc)
target_link_libraries(ProcessInfoTest PUBLIC LuxUtils)

add_executable(ThreadTest Thread_unit.cc)
target_link_libraries(ThreadTest PUBLIC LuxUtils)

add_executable(TimestampTest Timestamp_unit.cc)
target_link_libraries(TimestampTest PUBLIC LuxUtils)

add_executable(LuxINITest LuxINI_unit.cc)
target_link_libraries(LuxINITest PUBLIC LuxUtils)


