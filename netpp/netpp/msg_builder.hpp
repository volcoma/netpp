#pragma once
#include "connector.h"
#include <cstdint>
#include <cstring>

namespace net
{

template <typename T>
class msg_builder
{
public:
	static size_t get_header_size()
	{
		return sizeof(T);
	}

	static byte_buffer write_header(const byte_buffer& msg)
	{
		auto msg_size = static_cast<T>(msg.size());
		size_t header_size = get_header_size();

		byte_buffer output_msg;
		output_msg.reserve(header_size + msg_size);
		for(size_t i = 0; i < header_size; ++i)
		{
			output_msg.push_back(char(msg_size >> (i * 8)));
		}

		return output_msg;
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
			return get_header_size();
		}

		return msg_size;
	}
	bool is_ready() const
	{
		return buffer.size() == msg_size;
	}

	byte_buffer&& extract_buffer()
	{
		msg_size = 0;
		return std::move(buffer);
	}

	byte_buffer& get_buffer()
	{
		return buffer;
	}

private:
	byte_buffer buffer;
	size_t msg_size = 0;
};

} // namespace net
