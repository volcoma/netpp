#pragma once
#include "logging.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

namespace net
{
using byte_buffer = std::vector<uint8_t>;
using data_channel = uint64_t;

struct msg_builder
{
	virtual ~msg_builder() = default;

	enum class op_type
	{
		read_header_size,
		read_header,
		read_msg
	};

	struct operation
	{
		size_t bytes = 0;
		op_type type = op_type::read_header_size;
	};

	//-----------------------------------------------------------------------------
	/// Builds a message provided payload and channel.
	/// This function is responsible to properly format
	/// the message e.g (a header/payload approach or a completely custom format).
	//-----------------------------------------------------------------------------
	virtual std::vector<byte_buffer> build(byte_buffer&& msg, data_channel channel = 0) const = 0;

	//-----------------------------------------------------------------------------
	/// Set processed bytes count.
	/// Returns whether the message is ready to be extracted.
	//-----------------------------------------------------------------------------
	virtual bool process_operation(size_t size) = 0;

	//-----------------------------------------------------------------------------
	/// Gets the next operation to be performed.
	/// Typically the user will make some preparations based on this information.
	/// e.g The user may resize the work buffer or peek this much bytes.
	//-----------------------------------------------------------------------------
	virtual operation get_next_operation() const = 0;

	//-----------------------------------------------------------------------------
	/// Gets a reference to an internal work buffer.
	//-----------------------------------------------------------------------------
	virtual byte_buffer& get_work_buffer() = 0;

	//-----------------------------------------------------------------------------
	/// Extracts the ready message.
	//-----------------------------------------------------------------------------
	virtual byte_buffer&& extract_msg() = 0;
};

using msg_builder_ptr = std::unique_ptr<msg_builder>;

class multi_buffer_builder : public msg_builder
{
public:
	using header_size_t = uint8_t;
	using payload_size_t = uint32_t;
	using channel_t = uint64_t;
	using id_t = uint16_t;

	multi_buffer_builder();

	static size_t get_header_size();
	static size_t get_total_header_size();

	std::vector<byte_buffer> build(byte_buffer&& msg, data_channel channel) const final;

	bool process_operation(size_t size) final;

	operation get_next_operation() const final;

	byte_buffer&& extract_msg() final;

	byte_buffer& get_work_buffer() final;

private:
	void set_next_operation(op_type type, size_t size);

	byte_buffer msg_;
	operation op_;
};

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

	byte_buffer&& extract_msg() final;

	byte_buffer& get_work_buffer() final;

private:
	void set_next_operation(op_type type, size_t size);

	byte_buffer msg_;
	operation op_;
};

} // namespace net
