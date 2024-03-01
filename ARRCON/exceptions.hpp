#pragma once
// 307lib::shared
#include <sysarch.h>					//< for preprocessor defs
#include <make_exception.hpp>			//< for ex::except
#include <strcore.hpp>					//< for str::stringify

// Boost
#ifdef OS_WIN
// prevents "fatal error C1189: #error:  WinSock.h has already been included"
// this MUST be included prior to boost/stacktrace.hpp on Windows
#include <boost/asio/detail/socket_types.hpp>
#endif
#include <boost/system/error_code.hpp>	//< for boost::system::error_code
#include <boost/stacktrace.hpp>			//< for boost::stacktrace::stacktrace
#include <boost/exception/all.hpp>		//< for boost::error_info

// STL
#include <cstdint>

using traced_exception = boost::error_info<struct tag_stacktrace, boost::stacktrace::stacktrace>;

/**
 * @brief               Throws the specified exception with an attached stacktrace.
 * @tparam Ex         - The type of exception to throw.
 * @param ex          - The exception instance to throw.
 * @param stacktrace  - The stacktrace instance to throw. Defaults to a new stacktrace for the caller.
 */
template<class Ex>
void throw_with_stacktrace(Ex const& ex, boost::stacktrace::stacktrace&& stacktrace = {})
{
    throw boost::enable_error_info(ex) << traced_exception(std::forward<boost::stacktrace::stacktrace>(stacktrace));
}
