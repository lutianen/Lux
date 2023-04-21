#include <LuxLog/Logger.h>

int main(int argc, char** argv) {
    Lux::Logger::setLogLevel(Lux::Logger::LogLevel::INFO);

    LOG_TRACE << "This is log TRACE TEST";
    LOG_DEBUG << "This is log DEBUG TEST";
    LOG_INFO << "This is log INFO TEST";
    LOG_WARN << "This is log WARN TEST";
    LOG_ERROR << "This is log ERROR TEST";
    LOG_SYSERR << "This is log SYSERR TEST";
}