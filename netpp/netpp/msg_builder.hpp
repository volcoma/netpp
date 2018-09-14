#pragma once
#include <cstdint>
#include <cstring>
#include <cassert>
#include <memory>
#include <algorithm>

namespace net
{
using byte_buffer = std::string; // std::vector<uint8_t>;

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
		op_type type = op_type::read_header;
		size_t bytes = 0;
	};

    virtual void build(byte_buffer& msg) const = 0;

	virtual bool process_operation(size_t size) = 0;

	virtual const operation& get_next_operation() const = 0;

	virtual byte_buffer&& extract_msg() = 0;

    virtual byte_buffer& get_work_buffer() = 0;

};


class standard_builder : public msg_builder
{
    template<typename T>
    static uint8_t* encode(uint8_t* raw, T data)
    {
        for (size_t i = 0; i < sizeof(T); i++)
        {
            raw[i] = (data >> (i * 8)) & 255;
        }
        return raw + sizeof(T);
    }
public:
    standard_builder()
    {
        op_.type = op_type::read_header;
        op_.bytes = get_header_size();
    }

    static size_t get_header_size()
    {
        return sizeof(uint32_t);
    }

	void build(byte_buffer& msg) const final
	{
        auto header_size = get_header_size();

        //uint32 forced
        auto payload_size = uint32_t(msg.size());

        msg.resize(payload_size + header_size, 0);
        size_t copy_sz = std::min(header_size, size_t(payload_size));
        size_t offset = header_size + payload_size - copy_sz;
        for (size_t i = 0; i < copy_sz; ++i)
        {
            std::swap(msg[offset + i], msg[i]);
        }

        for (size_t i = 0; i < header_size; ++i)
        {
            msg[i] = (byte_buffer::value_type(payload_size) >> (i * 8));
        }

	}

	bool process_operation(size_t size) final
	{
        assert(size == op_.bytes && "read was not completed properly");

        bool ready = false;
		switch(op_.type)
		{
			case op_type::read_header:
			{
				// read header
				size_t msg_size = 0;
				std::memcpy(&msg_size, msg_.data(), msg_.size());

                set_next_operation(op_type::read_msg, msg_size);
			}
			break;

			case op_type::read_msg:
			{
                auto header_size = get_header_size();
                auto payload_size = size;

                size_t copy_sz = std::min(header_size, payload_size);
                size_t offset = header_size + payload_size - copy_sz;

                for (size_t i = 0; i < copy_sz; i++)
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

	const operation& get_next_operation() const final
	{
		return op_;
	}

	byte_buffer&& extract_msg() final
	{
		return std::move(msg_);
	}

    byte_buffer& get_work_buffer() final
	{
		return msg_;
	}
private:

	void set_next_operation(op_type type, size_t size)
	{
		op_.type = type;
		op_.bytes = size;
	}

    byte_buffer header_;
	byte_buffer msg_;
	operation op_;
};

using msg_builder_ptr = std::unique_ptr<msg_builder>;

} // namespace net
