#pragma once
#include <netpp/msg_builder.h>

namespace net
{

// Format
// header 15 bytes
// 1 byte = total size of the header including this 1 byte
// 4 bytes = size of the payload(the actual message).
// 8 bytes = data channel.
// 2 bytes = id
// n bytes = payload

class single_buffer_builder : public msg_builder
{
public:
	using header_size_t = uint8_t;
	using payload_size_t = uint32_t;
	using channel_t = uint64_t;
	using id_t = uint16_t;
	single_buffer_builder();

	static size_t get_header_size();
	static size_t get_total_header_size();

	std::vector<byte_buffer> build(byte_buffer&& msg, data_channel channel) const final;

	bool process_operation(size_t size) final;

	operation get_next_operation() const final;

	std::pair<byte_buffer, data_channel> extract_msg() final;

	byte_buffer& get_work_buffer() final;

private:
	void set_next_operation(op_type type, size_t size);

	byte_buffer msg_;
	channel_t channel_ = 0;
	operation op_;
};

} // namespace net
