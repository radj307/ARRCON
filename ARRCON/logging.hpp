#pragma once
// 307lib::shared
#include <strcore.hpp>	//< for str::stringify
#include <var.hpp>		//< for var::streamable

// Boost
#include <boost/date_time/posix_time/posix_time.hpp> //< for boost::posix_time

// STL
#include <cstdint>		//< for sized integer types
#include <ostream>		//< for std::ostream

// the margin size for the timestamp
#define LM_TIMESTAMP 17
// the margin size for the message level
#define LM_LEVEL 10

enum class LogLevel : uint8_t {
	/// @brief	Situational debugging information.
	Trace = 1,
	Debug = 2,
	Info = 4,
	Warning = 8,
	Error = 16,
	Critical = 32,
	Fatal = 64,
};

inline std::ostream& operator<<(std::ostream& os, const LogLevel& logLevel)
{
	switch (logLevel) {
	case LogLevel::Trace:
		os << "TRACE";
		break;
	case LogLevel::Debug:
		os << "DEBUG";
		break;
	case LogLevel::Info:
		os << "INFO";
		break;
	case LogLevel::Warning:
		os << "WARN";
		break;
	case LogLevel::Error:
		os << "ERROR";
		break;
	case LogLevel::Critical:
		os << "CRITICAL";
		break;
	case LogLevel::Fatal:
		os << "FATAL";
		break;
	default:
		throw make_exception(logLevel, " is an invalid value for the LogLevel enum!");
	}
	return os;
}

struct MessageHeader {
	LogLevel level;

	friend std::ostream& operator<<(std::ostream& os, const MessageHeader& m)
	{
		const auto timestamp{ boost::posix_time::to_iso_string(boost::posix_time::second_clock::universal_time()) };
		const auto level{ str::stringify(m.level) };
		return os
			<< timestamp << indent(LM_TIMESTAMP, timestamp.size())
			<< '[' << level << ']' << indent(LM_LEVEL, level.size() + 2);
	}
};

/**
 * @class	Logger
 * @brief	Manager object that handles swapping the read buffer of std::clog with another one.
 *			The read buffer is swapped back in the destructor.
 */
class Logger {
	std::streambuf* original_clog_rdbuf;

public:
	Logger(std::streambuf* rdbuf) : original_clog_rdbuf{ std::clog.rdbuf() }
	{
		// swap clog rdbuf
		std::clog.rdbuf(rdbuf);
	}
	~Logger()
	{
		// reset clog rdbuf
		std::clog.rdbuf(original_clog_rdbuf);
	}

	/**
	 * @brief	Prints a header line that indicates the segments of a log message.
	 */
	void print_header() const
	{
		std::clog << "YYYYMMDDTHHMMSS" << indent(LM_TIMESTAMP, 15) << "LEVEL" << indent(LM_LEVEL, 5) << "MESSAGE" << std::endl;
	}
};
