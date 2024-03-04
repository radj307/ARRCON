#pragma once
// 307lib::shared
#include <make_exception.hpp> //< for ex::except
#include <indentor.hpp>		  //< for shared::indent()

// 307lib::TermAPI
#include <Message.hpp>		  //< for term::MessageMarginSize

class ExceptionBuilder {
	using this_t = ExceptionBuilder;

	std::stringstream ss;
	bool isFirstLine{ true };

public:
	ExceptionBuilder() {}

	/**
	 * @brief		Builds an exception and returns it.
	 * @returns		ex::except with the previously-specified message.
	 */
	ex::except build() const
	{
		return{ ss.str() };
	}

	/**
	 * @brief				Adds a line to the exception message.
	 * @param ...content  -	The content of the line.
	 * @returns				*this
	 */
	this_t& line(auto&&... content)
	{
		if (!isFirstLine)
			ss << std::endl << indent(term::MessageMarginSize);
		else isFirstLine = false;

		(ss << ... << std::forward<decltype(content)>(content));

		return *this;
	}
};
