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
	using creator = std::function<std::unique_ptr<msg_builder>()>;

	virtual ~msg_builder() = default;

    enum class op_type
    {
        read_bytes,
        peek_bytes
    };

    struct operation
    {
        size_t bytes = 0;
        op_type type = op_type::read_bytes;
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
	virtual std::pair<byte_buffer, data_channel> extract_msg() = 0;

	//-----------------------------------------------------------------------------
	/// Create a creator of any derived type.
	//-----------------------------------------------------------------------------
	template <typename T>
	static creator get_creator()
	{
		static_assert(std::is_base_of<msg_builder, T>::value, "Only derived classes"
															  "can be created this way.");
		return []() { return std::make_unique<T>(); };
	}
};

using msg_builder_ptr = std::unique_ptr<msg_builder>;

} // namespace net
