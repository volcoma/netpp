#ifndef STRING_VIEW_HPP
#define STRING_VIEW_HPP

#include <algorithm>
#include <cassert>
#include <limits>
#include <stdexcept>
#include <string>

namespace hpp
{
#define HPP_CHECK_STRING_LEN(_String, _Len) assert(_String != 0 || _Len == 0)
#define HPP_REQUIRES_STRING_LEN(_String, _Len) HPP_CHECK_STRING_LEN(_String, _Len)

/**
 *  @tparam CharT  Type of character
 *  @tparam Traits  Traits for character type, defaults to
 *                   charTraits<CharT>.
 *
 *  A basic_string_view looks like this:
 *
 *  @code
 *    CharT*    str_
 *    size_t     en_
 *  @endcode
 */
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_string_view
{
public:
	// types
	using traits_type = Traits;
	using value_type = CharT;
	using pointer = const CharT*;
	using const_pointer = const CharT*;
	using reference = const CharT&;
	using const_reference = const CharT&;
	using const_iterator = const CharT*;
	using iterator = const_iterator;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	using reverse_iterator = const_reverse_iterator;
	using size_type = size_t;
	using difference_type = ptrdiff_t;
	static constexpr size_type npos = size_type(-1);

	// [string.view.cons], construct/copy

	constexpr basic_string_view() noexcept
		: len_{0}
		, str_{nullptr}
	{
	}

	constexpr basic_string_view(const basic_string_view&) noexcept = default;

    template<typename T, typename = std::enable_if_t<std::is_same<T, const CharT*>::value>>
    constexpr basic_string_view(T str) noexcept
        : len_{traits_type::length(str)}
        , str_{str}
    {
    }

    // From static arrays - if 0-terminated, remove 0 from the view
	template <size_t N>
	constexpr basic_string_view(const CharT (&str)[N])
		: len_{N-1}
		, str_{str}
	{
	}

	constexpr basic_string_view(const CharT* str, size_type len) noexcept
		: len_{len}
		, str_{str}
	{
	}

	template <typename Allocator>
	basic_string_view(const std::basic_string<CharT, Traits, Allocator>& str) noexcept
		: len_{str.length()}
		, str_{str.data()}
	{
	}

	constexpr basic_string_view& operator=(const basic_string_view&) noexcept = default;

	// [string.view.iterators], iterators

	constexpr const_iterator begin() const noexcept
	{
		return this->str_;
	}

	constexpr const_iterator end() const noexcept
	{
		return this->str_ + this->len_;
	}

	constexpr const_iterator cbegin() const noexcept
	{
		return this->str_;
	}

	constexpr const_iterator cend() const noexcept
	{
		return this->str_ + this->len_;
	}

	constexpr const_reverse_iterator rbegin() const noexcept
	{
		return const_reverse_iterator(this->end());
	}

	constexpr const_reverse_iterator rend() const noexcept
	{
		return const_reverse_iterator(this->begin());
	}

	constexpr const_reverse_iterator crbegin() const noexcept
	{
		return const_reverse_iterator(this->end());
	}

	constexpr const_reverse_iterator crend() const noexcept
	{
		return const_reverse_iterator(this->begin());
	}

	// [string.view.capacity], capacity

	constexpr size_type size() const noexcept
	{
		return this->len_;
	}

	constexpr size_type length() const noexcept
	{
		return len_;
	}

	constexpr size_type max_size() const noexcept
	{
		return (npos - sizeof(size_type) - sizeof(void*)) / sizeof(value_type) / 4;
	}

	constexpr bool empty() const noexcept
	{
		return this->len_ == 0;
	}

	// [string.view.access], element access

	constexpr const CharT& operator[](size_type pos) const noexcept
	{
		// TODO: Assert to restore in a way compatible with the constexpr.
		// assert(pos < this->len_);
		return *(this->str_ + pos);
	}

	constexpr const CharT& at(size_type pos) const
	{
		if(pos >= len_)
		{
			throw std::out_of_range("basic_string_view::at: pos >= this->size()");
		}

		return *(this->str_ + pos);
	}

	constexpr const CharT& front() const noexcept
	{
		// TODO: Assert to restore in a way compatible with the constexpr.
		// assert(this->len_ > 0);
		return *this->str_;
	}

	constexpr const CharT& back() const noexcept
	{
		// TODO: Assert to restore in a way compatible with the constexpr.
		// assert(this->len_ > 0);
		return *(this->str_ + this->len_ - 1);
	}

	constexpr const CharT* data() const noexcept
	{
		return this->str_;
	}

	// [string.view.modifiers], modifiers:

	constexpr void remove_prefix(size_type n) noexcept
	{
		aseert(this->len_ >= n);
		this->str_ += n;
		this->len_ -= n;
	}

	constexpr void remove_suffix(size_type n) noexcept
	{
		this->len_ -= n;
	}

	constexpr void swap(basic_string_view& sv) noexcept
	{
		auto tmp = *this;
		*this = sv;
		sv = tmp;
	}

	template <typename Allocator>
	explicit operator std::basic_string<CharT, Traits, Allocator>() const
	{
		return {this->str_, this->len_};
	}

	// [string.view.ops], string operations:

	size_type copy(CharT* str, size_type n, size_type pos = 0) const
	{
		HPP_REQUIRES_STRING_LEN(str, n);
		pos = check_impl(pos, "basic_string_view::copy");
		const size_type rlen = std::min(n, len_ - pos);
		for(auto begin = this->str_ + pos, end = begin + rlen; begin != end;)
			*str++ = *begin++;
		return rlen;
	}

	constexpr basic_string_view substr(size_type pos, size_type n = npos) const noexcept(false)
	{
		pos = check_impl(pos, "basic_string_view::substr");
		const size_type rlen = std::min(n, len_ - pos);
		return basic_string_view{str_ + pos, rlen};
	}

	constexpr int compare(basic_string_view str) const noexcept
	{
		const size_type rlen = std::min(this->len_, str.len_);
		int ret = traits_type::compare(this->str_, str.str_, rlen);
		if(ret == 0)
			ret = complare_impl(this->len_, str.len_);
		return ret;
	}

	constexpr int compare(size_type pos1, size_type n1, basic_string_view str) const
	{
		return this->substr(pos1, n1).compare(str);
	}

	constexpr int compare(size_type pos1, size_type n1, basic_string_view str, size_type pos2,
						  size_type n2) const
	{
		return this->substr(pos1, n1).compare(str.substr(pos2, n2));
	}

	constexpr int compare(const CharT* str) const noexcept
	{
		return this->compare(basic_string_view{str});
	}

	constexpr int compare(size_type pos1, size_type n1, const CharT* str) const
	{
		return this->substr(pos1, n1).compare(basic_string_view{str});
	}

	constexpr int compare(size_type pos1, size_type n1, const CharT* str, size_type n2) const noexcept(false)
	{
		return this->substr(pos1, n1).compare(basic_string_view(str, n2));
	}

	constexpr size_type find(basic_string_view str, size_type pos = 0) const noexcept
	{
		return this->find(str.str_, pos, str.len_);
	}

	constexpr size_type find(CharT c, size_type pos = 0) const noexcept;

	constexpr size_type find(const CharT* str, size_type pos, size_type n) const noexcept;

	constexpr size_type find(const CharT* str, size_type pos = 0) const noexcept
	{
		return this->find(str, pos, traits_type::length(str));
	}

	constexpr size_type rfind(basic_string_view str, size_type pos = npos) const noexcept
	{
		return this->rfind(str.str_, pos, str.len_);
	}

	constexpr size_type rfind(CharT c, size_type pos = npos) const noexcept;

	constexpr size_type rfind(const CharT* str, size_type pos, size_type n) const noexcept;

	constexpr size_type rfind(const CharT* str, size_type pos = npos) const noexcept
	{
		return this->rfind(str, pos, traits_type::length(str));
	}

	constexpr size_type find_first_of(basic_string_view str, size_type pos = 0) const noexcept
	{
		return this->find_first_of(str.str_, pos, str.len_);
	}

	constexpr size_type find_first_of(CharT c, size_type pos = 0) const noexcept
	{
		return this->find(c, pos);
	}

	constexpr size_type find_first_of(const CharT* str, size_type pos, size_type n) const noexcept;

	constexpr size_type find_first_of(const CharT* str, size_type pos = 0) const noexcept
	{
		return this->find_first_of(str, pos, traits_type::length(str));
	}

	constexpr size_type find_last_of(basic_string_view str, size_type pos = npos) const noexcept
	{
		return this->find_last_of(str.str_, pos, str.len_);
	}

	constexpr size_type find_last_of(CharT c, size_type pos = npos) const noexcept
	{
		return this->rfind(c, pos);
	}

	constexpr size_type find_last_of(const CharT* str, size_type pos, size_type n) const noexcept;

	constexpr size_type find_last_of(const CharT* str, size_type pos = npos) const noexcept
	{
		return this->find_last_of(str, pos, traits_type::length(str));
	}

	constexpr size_type find_first_not_of(basic_string_view str, size_type pos = 0) const noexcept
	{
		return this->find_first_not_of(str.str_, pos, str.len_);
	}

	constexpr size_type find_first_not_of(CharT c, size_type pos = 0) const noexcept;

	constexpr size_type find_first_not_of(const CharT* str, size_type pos, size_type n) const noexcept;

	constexpr size_type find_first_not_of(const CharT* str, size_type pos = 0) const noexcept
	{
		return this->find_first_not_of(str, pos, traits_type::length(str));
	}

	constexpr size_type find_last_not_of(basic_string_view str, size_type pos = npos) const noexcept
	{
		return this->find_last_not_of(str.str_, pos, str.len_);
	}

	constexpr size_type find_last_not_of(CharT c, size_type pos = npos) const noexcept;

	constexpr size_type find_last_not_of(const CharT* str, size_type pos, size_type n) const noexcept;

	constexpr size_type find_last_not_of(const CharT* str, size_type pos = npos) const noexcept
	{
		return this->find_last_not_of(str, pos, traits_type::length(str));
	}

	constexpr size_type check_impl(size_type pos, const char* s) const noexcept(false)
	{
		if(pos > this->size())
		{
			throw std::out_of_range(std::string(s) + ": pos  > this->size()");
		}
		return pos;
	}

private:
	static constexpr int complare_impl(size_type n1, size_type n2) noexcept
	{
		const difference_type diff = n1 - n2;
		if(diff > std::numeric_limits<int>::max())
			return std::numeric_limits<int>::max();
		if(diff < std::numeric_limits<int>::min())
			return std::numeric_limits<int>::min();
		return static_cast<int>(diff);
	}

	size_t len_;
	const CharT* str_;
};

// [string.view.comparison], non-member basic_string_view comparison function

namespace detail
{
// Identity transform to create a non-deduced context, so that only one
// argument participates in template argument deduction and the other
// argument gets implicitly converted to the deduced type. See n3766.html.
template <typename T>
using idt = std::common_type_t<T>;
}

template <typename CharT, typename Traits>
constexpr bool operator==(basic_string_view<CharT, Traits> lhs, basic_string_view<CharT, Traits> rhs) noexcept
{
	return lhs.size() == rhs.size() && lhs.compare(rhs) == 0;
}

template <typename CharT, typename Traits>
constexpr bool operator==(basic_string_view<CharT, Traits> lhs,
						  detail::idt<basic_string_view<CharT, Traits>> rhs) noexcept
{
	return lhs.size() == rhs.size() && lhs.compare(rhs) == 0;
}

template <typename CharT, typename Traits>
constexpr bool operator==(detail::idt<basic_string_view<CharT, Traits>> lhs,
						  basic_string_view<CharT, Traits> rhs) noexcept
{
	return lhs.size() == rhs.size() && lhs.compare(rhs) == 0;
}

template <typename CharT, typename Traits>
constexpr bool operator!=(basic_string_view<CharT, Traits> lhs, basic_string_view<CharT, Traits> rhs) noexcept
{
	return !(lhs == rhs);
}

template <typename CharT, typename Traits>
constexpr bool operator!=(basic_string_view<CharT, Traits> lhs,
						  detail::idt<basic_string_view<CharT, Traits>> rhs) noexcept
{
	return !(lhs == rhs);
}

template <typename CharT, typename Traits>
constexpr bool operator!=(detail::idt<basic_string_view<CharT, Traits>> lhs,
						  basic_string_view<CharT, Traits> rhs) noexcept
{
	return !(lhs == rhs);
}

template <typename CharT, typename Traits>
constexpr bool operator<(basic_string_view<CharT, Traits> lhs, basic_string_view<CharT, Traits> rhs) noexcept
{
	return lhs.compare(rhs) < 0;
}

template <typename CharT, typename Traits>
constexpr bool operator<(basic_string_view<CharT, Traits> lhs,
						 detail::idt<basic_string_view<CharT, Traits>> rhs) noexcept
{
	return lhs.compare(rhs) < 0;
}

template <typename CharT, typename Traits>
constexpr bool operator<(detail::idt<basic_string_view<CharT, Traits>> lhs,
						 basic_string_view<CharT, Traits> rhs) noexcept
{
	return lhs.compare(rhs) < 0;
}

template <typename CharT, typename Traits>
constexpr bool operator>(basic_string_view<CharT, Traits> lhs, basic_string_view<CharT, Traits> rhs) noexcept
{
	return lhs.compare(rhs) > 0;
}

template <typename CharT, typename Traits>
constexpr bool operator>(basic_string_view<CharT, Traits> lhs,
						 detail::idt<basic_string_view<CharT, Traits>> rhs) noexcept
{
	return lhs.compare(rhs) > 0;
}

template <typename CharT, typename Traits>
constexpr bool operator>(detail::idt<basic_string_view<CharT, Traits>> lhs,
						 basic_string_view<CharT, Traits> rhs) noexcept
{
	return lhs.compare(rhs) > 0;
}

template <typename CharT, typename Traits>
constexpr bool operator<=(basic_string_view<CharT, Traits> lhs, basic_string_view<CharT, Traits> rhs) noexcept
{
	return lhs.compare(rhs) <= 0;
}

template <typename CharT, typename Traits>
constexpr bool operator<=(basic_string_view<CharT, Traits> lhs,
						  detail::idt<basic_string_view<CharT, Traits>> rhs) noexcept
{
	return lhs.compare(rhs) <= 0;
}

template <typename CharT, typename Traits>
constexpr bool operator<=(detail::idt<basic_string_view<CharT, Traits>> lhs,
						  basic_string_view<CharT, Traits> rhs) noexcept
{
	return lhs.compare(rhs) <= 0;
}

template <typename CharT, typename Traits>
constexpr bool operator>=(basic_string_view<CharT, Traits> lhs, basic_string_view<CharT, Traits> rhs) noexcept
{
	return lhs.compare(rhs) >= 0;
}

template <typename CharT, typename Traits>
constexpr bool operator>=(basic_string_view<CharT, Traits> lhs,
						  detail::idt<basic_string_view<CharT, Traits>> rhs) noexcept
{
	return lhs.compare(rhs) >= 0;
}

template <typename CharT, typename Traits>
constexpr bool operator>=(detail::idt<basic_string_view<CharT, Traits>> lhs,
						  basic_string_view<CharT, Traits> rhs) noexcept
{
	return lhs.compare(rhs) >= 0;
}

// [string.view.io], Inserters and extractors
template <typename CharT, typename Traits>
inline std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
													 basic_string_view<CharT, Traits> str)
{
	return os.write(str.data(), str.size());
}
template <typename CharT, typename Traits>
constexpr typename basic_string_view<CharT, Traits>::size_type
basic_string_view<CharT, Traits>::find(const CharT* str, size_type pos, size_type n) const noexcept
{
	HPP_REQUIRES_STRING_LEN(str, n);

	if(n == 0)
		return pos <= this->len_ ? pos : npos;

	if(n <= this->len_)
	{
		for(; pos <= this->len_ - n; ++pos)
			if(traits_type::eq(this->str_[pos], str[0]) &&
			   traits_type::compare(this->str_ + pos + 1, str + 1, n - 1) == 0)
				return pos;
	}
	return npos;
}

template <typename CharT, typename Traits>
constexpr typename basic_string_view<CharT, Traits>::size_type
basic_string_view<CharT, Traits>::find(CharT c, size_type pos) const noexcept
{
	size_type ret = npos;
	if(pos < this->len_)
	{
		const size_type n = this->len_ - pos;
		const CharT* p = traits_type::find(this->str_ + pos, n, c);
		if(p)
			ret = p - this->str_;
	}
	return ret;
}

template <typename CharT, typename Traits>
constexpr typename basic_string_view<CharT, Traits>::size_type
basic_string_view<CharT, Traits>::rfind(const CharT* str, size_type pos, size_type n) const noexcept
{
	HPP_REQUIRES_STRING_LEN(str, n);

	if(n <= this->len_)
	{
		pos = std::min(size_type(this->len_ - n), pos);
		do
		{
			if(traits_type::compare(this->str_ + pos, str, n) == 0)
				return pos;
		} while(pos-- > 0);
	}
	return npos;
}

template <typename CharT, typename Traits>
constexpr typename basic_string_view<CharT, Traits>::size_type
basic_string_view<CharT, Traits>::rfind(CharT c, size_type pos) const noexcept
{
	size_type size = this->len_;
	if(size > 0)
	{
		if(--size > pos)
			size = pos;
		for(++size; size-- > 0;)
			if(traits_type::eq(this->str_[size], c))
				return size;
	}
	return npos;
}

template <typename CharT, typename Traits>
constexpr typename basic_string_view<CharT, Traits>::size_type
basic_string_view<CharT, Traits>::find_first_of(const CharT* str, size_type pos, size_type n) const noexcept
{
	HPP_REQUIRES_STRING_LEN(str, n);
	for(; n && pos < this->len_; ++pos)
	{
		const CharT* p = traits_type::find(str, n, this->str_[pos]);
		if(p)
			return pos;
	}
	return npos;
}

template <typename CharT, typename Traits>
constexpr typename basic_string_view<CharT, Traits>::size_type
basic_string_view<CharT, Traits>::find_last_of(const CharT* str, size_type pos, size_type n) const noexcept
{
	HPP_REQUIRES_STRING_LEN(str, n);
	size_type size = this->size();
	if(size && n)
	{
		if(--size > pos)
			size = pos;
		do
		{
			if(traits_type::find(str, n, this->str_[size]))
				return size;
		} while(size-- != 0);
	}
	return npos;
}

template <typename CharT, typename Traits>
constexpr typename basic_string_view<CharT, Traits>::size_type
basic_string_view<CharT, Traits>::find_first_not_of(const CharT* str, size_type pos, size_type n) const
	noexcept
{
	HPP_REQUIRES_STRING_LEN(str, n);
	for(; pos < this->len_; ++pos)
		if(!traits_type::find(str, n, this->str_[pos]))
			return pos;
	return npos;
}

template <typename CharT, typename Traits>
constexpr typename basic_string_view<CharT, Traits>::size_type
basic_string_view<CharT, Traits>::find_first_not_of(CharT c, size_type pos) const noexcept
{
	for(; pos < this->len_; ++pos)
		if(!traits_type::eq(this->str_[pos], c))
			return pos;
	return npos;
}

template <typename CharT, typename Traits>
constexpr typename basic_string_view<CharT, Traits>::size_type
basic_string_view<CharT, Traits>::find_last_not_of(const CharT* str, size_type pos, size_type n) const
	noexcept
{
	HPP_REQUIRES_STRING_LEN(str, n);
	size_type size = this->len_;
	if(size)
	{
		if(--size > pos)
			size = pos;
		do
		{
			if(!traits_type::find(str, n, this->str_[size]))
				return size;
		} while(size--);
	}
	return npos;
}

template <typename CharT, typename Traits>
constexpr typename basic_string_view<CharT, Traits>::size_type
basic_string_view<CharT, Traits>::find_last_not_of(CharT c, size_type pos) const noexcept
{
	size_type size = this->len_;
	if(size)
	{
		if(--size > pos)
			size = pos;
		do
		{
			if(!traits_type::eq(this->str_[size], c))
				return size;
		} while(size--);
	}
	return npos;
}

// basic_string_view typedef names

using string_view = basic_string_view<char>;
using wstring_view = basic_string_view<wchar_t>;
using u16string_view = basic_string_view<char16_t>;
using u32string_view = basic_string_view<char32_t>;

namespace impl
{

/* implementations of MurmurHash2 *Endian Neutral* (but not alignment!) */
template <::std::size_t = sizeof(::std::size_t)>
struct murmur;
template <>
struct murmur<4>
{
	constexpr murmur() = default;

	::std::uint32_t operator()(void const* p, ::std::size_t len) const noexcept
	{
		static constexpr ::std::uint32_t magic = UINT32_C(0x5BD1E995);
		static constexpr auto shift = 24;

		auto hash = static_cast<::std::uint32_t>(len);
		auto data = static_cast<::std::uint8_t const*>(p);

		while(len >= sizeof(::std::uint32_t))
		{
			::std::uint32_t mix = data[0];
			mix |= ::std::uint32_t(data[1]) << 8;
			mix |= ::std::uint32_t(data[2]) << 16;
			mix |= ::std::uint32_t(data[3]) << 24;

			mix *= magic;
			mix ^= mix >> shift;
			mix *= magic;

			hash *= magic;
			hash ^= mix;

			data += sizeof(::std::uint32_t);
			len -= sizeof(::std::uint32_t);
		}

		switch(len)
		{
			case 3:
				hash ^= ::std::uint32_t(data[2]) << 16;
			case 2:
				hash ^= ::std::uint32_t(data[1]) << 8;
			case 1:
				hash ^= ::std::uint32_t(data[0]);
				hash *= magic;
		}

		hash ^= hash >> 13;
		hash *= magic;
		hash ^= hash >> 15;

		return hash;
	}
};

template <>
struct murmur<8>
{
	constexpr murmur() = default;

	::std::uint64_t operator()(void const* p, ::std::size_t len) const noexcept
	{
		static constexpr ::std::uint64_t magic = UINT64_C(0xC6A4A7935BD1E995);
		static constexpr auto shift = 47;

		::std::uint64_t hash = len * magic;

		auto data = static_cast<::std::uint8_t const*>(p);

		while(len >= sizeof(::std::uint64_t))
		{
			::std::uint64_t mix = data[0];
			mix |= ::std::uint64_t(data[1]) << 8;
			mix |= ::std::uint64_t(data[2]) << 16;
			mix |= ::std::uint64_t(data[3]) << 24;
			mix |= ::std::uint64_t(data[4]) << 32;
			mix |= ::std::uint64_t(data[5]) << 40;
			mix |= ::std::uint64_t(data[6]) << 48;
			mix |= ::std::uint64_t(data[7]) << 54;

			mix *= magic;
			mix ^= mix >> shift;
			mix *= magic;

			hash ^= mix;
			hash *= magic;

			data += sizeof(::std::uint64_t);
			len -= sizeof(::std::uint64_t);
		}

		switch(len & 7)
		{
			case 7:
				hash ^= ::std::uint64_t(data[6]) << 48;
			case 6:
				hash ^= ::std::uint64_t(data[5]) << 40;
			case 5:
				hash ^= ::std::uint64_t(data[4]) << 32;
			case 4:
				hash ^= ::std::uint64_t(data[3]) << 24;
			case 3:
				hash ^= ::std::uint64_t(data[2]) << 16;
			case 2:
				hash ^= ::std::uint64_t(data[1]) << 8;
			case 1:
				hash ^= ::std::uint64_t(data[0]);
				hash *= magic;
		}

		hash ^= hash >> shift;
		hash *= magic;
		hash ^= hash >> shift;

		return hash;
	}
};

} /* namespace hpp::impl */

} // namespace hpp

namespace std
{
// [string.view.hash], hash support:
template <typename CharT, typename Traits>
struct hash<hpp::basic_string_view<CharT, Traits>>
{
	using argument_type = hpp::basic_string_view<CharT, Traits>;
	using result_type = size_t;

	result_type operator()(argument_type const& ref) const noexcept
	{
		static constexpr hpp::impl::murmur<sizeof(size_t)> hasher{};
		return hasher(ref.data(), ref.size());
	}
};
} // namespace std

#endif // STRING_VIEW_HPP
