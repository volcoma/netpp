#pragma once
#include <iostream>
#include <sstream>
struct sout
{
	~sout()
	{
		std::cout << str.str();
	}
	template <typename T>
	sout& operator<<(T&& val)
	{
		str << val;
		return *this;
	}

	std::stringstream str;
};
