#pragma once
#include <functional>
#include <string>
#include <iostream>
#include <sstream>
namespace net
{

using logger = std::function<void(const std::string&)>;
//-----------------------------------------------------------------------------
/// Sets the library logging callback.
//-----------------------------------------------------------------------------
void set_logger(const logger& log);

//-----------------------------------------------------------------------------
/// Logging utility class
//-----------------------------------------------------------------------------
struct log
{
	~log();
	template <typename T>
	log& operator<<(T&& val)
	{
		str << val;
		return *this;
	}

	std::stringstream str;
};
}
