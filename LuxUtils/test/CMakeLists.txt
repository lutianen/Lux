add_executable(AtomicTest Atomic_unit.cc)
target_link_libraries(AtomicTest PRIVATE LuxUtils)

add_executable(ExceptionTest Exception_unit.cc)
target_link_libraries(ExceptionTest PRIVATE LuxUtils)

add_executable(FileUtilTest FileUtil_unit.cc)
target_link_libraries(FileUtilTest PRIVATE LuxUtils)

add_executable(MutexTest Mutex_unit.cc)
target_link_libraries(MutexTest PRIVATE LuxUtils)

add_executable(ProcessInfoTest ProcessInfo_unit.cc)
target_link_libraries(ProcessInfoTest PRIVATE LuxUtils)

add_executable(ThreadTest Thread_unit.cc)
target_link_libraries(ThreadTest PRIVATE LuxUtils)

add_executable(TimestampTest Timestamp_unit.cc)
target_link_libraries(TimestampTest PRIVATE LuxUtils)

add_executable(LuxINITest LuxINI_unit.cc)
target_link_libraries(LuxINITest PRIVATE LuxUtils)


