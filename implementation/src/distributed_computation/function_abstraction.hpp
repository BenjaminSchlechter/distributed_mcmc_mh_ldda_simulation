#ifndef FUNCTION_ABSTRACTION_HPP
#define FUNCTION_ABSTRACTION_HPP

#include <functional>

#include "rtti.hpp"

/// helper struct to get type of function parameters and return value
template <class type_abstraction_adapter_t>
	requires (type_abstraction_adapter_t::is_TypeAbstractionAdapter)
struct function_typeinfo
{
	// functions without arguments

	/// get function argument type of function pointer without parameters
	template <typename return_type>
	static ConcreteType<type_abstraction_adapter_t>
		get_concrete_argument_type(return_type(*)())
	{
		return ConcreteType<type_abstraction_adapter_t>(true);
	}

	/// get function argument type of function object without parameters
	template <typename fobject, typename return_type>
	static ConcreteType<type_abstraction_adapter_t>
		get_concrete_argument_type(return_type(fobject::*)())
	{
		return ConcreteType<type_abstraction_adapter_t>(true);
	}

	/// get function argument type of std::function without parameters
	template <typename return_type>
	static ConcreteType<type_abstraction_adapter_t>
		get_concrete_argument_type(std::function<return_type()> f)
	{
		ignore(f);
		return ConcreteType<type_abstraction_adapter_t>(true);
	}


	/// helper function for recursive argument reduction
	template<typename first_arg_type, typename... tuple_tail_types>
	static auto create_concrete_type(
		first_arg_type *first_arg,
		std::tuple<tuple_tail_types...> &tail)
	{
		ignore(first_arg, tail);
		return ConcreteType<
			type_abstraction_adapter_t,
			first_arg_type,
			tuple_tail_types...>();
	}


	/// recursive argument reduction to get argument type (fct pointer)
	template <
		typename return_type,
		typename first_arg_type,
		typename... arg_types>
	static auto get_concrete_argument_type(
		return_type(*)(first_arg_type, arg_types...))
	{
		first_arg_type first_arg;
		std::function<return_type(arg_types...)> tail_func;
		typename decltype(
			get_concrete_argument_type(tail_func)
		)::tuple_t tail;
		return create_concrete_type(&first_arg, tail);
	}

	/// recursive argument reduction to get argument type (fct object)
	template <
		typename fobject,
		typename return_type,
		typename first_arg_type,
		typename... arg_types>
	static auto get_concrete_argument_type(
		return_type(fobject::*)(first_arg_type, arg_types...))
	{
		first_arg_type first_arg;
		std::function<return_type(arg_types...)> tail_func;
		typename decltype(
			get_concrete_argument_type(tail_func)
		)::tuple_t tail;
		return create_concrete_type(&first_arg, tail);
	}

	/// recursive argument reduction to get argument type (std::function)
	template <
		typename return_type,
		typename first_arg_type,
		typename... arg_types>
	static auto get_concrete_argument_type(
		std::function<return_type(first_arg_type, arg_types...)> f)
	{
		ignore(f);
		first_arg_type first_arg;
		std::function<return_type(arg_types...)> tail_func;
		typename decltype(
			get_concrete_argument_type(tail_func)
		)::tuple_t tail;
		return create_concrete_type(&first_arg, tail);
	}

	/// recursive argument reduction (const reference) to get argument type (fct pointer)
	template <
		typename return_type,
		typename first_arg_type,
		typename... arg_types>
	static auto get_concrete_argument_type(
		return_type(*)(const first_arg_type &, arg_types...))
	{
		first_arg_type first_arg;
		std::function<return_type(arg_types...)> tail_func;
		typename decltype(
			get_concrete_argument_type(tail_func)
		)::tuple_t tail;
		return create_concrete_type(&first_arg, tail);
	}

	/// recursive argument reduction (const reference) to get argument type (fct object)
	template <
		typename fobject,
		typename return_type,
		typename first_arg_type,
		typename... arg_types>
	static auto get_concrete_argument_type(
		return_type(fobject::*)(const first_arg_type &, arg_types...))
	{
		first_arg_type first_arg;
		std::function<return_type(arg_types...)> tail_func;
		typename decltype(
			get_concrete_argument_type(tail_func)
		)::tuple_t tail;
		return create_concrete_type(&first_arg, tail);
	}

	/// recursive argument reduction (const reference) to get argument type (fct object)
	template <
		typename return_type,
		typename first_arg_type,
		typename... arg_types>
	static auto get_concrete_argument_type(
		std::function<return_type(const first_arg_type &, arg_types...)> f)
	{
		ignore(f);
		first_arg_type first_arg;
		std::function<return_type(arg_types...)> tail_func;
		typename decltype(
			get_concrete_argument_type(tail_func)
		)::tuple_t tail;
		return create_concrete_type(&first_arg, tail);
	}

/* old implementation: did not allow const or const reference as parameters
	template <typename return_type, typename... arg_types>
	static ConcreteType<type_abstraction_adapter_t, arg_types...>
		get_concrete_argument_type(return_type(*)(arg_types...))
	{
		return ConcreteType<type_abstraction_adapter_t, arg_types...>();
	}

	template <typename fobject, typename return_type, typename... arg_types>
	static ConcreteType<type_abstraction_adapter_t, arg_types...>
		get_concrete_argument_type(return_type(fobject::*)(arg_types...))
	{
		return ConcreteType<type_abstraction_adapter_t, arg_types...>();
	}

	template <typename return_type, typename... arg_types>
	static ConcreteType<type_abstraction_adapter_t, arg_types...>
	 	get_concrete_argument_type(std::function<return_type(arg_types...)> f)
	{
		ignore(f);
		return ConcreteType<type_abstraction_adapter_t, arg_types...>();
	}
*/

	// idea to get return type is based on:
	// https://stackoverflow.com/questions/41301536/get-function-return-type-in-template

	/// get function return type for function pointer
	template <typename return_type, typename... arg_types>
	static auto get_concrete_return_type(return_type(*)(arg_types...))
	{
		if constexpr (std::is_same<void, return_type>::value) {
			return ConcreteType<type_abstraction_adapter_t>(true);
		} else {
			return ConcreteType<type_abstraction_adapter_t, return_type>();
		}
	}

	/// get function return type for function object
	template <
		typename fobject, typename return_type, typename... arg_types>
	static auto get_concrete_return_type(
		return_type(fobject::*)(arg_types...))
	{
		if constexpr (std::is_same<void, return_type>::value) {
			return ConcreteType<type_abstraction_adapter_t>(true);
		} else {
			return ConcreteType<type_abstraction_adapter_t, return_type>();
		}
	}

	/// get function return type for std::function
	template <typename return_type, typename... arg_types>
	static ConcreteType<type_abstraction_adapter_t>
		get_concrete_return_type(
			std::function<return_type(arg_types...)> f)
	{
		ignore(f);
		if constexpr (std::is_same<void, return_type>::value) {
			return ConcreteType<type_abstraction_adapter_t>(true);
		} else {
			return ConcreteType<type_abstraction_adapter_t, return_type>();
		}
	}
};



/// base class to represent an arbitrary function
template<typename type_abstraction_adapter_t>
	requires (type_abstraction_adapter_t::is_TypeAbstractionAdapter)
class AbstractFunction
{
    public:
		/// execution operator wrapper around function execution
		type_abstraction_adapter_t::abstract_type operator()(
			type_abstraction_adapter_t &type_abstraction,
			const type_abstraction_adapter_t::abstract_type &abstract_param)
		{
			return this->execute(type_abstraction, abstract_param);
		}

		/// function execution implementation (virtual)
		virtual type_abstraction_adapter_t::abstract_type execute(
			type_abstraction_adapter_t &type_abstraction,
			const type_abstraction_adapter_t::abstract_type &abstract_param)
		{
			ignore(type_abstraction, abstract_param);
			typename type_abstraction_adapter_t::abstract_type result;
			return result;
		}
		
		/// destructor
        virtual ~AbstractFunction() { }
};


/// concrete implementation of an arbitrary function
template<
	typename type_abstraction_adapter_t,
	typename function_t,
	typename param_t,
	typename result_t>
requires (type_abstraction_adapter_t::is_TypeAbstractionAdapter)
class ConcreteFunction : public AbstractFunction<type_abstraction_adapter_t>
{
	private:
		/// function to abstract
		function_t function;

		/// abstract function parameters
		param_t parameters;

		/// abstract result
		result_t result;

    public:
		/// constructor
		ConcreteFunction(auto function, auto param, auto result) :
			function(function), parameters(param), result(result) {}

		/// concrete function execution
		virtual type_abstraction_adapter_t::abstract_type execute(
			type_abstraction_adapter_t &type_abstraction,
			const type_abstraction_adapter_t::abstract_type &abstract_param
		) override
		{
			assert(parameters.deserialize(type_abstraction, abstract_param)
				&& "(most probably wrong parameters passed)");

			using ret_t = decltype(std::apply(function, parameters.values));

			if constexpr (std::is_same<void, ret_t>::value) {
				std::apply(function, parameters.values);
			} else {
				result.values = std::make_tuple(
					std::apply(function, parameters.values));
			}

			return result.serialize(type_abstraction);
		}

		/// destructor of derived class
        virtual ~ConcreteFunction() { }
};


/// wrapper function to create an abstract function from a function
template<typename type_abstraction_adapter_t, typename function_t>
	requires (type_abstraction_adapter_t::is_TypeAbstractionAdapter)
AbstractFunction<type_abstraction_adapter_t> *
	getAbstractFunctionFor(function_t f)
{
	using taa_t = type_abstraction_adapter_t;

    auto params = function_typeinfo<taa_t>::get_concrete_argument_type(f);
    auto result = function_typeinfo<taa_t>::get_concrete_return_type(f);

    return new ConcreteFunction<
		type_abstraction_adapter_t,
		decltype(f),
		decltype(params),
		decltype(result)
	>(  f, params, result  );
}

#endif
