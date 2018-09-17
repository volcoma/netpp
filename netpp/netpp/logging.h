#pragma once
#include <functional>
#include <iostream>
#include <sstream>
namespace net
{

using logger = std::function<void(const std::string&)>;
void set_logger(const logger& log);

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
