#pragma once
#include "connector.h"

namespace net
{

struct msg_builder
{
	msg_builder(size_t header_sz)
		: header_size(header_sz)
	{
	}
	void read_chunk()
	{
		if(msg_size == 0)
		{
			// read header
			std::memcpy(&msg_size, buffer.data(), buffer.size());
			buffer.clear();
		}
	}

	size_t get_next_read() const
	{
		if(msg_size == 0)
		{
			return header_size;
		}

		return msg_size;
	}
	bool is_ready() const
	{
		return buffer.size() == msg_size;
	}

	void clear()
	{
		msg_size = 0;
		buffer.clear();
	}

	size_t header_size = 0;
	size_t msg_size = 0;
	byte_buffer buffer;
};

} // namespace net
