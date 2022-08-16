#ifndef BASIC_SERIALIZATION_HPP
#define BASIC_SERIALIZATION_HPP

#include <cassert>
#include <string>
#include <tuple>
#include <vector>

/// template to serialize basic types using bitsery
template <typename S, typename basic_t>
void serialize_basic_type(S& s, basic_t &v) {
	if constexpr (1 == sizeof(basic_t)) {
		s.value1b(v);
	} else if constexpr (2 == sizeof(basic_t)) {
		s.value2b(v);
	} else if constexpr (4 == sizeof(basic_t)) {
		s.value4b(v);
	} else if constexpr (8 == sizeof(basic_t)) {
		s.value8b(v);
	}
	#if DEBUG_ASSERTIONS
	else {
		assert(false);
	}
	#endif	

	static_assert(
		(1 == sizeof(basic_t)) ||
		(2 == sizeof(basic_t)) ||
		(4 == sizeof(basic_t)) ||
		(8 == sizeof(basic_t)) );

	// std::cout << "basic type of size " << sizeof(basic_t) << ":" << v << std::endl;
}


/// serialize a text marker and check on deserialization for it
template<typename S>
void serialize_marker(
	S &s, const std::string marker, const std::size_t length)
{
	// length must contain null byte!
	std::string marker_ = marker;
	s.text1b(marker_, length);
	assert(marker == marker_);
}


/// serialize a vector of data type with size of one byte except bool
template<typename S, typename data_t> requires
((1 == sizeof(data_t)) && (false == std::is_same<bool, data_t>::value))
void serialize(S &s, std::vector<data_t> &v)
{
	std::size_t length = v.size();
	serialize_basic_type<S, std::size_t>(s, length);
	v.resize(length);
	s.container1b(v, length);
	assert(v.size() == length);
}


/// generic serialization (recursive base condition)
template<typename S>
void generic_serialize(S &s) { ignore(s); }

/// declaration of generic serialization (recursive)
template<typename S, typename first_t, typename... arg_types>
void generic_serialize(S &s, first_t &a, arg_types &... args);

/// serialization of std::pair
template<typename S, typename first_t, typename second_t>
void serialize(S &s, std::pair<first_t, second_t> &v) {
	generic_serialize(s, v.first, v.second);
}

/// generic serialization of arbitrary parameters (recursive)
template<typename S, typename first_t, typename... arg_types>
void generic_serialize(S &s, first_t &a, arg_types &... args)
{
	// serialize first argument
	if constexpr (std::is_fundamental<first_t>::value) {
		serialize_basic_type<S, first_t>(s, a);
	} else if constexpr (std::is_same<std::string, first_t>::value) {
		auto length = a.size();
		generic_serialize(s, length);
		a.reserve(length);
		s.text1b(a, length);
		assert(a.size() == length);
	} else if constexpr (std::is_same<const char *, first_t>::value) {
		// one way trap for serialization of const char *
		// note: when used with remote procedure calls
		// the function parameter must be std::string,
		// but the argument passed may const char *
		std::string tmp_str = a;
		generic_serialize(s, tmp_str);
	} else {
		serialize(s, a);
	}

	// recursively pass further arguments
	generic_serialize(s, args...);
}


/// serialize empty tuple
template<typename S>
void serialize(S &s, std::tuple<> &v) {
	ignore(s, v);
}

/*
/// serialize tuple recursively
template<typename S, typename head_arg_type, typename... tail_arg_types>
void serialize(S &s, std::tuple<head_arg_type, tail_arg_types...> &v) {
	head_arg_type tmp = std::get<0>(v);
	generic_serialize<S, head_arg_type>(s, tmp);
	if constexpr (0 < std::tuple_size<std::tuple<tail_arg_types...>>::value)
	{
		auto t = tail(v);
		serialize<S, tail_arg_types...>(s, t);
		v = std::tuple_cat(std::make_tuple(tmp), t);
	} else {
		v = std::make_tuple(tmp);
	}
}
*/

/// serialize non empty tuple
template<typename S, typename... arg_types>
void serialize(S &s, std::tuple<arg_types...> &t) {
	apply([&s](auto &... args) { generic_serialize<S, arg_types...>(s, args...); }, t);
}


/// wrapper code for generic serialization
template<typename t>
struct make_generic_serializeable {
	t data;
};

/// generic serialization call for wrapper
template<typename S, typename t>
void serialize(S &s, make_generic_serializeable<t> &o) {
	generic_serialize(s, o.data);
}


/*
#include "util.hpp"

/// serialize std::vector<bool> using a string representation
template<typename S>
void serialize(S &s, std::vector<bool> &v) {
	auto content = vector_to_string(v);
	std::cout << "content length before: " << content.length() << std::endl;
	// ignore(content);
	generic_serialize<S, std::string>(s, content);
	std::cout << "content length after: " << content.length() << std::endl;
	v = vector_from_string(content);
}
*/

/// binary serialization of std::vector<bool>
template<typename S>
void serialize(S &s, std::vector<bool> &v) {
	std::size_t length = v.size();
	serialize_basic_type<S, std::size_t>(s, length);
	v.resize(length);

	for (std::size_t i = 0; i < length; i++) {
		uint8_t value = v[i] ? 1 : 0;
		s.value1b(value);
		v[i] = (0 != value);
	}

	// s.container1b(v, length);
	assert(v.size() == length);
}

#endif
