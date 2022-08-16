#ifndef REMOTE_EXECUTION_CONTEXT_HPP
#define REMOTE_EXECUTION_CONTEXT_HPP

#include "rpc.hpp"
#include "../util/uuid.hpp"

/// context for remote function execution including on_result functions
template<typename taa_t> requires (taa_t::is_TypeAbstractionAdapter)
class remote_execution_context
{
	public:
		/// type of data type abstraction adapter
		using type_abstraction_adapter_t = taa_t;

	private:
		/// uuid data type for function call tracking
		using uuid_t =
			uuid<remote_execution_context<type_abstraction_adapter_t>>;

	public:
		/// abstract data type
		using data_t = type_abstraction_adapter_t::abstract_type;

		/// remote procedure call message type
		using rpc_message_t = rpc_message<data_t>;

		/// remote procedure call manager type
		using remote_procedure_manager_t =
			remote_procedure_manager<type_abstraction_adapter_t>;

		/// remote procedure call manager
		remote_procedure_manager_t &rpmanager;


		/// constructor
		remote_execution_context(remote_procedure_manager_t &rpmanager) :
			rpmanager(rpmanager) {}


		/// adding an on_result function to a remote procedure call message
		template<typename function_t>
		rpc_message_t on_result(auto msg, function_t function)
		{
			assert(0 == msg.id);

			std::size_t id = 1 + uuid_t::get();
			assert(0 != id);

			std::string rfname = msg.function_name
				+ FCT_NAME_RESULT_TAG + "_" + std::to_string(id);
			rpmanager.add_function(rfname, function);

			msg.id = id;
			msg.answer_required = true;

			return msg;
		}


		/// execute a function by passing a remote prodedure call message
		auto execute_rpc(rpc_message_t &msg)
		{
			bool is_result =
				(false == msg.answer_required) && (0 != msg.id);

			if (is_result) {
				msg.function_name =
					msg.function_name + "_" + std::to_string(msg.id);
			}

			rpc_message_t result_msg = rpmanager.execute_rpc(msg);

			if (is_result) {
				assert(1 == rpmanager.function_map.erase(msg.function_name));
				uuid_t::free(msg.id - 1);
				result_msg.id = 0;
				result_msg.function_name = "";
			}

			return result_msg;
		}
};


#endif
