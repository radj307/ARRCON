#pragma once
// 307lib::shared
#include <strcore.hpp>	//< for str::stringify
#include <var.hpp>		//< for var::streamable

// Boost
#include <boost/date_time/posix_time/posix_time.hpp> //< for boost::posix_time

// STL
#include <cstdint>		//< for sized integer types
#include <ostream>		//< for std::ostream

#define LM_TIMESTAMP 29
#define LM_LEVEL 12

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

class LogWriter {
	std::ostream& output_target;
	LogLevel filter;

public:
	LogWriter(std::ostream& os, LogLevel const filter) : output_target{ os }, filter{ filter } {}

#pragma region Filter

	// Gets the current message filter
	LogLevel get_filter() const { return filter; }
	// Sets the message filter
	void set_filter(LogLevel const newFilter) { filter = newFilter; }
	// Adds the specified log level to the message filter
	LogLevel add_filter(LogLevel const level) { return filter = static_cast<LogLevel>(static_cast<int32_t>(filter) | static_cast<int32_t>(level)); }
	// Removes the specified log level from the message filter
	LogLevel remove_filter(LogLevel const level) { return filter = static_cast<LogLevel>(static_cast<int32_t>(filter) & ~static_cast<int32_t>(level)); }

#pragma endregion Filter

	/// @brief	Gets the log's read buffer
	std::streambuf* rdbuf()
	{
		return output_target.rdbuf();
	}

	friend LogWriter& operator<<(LogWriter& lw, auto&& data)
	{
		lw.output_target << std::forward<decltype(data)>(data);
		return lw;
	}
};
