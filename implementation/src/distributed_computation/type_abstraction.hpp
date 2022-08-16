#ifndef TYPE_ABSTRACTION_HPP
#define TYPE_ABSTRACTION_HPP

#include <any>

#include "../util/include_bitsery.hpp"

/// data type abstraction using std::any (for testing)
class Any_TypeAbstractionAdapter
{
	public:

		/// member used for static compile time polymorphism
		static const bool is_TypeAbstractionAdapter = true;
		
		/// abstract type
		using abstract_type = std::any;

		/// serialization
		template<typename t>
		static abstract_type serialize(t &v) {
			return v;
		}

		/// deserialization
		template<typename t>
		static bool deserialize(const abstract_type &v, t &result) {
			try {
				result = std::any_cast<t>(v);
				return true;
			} catch (...) {
				return false;
			}
		}
};

/// data type abstraction using bitsery providing (de-)serialization
template<
	typename buffer_t,
	typename input_adapter_t,
	typename output_adapter_t
>
class Bitsery_TypeAbstractionAdapter
{
	public:

		/// member used for static compile time polymorphism
		static const bool is_TypeAbstractionAdapter = true;

		/// abstract type
		using abstract_type = buffer_t;

		/// serialization
		template<typename t>
		static abstract_type serialize(t &v)
		{
			buffer_t buffer;
			auto length = bitsery::quickSerialization(
				std::move(output_adapter_t{buffer}), v);
			buffer.resize(length);
			return buffer;
		}

		/// deserialization
		template<typename t>
		static bool deserialize(const abstract_type &buffer, t &result)
		{
			auto state = bitsery::quickDeserialization(
				std::move(input_adapter_t{buffer.begin(), buffer.size()}),
				result);

			if ((state.first != bitsery::ReaderError::NoError)
				|| (false == state.second))
			{
				return false;
			}

			return true;
		}
};

#endif

