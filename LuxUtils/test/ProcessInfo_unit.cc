#include <LuxUtils/ProcessInfo.h>

#include <cstdio>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

int main() {
    printf("pid = %d\n", Lux::ProcessInfo::pid());
    printf("uid = %d\n", Lux::ProcessInfo::uid());
    printf("euid = %d\n", Lux::ProcessInfo::euid());
    printf("start time = %s\n",
           Lux::ProcessInfo::startTime().toFormattedString().c_str());
    printf("hostname = %s\n", Lux::ProcessInfo::hostname().c_str());
    printf("opened files = %d\n", Lux::ProcessInfo::openedFiles());
    printf("threads = %zd\n", Lux::ProcessInfo::threads().size());
    printf("num threads = %d\n", Lux::ProcessInfo::numThreads());
    printf("status = %s\n", Lux::ProcessInfo::procStatus().c_str());
}