#pragma once
#include "connector.h"
#include <cstdint>
#include <cstring>
#include <cassert>
namespace net
{

template <typename T>
class msg_builder
{
    enum class op_type
	{
		read_header,
		read_msg
	};

	struct operation
	{
		op_type type = op_type::read_header;
		size_t bytes = get_header_size();
	};


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

	void process_operation(byte_buffer&& buffer)
	{
        assert(buffer.size() == op_.bytes && "read was not completed properly");

		switch(op_.type)
		{
			case op_type::read_header:
			{
				// read header
				size_t msg_size = 0;
				std::memcpy(&msg_size, buffer.data(), buffer.size());
                buffer.clear();

                set_next_operation(op_type::read_msg, msg_size);
			}
			break;

			case op_type::read_msg:
			{
                msg_ = std::move(buffer);
				set_next_operation(op_type::read_header, get_header_size());
			}
			break;
		}
	}

	const operation& get_next_operation() const
	{
		return op_;
	}

    bool is_ready() const
    {
        return !msg_.empty();
    }

	byte_buffer&& extract_msg()
	{
		return std::move(msg_);
	}

private:

	void set_next_operation(op_type type, size_t size)
	{
		op_.type = type;
		op_.bytes = size;
	}

	byte_buffer msg_;
	operation op_;
};

} // namespace net
