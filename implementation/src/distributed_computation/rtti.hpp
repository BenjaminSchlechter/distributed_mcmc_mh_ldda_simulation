#ifndef RTTI_HPP
#define RTTI_HPP

#include <tuple>

#include "../util/util.hpp"


/// base class to represent tuples of arbitrary types
template<typename type_abstraction_adapter_t> requires
	(type_abstraction_adapter_t::is_TypeAbstractionAdapter)
class GenericType
{
	public:
		/// serialization to type abstraction
		virtual type_abstraction_adapter_t::abstract_type serialize
			(type_abstraction_adapter_t &adapter) const
		{
			ignore(adapter);
			return typename type_abstraction_adapter_t::abstract_type();
		}

		/// deserialization from type abstraction
		virtual bool deserialize(
			type_abstraction_adapter_t &adapter,
			const type_abstraction_adapter_t::abstract_type &v)
		{
			ignore(adapter, v);
			return false;
		}

		/// virtual destructor
		virtual ~GenericType() {}
};

/// a concrete tuple of arbitrary types
template<typename type_abstraction_adapter_t, typename... arg_types>
	requires (type_abstraction_adapter_t::is_TypeAbstractionAdapter)
class ConcreteType : public GenericType<type_abstraction_adapter_t>
{
	public:
		/// tuple data type
		using tuple_t = std::tuple<arg_types ...>;

		/// tuple with values
		tuple_t values;

		/// default constructor
		ConcreteType() : values() {}

		/// constructor variant with additional value to resolve ambiguous calls
		ConcreteType(
			arg_types... args,
			bool use_value_constructor = true
		) :
			values(args...)
		{
			ignore(use_value_constructor);
		}

		/// normal constructor passing values
		ConcreteType(std::tuple<arg_types ...> &values) : values(values) {}

		/// copy constructor
		ConcreteType(const ConcreteType &other) : values(other.values) {}

		/// type conversion to differen type abstraction adapters
		template<typename other_type_abstraction_adapter_t>
		operator ConcreteType<
			other_type_abstraction_adapter_t, arg_types...>() const
		{
			return ConcreteType<
				other_type_abstraction_adapter_t, arg_types...>(values);
		}

		/// serialization of values using the type abstraction adapter
		virtual type_abstraction_adapter_t::abstract_type
			serialize(type_abstraction_adapter_t &adapter) const override
		{
			return adapter.serialize(values);
		}

		/// deserialization of values using the type abstraction adapter
		virtual bool deserialize(
			type_abstraction_adapter_t &adapter,
			const type_abstraction_adapter_t::abstract_type &v) override
		{
			return adapter.deserialize(v, values);
		}
};


#endif


