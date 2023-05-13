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

namespace utils
{

template <class T>
inline void endian_swap(T* objp)
{
    auto memp = reinterpret_cast<std::uint8_t*>(objp);
    std::reverse(memp, memp + sizeof(T));
}

inline bool is_big_endian()
{
    union {
        uint32_t i;
        char c[sizeof(uint32_t)];
    } val = {0x01020304};

    return val.c[0] == 1;
}

inline bool is_little_endian()
{
    return !is_big_endian();
}

template <typename T>
inline size_t to_bytes(const T& data, uint8_t* dst)
{
    static_assert(std::is_trivially_copyable<T>::value, "not a trivially_copyable type");


    const auto begin = reinterpret_cast<const uint8_t*>(std::addressof(data));
    const auto sz = sizeof(T);
    std::memcpy(dst, begin, sz);

    return sz;
}

//vector specialization
template <typename T>
inline size_t to_bytes(const std::vector<T>& data, uint8_t* dst)
{
    static_assert(std::is_trivially_copyable<T>::value, "not a trivially_copyable type");

    const auto begin = data.data();
    const auto sz = data.size() * sizeof(T);
    std::memcpy(dst, begin, sz);

    return sz;
}

template <>
inline size_t to_bytes<std::string>(const std::string& data, uint8_t* dst)
{
    const auto begin = data.data();
    const auto sz = data.size() * sizeof(std::string::value_type);
    std::memcpy(dst, begin, sz);

    return sz;
}

template <typename T>
inline size_t from_bytes(T& data, const uint8_t* src)
{
    static_assert(std::is_trivially_copyable<T>::value, "not a trivially_copyable type");

    auto dst = reinterpret_cast<uint8_t*>(std::addressof(data));
    const auto sz = sizeof(T);
    std::memcpy(dst, src, sz);

    return sz;
}

template <typename T>
inline size_t from_bytes(std::vector<T>& data, const uint8_t* src)
{
    static_assert(std::is_trivially_copyable<T>::value, "not a trivially_copyable type");

    if (data.empty())
    {
        return 0;
    }

    auto dst = data.data();
    const auto sz = data.size() * sizeof(T);
    std::memcpy(dst, src, sz);

    return sz;
}
}

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
    /// Get is thrown error is critical for connection
    //-----------------------------------------------------------------------------
    virtual bool critical_error() const noexcept
    {
        return true;
    }

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
