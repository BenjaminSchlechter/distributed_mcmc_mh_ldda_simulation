#ifndef RPC_HPP
#define RPC_HPP

#include <iostream>
#include <limits>
#include <functional>
#include <type_traits>
#include <map>
#include <memory>

#include "../util/util.hpp"

#include "../util/basic_serialization.hpp"

#include "type_abstraction.hpp"
#include "rtti.hpp"
#include "function_abstraction.hpp"


/// appended identifier string for function results
const std::string FCT_NAME_RESULT_TAG = "__result";


/// message to code a (remote) procedure call
template<typename data_t = std::vector<uint8_t>>
class rpc_message
{
	public:
		/// message type
		using data_type = data_t;

		/// message id
		std::size_t id;

		/// function name
		std::string function_name;

		/// wheter an answer is awaited
		bool answer_required;

		/// function parameter data (abstract representation)
		data_t data;

		/// checksum (optional, used for debugging and error detection)
		uint8_t crc;


		/// constructor
		rpc_message() :
			id(std::numeric_limits<std::size_t>::max()),
			function_name(),
			answer_required(false),
			data(),
			crc(0)
		{}

		/// copy constructor
		rpc_message(const rpc_message &other) :
			id(other.id),
			function_name(other.function_name),
			answer_required(other.answer_required),
			data(other.data),
			crc(other.crc)
		{}

		/// swap function
		friend void swap(rpc_message &l, rpc_message &r)
		{
			std::swap(l.id, r.id);
			std::swap(l.function_name, r.function_name);
			std::swap(l.answer_required, r.answer_required);
			std::swap(l.data, r.data);
			std::swap(l.crc, r.crc);
		}


		/// assignment operator
		rpc_message &operator=(const rpc_message &other) {
			id = other.id;
			function_name = other.function_name;
			answer_required = other.answer_required;
			data = other.data;
			crc = other.crc;
			return *this;
		}

/*
		/// assignment operator by using swap
		rpc_message &operator=(const rpc_message &other) {
			swap(*this, other);
			return *this;
		}
*/

		/// move semantics by using swap
		rpc_message(rpc_message &&other) {
			swap(*this, other);
		}


	private:
		/// actual checksum computation (helper function)
		uint8_t compute_crc_() const
		{
			decltype(crc) crc_;
			crc_ = crc8((uint8_t *) &id, sizeof(decltype(id)));
			crc_ = crc8((uint8_t *) function_name.c_str(),
				function_name.length(), crc_);
			crc_ = crc8((uint8_t *) &answer_required, 1, crc_);
			crc_ = crc8((uint8_t *) data.data(), data.size(), crc_);
			return crc_;
		}

	public:
		// optional crc functionality
		
		/// compute and set checksum
		uint8_t compute_crc() {
			crc = compute_crc_();
			return crc;
		}

		/// test for correct checksum
		bool check_crc() const {
			return (compute_crc_() == crc);
		}
};


/// bitsery serialization of remote procedure call messages
template<typename S>
void serialize(S &s, rpc_message<std::vector<uint8_t>> &v) {
	std::size_t data_len = v.data.size();
	generic_serialize(s,
		v.id, v.function_name, v.answer_required, data_len);
	v.data.resize(data_len);
	s.container1b(v.data, data_len);
	assert(v.data.size() == data_len);
	generic_serialize(s, v.crc);
}


/// class to manager remote procedure calls
template<typename type_abstraction_adapter_t>
	requires (type_abstraction_adapter_t::is_TypeAbstractionAdapter)
class remote_procedure_manager
{
	private:

		/// used for parameter abstraction
		type_abstraction_adapter_t type_abstraction;

		/// function abstraction
		using abstract_function_t =
			AbstractFunction<type_abstraction_adapter_t>;

	public:

		/// remote procedure call message type
		using rpc_message_t =
			rpc_message<typename type_abstraction_adapter_t::abstract_type>;
	
		/// functions available for call
		std::map<std::string, std::unique_ptr<abstract_function_t>>
			function_map;


		/// make function available for remote procedure calls
		template<typename function_t>
		void add_function(std::string name, function_t function)
		{
			assert(!name.empty());

			AbstractFunction<type_abstraction_adapter_t> *afptr
				= getAbstractFunctionFor<
					type_abstraction_adapter_t,
					function_t> (function);
			assert(afptr);

			auto result = function_map.try_emplace(
				name,
				std::unique_ptr<abstract_function_t>(afptr)
			);
			assert(result.second &&
				"there is already a function with this name registered!");
		}

		/// create a remote procedure call message for a function call
		template<typename... param_types>
		auto prepare_call(std::string name, param_types... params)
		{
			assert(!name.empty());

			rpc_message_t msg;
			msg.id = 0;
			msg.function_name = name;
			msg.answer_required = false;

			ConcreteType<type_abstraction_adapter_t, param_types...>
				param_container = {params..., true};

			msg.data = param_container.serialize(type_abstraction);
			return msg;
		}

		/// call function by passing a remote procedure call message
		auto execute_rpc(const rpc_message_t &msg)
		{
			// std::cout << "execute_rpc: " <<
				// msg.function_name << std::endl;

			assert(!msg.function_name.empty());

			auto fsearch = function_map.find(msg.function_name);
			if (function_map.end() == fsearch) {
				std::cerr << "Unknown remote procedure call: "
					<< msg.function_name << std::endl;
				assert(false && "Unknown remote procedure call!");
			} else {
				// std::cout << "rpc executing: "
					// << msg.function_name << std::endl;
			}

			rpc_message_t result_msg;
			result_msg.id = msg.id;
			result_msg.function_name =
				msg.function_name + FCT_NAME_RESULT_TAG;
			result_msg.answer_required = false;

			abstract_function_t *fptr = (fsearch->second).get();

			try {
				result_msg.data =
					fptr->execute(type_abstraction, msg.data);
			} catch (...) {
				std::cout << "exception executing function: "
					<< result_msg.function_name << std::endl;
				abort();
			}

			return result_msg;
		}
};


#endif
