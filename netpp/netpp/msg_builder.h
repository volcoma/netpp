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
		read_header,
		read_msg
	};

	struct operation
	{
		size_t bytes = 0;
		op_type type = op_type::read_header;
	};

	virtual std::vector<byte_buffer> build(byte_buffer&& msg, data_channel channel = 0) const = 0;
	virtual bool process_operation(size_t size) = 0;
	virtual const operation& get_next_operation() const = 0;
	virtual byte_buffer&& extract_msg() = 0;
	virtual byte_buffer& get_work_buffer() = 0;
};

using msg_builder_ptr = std::unique_ptr<msg_builder>;

class multi_buffer_builder : public msg_builder
{
public:
	multi_buffer_builder();

	static size_t get_header_size();

	std::vector<byte_buffer> build(byte_buffer&& msg, data_channel channel) const final;

	bool process_operation(size_t size) final;

	const operation& get_next_operation() const final;

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
	single_buffer_builder();

	static size_t get_header_size();

	std::vector<byte_buffer> build(byte_buffer&& msg, data_channel channel) const final;

	bool process_operation(size_t size) final;

	const operation& get_next_operation() const final;

	byte_buffer&& extract_msg() final;

	byte_buffer& get_work_buffer() final;

private:
	void set_next_operation(op_type type, size_t size);

	byte_buffer msg_;
	operation op_;
};

} // namespace net
