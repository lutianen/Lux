/**
 * @file Exception.cc
 * @brief
 *
 * @author Tianen Lu (tianenlu957@gmail.com)
 */

#include <LuxUtils/CurrentThread.h>
#include <LuxUtils/Exception.h>

Lux::Exception::Exception(string msg)
    : message_(msg), stack_(CurrentThread::stackTrace(false)) {}
