#include "msg_builder.h"
#include <algorithm>
#include <iomanip>
#include <iterator>
#include <type_traits>

namespace net
{
namespace utils
{

template <class T>
static void endian_swap(T* objp)
{
	auto memp = reinterpret_cast<std::uint8_t*>(objp);
	std::reverse(memp, memp + sizeof(T));
}
bool is_big_endian()
{
	union {
		uint32_t i;
		char c[sizeof(uint32_t)];
	} val = {0x01020304};

	return val.c[0] == 1;
}

bool is_little_endian()
{
	return !is_big_endian();
}

template <typename T>
size_t to_bytes(T data, uint8_t* dst)
{
	endian_swap(&data);
	const auto begin = reinterpret_cast<const uint8_t*>(std::addressof(data));

	const auto sz = sizeof(T);
	std::memcpy(dst, begin, sz);

	return sz;
}

template <typename T>
size_t from_bytes(T& data, const uint8_t* src)
{
	static_assert(std::is_trivially_copyable<T>::value, "not a trivially_copyable type");
	auto dst = reinterpret_cast<uint8_t*>(std::addressof(data));
	const auto sz = sizeof(T);
	std::memcpy(dst, src, sz);
	endian_swap(&data);
	return sz;
}
}

multi_buffer_builder::multi_buffer_builder()
{
	op_.type = op_type::read_header;
	op_.bytes = get_header_size();
}

size_t multi_buffer_builder::get_header_size()
{
	return sizeof(uint32_t);
}

std::vector<byte_buffer> multi_buffer_builder::build(byte_buffer&& msg, data_channel channel) const
{
	std::vector<byte_buffer> buffers;
	buffers.reserve(2);
	auto header_size = get_header_size();
	// uint32 forced
	auto payload_size = uint32_t(msg.size());
	buffers.emplace_back(header_size);
	auto& buffer = buffers.back();
	size_t offset = 0;
	utils::to_bytes(payload_size, buffer.data() + offset);
	// utils::to_bytes(ch, buffer.data() + offset);

	buffers.emplace_back(std::move(msg));
	return buffers;
}

bool multi_buffer_builder::process_operation(size_t size)
{
	assert(size == op_.bytes && "read was not completed properly");

	bool ready = false;
	switch(op_.type)
	{
		case op_type::read_header:
		{
			// read header
			uint32_t msg_size = 0;
			utils::from_bytes(msg_size, msg_.data());
			msg_.clear();
			set_next_operation(op_type::read_msg, msg_size);
		}
		break;

		case op_type::read_msg:
		{
			ready = true;
			set_next_operation(op_type::read_header, get_header_size());
		}
		break;
	}

	return ready;
}

const msg_builder::operation& multi_buffer_builder::get_next_operation() const
{
	return op_;
}

byte_buffer&& multi_buffer_builder::extract_msg()
{
	return std::move(msg_);
}

byte_buffer& multi_buffer_builder::get_work_buffer()
{
	return msg_;
}

void multi_buffer_builder::set_next_operation(msg_builder::op_type type, size_t size)
{
	op_.type = type;
	op_.bytes = size;
}

single_buffer_builder::single_buffer_builder()
{
	op_.type = op_type::read_header;
	op_.bytes = get_header_size();
}

size_t single_buffer_builder::get_header_size()
{
	return sizeof(uint32_t);
}

std::vector<byte_buffer> single_buffer_builder::build(byte_buffer&& msg, data_channel channel) const
{
	auto header_size = get_header_size();
	// uint32 forced
	auto payload_size = uint32_t(msg.size());
	msg.resize(payload_size + header_size, 0);
	size_t copy_sz = std::min(header_size, size_t(payload_size));
	size_t offset = header_size + payload_size - copy_sz;
	for(size_t i = 0; i < copy_sz; ++i)
	{
		std::swap(msg[offset + i], msg[i]);
	}

	utils::to_bytes(payload_size, msg.data());
	return {std::move(msg)};
}

bool single_buffer_builder::process_operation(size_t size)
{
	assert(size == op_.bytes && "read was not completed properly");

	bool ready = false;
	switch(op_.type)
	{
		case op_type::read_header:
		{
			// read header
			uint32_t msg_size = 0;
			utils::from_bytes(msg_size, msg_.data());
			set_next_operation(op_type::read_msg, msg_size);
		}
		break;

		case op_type::read_msg:
		{
			auto header_size = get_header_size();
			auto payload_size = size;

			size_t copy_sz = std::min(header_size, payload_size);
			size_t offset = header_size + payload_size - copy_sz;

			for(size_t i = 0; i < copy_sz; i++)
			{
				std::swap(msg_[i], msg_[offset + i]);
			}
			msg_.resize(payload_size);
			ready = true;
			set_next_operation(op_type::read_header, get_header_size());
		}
		break;
	}

	return ready;
}

const msg_builder::operation& single_buffer_builder::get_next_operation() const
{
	return op_;
}

byte_buffer&& single_buffer_builder::extract_msg()
{
	return std::move(msg_);
}

byte_buffer& single_buffer_builder::get_work_buffer()
{
	return msg_;
}

void single_buffer_builder::set_next_operation(msg_builder::op_type type, size_t size)
{
	op_.type = type;
	op_.bytes = size;
}

} // namespace net
