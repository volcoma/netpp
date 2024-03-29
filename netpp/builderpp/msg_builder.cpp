#include "msg_builder.h"

#include <algorithm>
#include <iomanip>
#include <iterator>
#include <type_traits>

namespace net
{

single_buffer_builder::single_buffer_builder()
{
	op_.type = op_type::read_bytes;
	op_.bytes = sizeof(header_size_t);
	state_ = state::read_header_size;
}

size_t single_buffer_builder::get_header_size()
{
	return sizeof(header_size_t) + sizeof(payload_size_t) + sizeof(channel_t) + sizeof(id_t);
}

std::vector<byte_buffer> single_buffer_builder::build(byte_buffer&& msg, data_channel channel) const
{
	id_t id = 0;
	std::vector<byte_buffer> buffers;
	auto header_size = get_header_size();
	auto payload_size = payload_size_t(msg.size());
	buffers.emplace_back(header_size + payload_size);
	auto& buffer = buffers.back();
	size_t offset = 0;
	offset += utils::to_bytes(header_size_t(header_size), buffer.data());
	offset += utils::to_bytes(payload_size_t(payload_size), buffer.data() + offset);
	offset += utils::to_bytes(channel_t(channel), buffer.data() + offset);
	offset += utils::to_bytes(id_t(id), buffer.data() + offset);
	std::memcpy(buffer.data() + offset, msg.data(), msg.size());

	return buffers;
}

bool single_buffer_builder::process_operation(size_t size)
{
	if(size != op_.bytes)
	{
		throw std::runtime_error("Read was not completed properly");
	}

	bool ready = false;
	switch(state_)
	{
		case state::read_header_size:
		{
			// read header size
			header_size_t header_size = 0;
			utils::from_bytes(header_size, msg_.data());
			if(header_size != get_header_size())
			{
				throw std::runtime_error("Invalid header format");
			}

			msg_.clear();
			set_next_operation(op_type::read_bytes, header_size - sizeof(header_size_t), state::read_header);
		}
		break;
		case state::read_header:
		{
			// read header
			payload_size_t payload_size = 0;
			channel_t channel = 0;
			id_t id = 0;
			size_t offset = 0;
			offset += utils::from_bytes(payload_size, msg_.data());
			offset += utils::from_bytes(channel, msg_.data() + offset);
			offset += utils::from_bytes(id, msg_.data() + offset);
			(void)offset;
			channel_ = channel;
			msg_.clear();
			set_next_operation(op_type::read_bytes, payload_size, state::read_payload);
		}
		break;

		case state::read_payload:
		{
			ready = true;
			set_next_operation(op_type::read_bytes, sizeof(header_size_t), state::read_header_size);
		}
		break;
	}

	return ready;
}

msg_builder::operation single_buffer_builder::get_next_operation() const
{
	return op_;
}

std::pair<byte_buffer, data_channel> single_buffer_builder::extract_msg()
{
	return {std::move(msg_), channel_};
}

byte_buffer& single_buffer_builder::get_work_buffer()
{
	return msg_;
}

void single_buffer_builder::set_next_operation(msg_builder::op_type type, size_t size, state st)
{
	op_.type = type;
	op_.bytes = size;
	state_ = st;
}

} // namespace net
