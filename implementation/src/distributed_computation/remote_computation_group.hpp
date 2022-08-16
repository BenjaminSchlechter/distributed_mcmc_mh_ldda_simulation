#ifndef REMOTE_COMPUTATION_GROUP
#define REMOTE_COMPUTATION_GROUP

#include <cstddef>
#include <cassert>
#include <deque>

#include <mpi.h>

#include "../util/mpi/mpi_util.hpp"
#include "../util/mpi/mpi_async_communication.hpp"


/// manager class to execute functions on remote nodes using a remote execution context
template<
	typename remote_execution_context_t,
	class type_abstraction_adapter_t
		= remote_execution_context_t::type_abstraction_adapter_t>
requires (type_abstraction_adapter_t::is_TypeAbstractionAdapter)
class remote_computation_group
{
	public:
		/// task type: remote procedure call message
		using rpc_message_t = remote_execution_context_t::rpc_message_t;

		/// communication buffer type
		using buffer_t = type_abstraction_adapter_t::abstract_type;

	private:

		/// number of tasks waiting to be executed
		std::deque<std::pair<int, rpc_message_t>> tasks;

		/// serialization of abstract types
		type_abstraction_adapter_t taa;

		/// number of pending (scheduled) tasks
		std::size_t num_waiting_for_reply;

		/// asynchronous background communication
		async_comm_out background_comm;
		// fake_async_comm_out background_comm;


		/// recieve a pending message
		std::pair<int, rpc_message_t> get_msg(MPI_Status &status)
		{
			buffer_t b;
			get_message(env_comm, &status, b);
			rpc_message_t msg;
			assert(0 < b.size());
			assert(taa.deserialize(b, msg));

			assert(msg.check_crc());
			return std::make_pair(status.MPI_SOURCE, msg);
		}


		/// wait until a message arrives
		std::pair<int, rpc_message_t> wait_for_msg()
		{
			assert(0 != env_comm_id);
			MPI_Status status;
			while (1) {
				wait_for_message(env_comm, &status, mpi_tag);
				if (0 == get_message_size(&status)) {
					num_waiting_for_reply--;
				} else {
					break;
				}
			}
			
			// std::cout << "wait_for_msg" << std::endl;
			return get_msg(status);
		}


		/// execute a message using the remote execution context
		void execute(const std::pair<int, rpc_message_t> &task)
		{
			const int origin = task.first;
			rpc_message_t msg = task.second;
			// std::cout << "executing " << msg.function_name
				// << " (id: " << msg.id << ") with"
				// << (msg.answer_required ? "" : "out")
				// << " reply and arg size " << msg.data.size()
				// << " on " << env_comm_id << std::endl;

			assert(msg.check_crc());

			rpc_message_t result = rec.execute_rpc(msg);
			assert(!result.answer_required);

			if (msg.answer_required) {
				result.compute_crc();
			}

			if (origin == env_comm_id) {
				if (msg.answer_required) {
					rec.execute_rpc(result);
				}
			} else {
				if (msg.answer_required) {
					buffer_t b = taa.serialize(result);
					background_comm.send_message(
						env_comm, origin, mpi_tag, std::move(b));
					// print_vector("execute_send_reply:", b);
				} else {
					// std::cout << "ping " << origin << std::endl;
					ping(env_comm, origin, mpi_tag);
				}
			}
		}


	public:
		/// communicator this scheduler manages
		MPI_Comm env_comm;

		/// id of node in communicator
		const int env_comm_id;

		/// number of nodes in communicator
		const int env_comm_size;

		/// tag used for scheduler communication
		const int mpi_tag;


		/// remote execution context
		remote_execution_context_t &rec;



		/// constructor
		remote_computation_group(
			MPI_Comm env_comm,
			int mpi_tag,
			remote_execution_context_t &rec
		) :
			tasks(),
			taa(),
			num_waiting_for_reply(0),
			background_comm(),
			env_comm(env_comm),
			env_comm_id(mpi_get_comm_rank(env_comm)),
			env_comm_size(mpi_get_comm_size(env_comm)),
			mpi_tag(mpi_tag),
			rec(rec)
		{}


		/// schedule a task on a node
		void schedule(auto task, int target_id)
		{
			// std::cout << "scheduling " << task.function_name
				// << " (id: " << task.id << ") with"
				// << (task.answer_required ? "" : "out")
				// << " reply and arg size " << task.data.size()
				// << " on " << target_id << " by "
				// << env_comm_id << std::endl;

			task.compute_crc();

			if (env_comm_id == target_id) {
				tasks.push_back(std::make_pair(target_id, task));
			} else {
				buffer_t b = taa.serialize(task);
				// int bcrc = crc8((uint8_t *) b.data(), b.size());
				// std::cout << "CCC schedule bcrc: " << bcrc << std::endl;
				// print_vector("CCC get_msg:", b);
				background_comm.send_message(
					env_comm, target_id, mpi_tag, std::move(b));
				num_waiting_for_reply++;
				// std::cout << "num_waiting_for_reply increased to: "
					// << num_waiting_for_reply << std::endl;
			}
		}


		/// number tasks waiting to be executed
		std::size_t num_tasks_waiting() const {
			return tasks.size();
		}


		/// number of pending tasks on remote nodes
		std::size_t num_tasks_pending() const {
			return num_waiting_for_reply;
		}


		/// check if any work is left or everything completed
		bool is_work_outstanding() const {
			return ((0 < num_tasks_pending())
				|| (0 < num_tasks_waiting()))
				|| !background_comm.communication_is_done();
		}


		/// communicate, do available work and optionally wait for work
		void do_work(bool wait_for_work = false)
		{
			MPI_Status status;
			while (is_message_available(env_comm, &status, mpi_tag)) {
				if (0 == get_message_size(&status)) {
					accept_ping(env_comm, &status);
					num_waiting_for_reply--;
				} else {
					tasks.push_back(get_msg(status));
				}
			}

			background_comm.test_for_messages();

			if (wait_for_work && (1 < env_comm_size)) {
				if (!background_comm.communication_is_done()) {
					while (tasks.empty()) {
						if (is_message_available(
							env_comm, &status, mpi_tag))
						{
							if (0 == get_message_size(&status)) {
								accept_ping(env_comm, &status);
								num_waiting_for_reply--;
							} else {
								tasks.push_back(get_msg(status));
								break;
							}
						}

						background_comm.test_for_one_message();
						if (background_comm.communication_is_done()) {
							break;
						}
					}
				}
				
				if (tasks.empty()) {
					tasks.push_back(wait_for_msg());
				}
			}

			if (!tasks.empty()) {
				std::pair<int, rpc_message_t> p = tasks.front();
				execute(p);
				tasks.pop_front();
			}
		}

};

#endif
