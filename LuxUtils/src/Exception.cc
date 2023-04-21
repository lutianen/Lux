/**
 * @file Exception.cc
 * @brief
 *
 * @author Tianen Lu
 */

#include <LuxUtils/CurrentThread.h>
#include <LuxUtils/Exception.h>

Lux::Exception::Exception(string msg)
    : message_(msg), stack_(CurrentThread::stackTrace(false)) {}
