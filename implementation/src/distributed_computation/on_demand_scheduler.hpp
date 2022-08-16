#ifndef ON_DEMAND_SCHEDULER
#define ON_DEMAND_SCHEDULER

#include <cstddef>
#include <cassert>
#include <deque>
#include <memory>
#include <chrono>

#include <mpi.h>

#include "../util/mpi/mpi_util.hpp"
#include "../util/mpi/mpi_async_communication.hpp"

#include "lazy_round_robin_scheduler.hpp"


/** \brief scheduler for load balancing of probsat computations inside a workgroup
 * 
 * uses internally the lazy round robin scheduler with up to two jobs per worker
 */
template<
	typename remote_execution_context_t,
	class type_abstraction_adapter_t =
		remote_execution_context_t::type_abstraction_adapter_t>
requires (type_abstraction_adapter_t::is_TypeAbstractionAdapter)
class on_demand_scheduler
{
	public:
		/// task type: remote procedure call message
		using rpc_message_t = remote_execution_context_t::rpc_message_t;
		/// communication buffer type
		using buffer_t = type_abstraction_adapter_t::abstract_type;

	private:
		/// least workload capacity to keep workers busy
		static constexpr const std::size_t workload_capacity = 2;

		/// scheduler base type
		using scheduler_t = lazy_round_robin_scheduler<workload_capacity>;

		/// scheduler type, unique ptr only on head node used
		using uptr_scheduler_t = std::unique_ptr<scheduler_t>;

		/// lazy round robin scheduler
		uptr_scheduler_t scheduler;

		/// queue of waiting tasks (available to process or schedule)
		std::deque<rpc_message_t> tasks;

		/// serialization of abstract types
		type_abstraction_adapter_t taa;

		/// number of pending (already scheduled) tasks
		std::size_t num_waiting_for_reply;
		
		/// asynchronous background communication
		async_comm_out background_comm;
		// fake_async_comm_out background_comm;



		/// process a result message
		void process_msg(MPI_Status &status)
		{
			assert(0 == env_comm_id);
			
			if (0 < get_message_size(&status)) {
				// std::cout << "process_msg" << std::endl;
				execute(get_msg(status));
			} else {
				assert(MPI_SUCCESS == MPI_Recv(
					nullptr,
					0,
					MPI_BYTE,
					status.MPI_SOURCE,
					mpi_tag,
					env_comm,
					&status));
			}

			assert(0 < status.MPI_SOURCE);
			scheduler->task_finished(status.MPI_SOURCE - 1);
			num_waiting_for_reply--;
		}


		/// recieve a pending message
		rpc_message_t get_msg(MPI_Status &status)
		{
			buffer_t b;
			get_message(env_comm, &status, b);
			// print_vector("get_msg:", b);
			rpc_message_t msg;
			assert(0 < b.size());
			assert(taa.deserialize(b, msg));

			// std::cout << "c msg from: " << status.MPI_SOURCE
				// << " with tag " << status.MPI_TAG << std::endl;
			return msg;
		}


		/// wait until a message arrives
		rpc_message_t wait_for_msg()
		{
			assert(0 != env_comm_id);
			MPI_Status status;
			wait_for_message(env_comm, &status, mpi_tag);
			// std::cout << "wait_for_msg" << std::endl;
			return get_msg(status);
		}


		/// execute a message using the remote execution context
		void execute(auto msg)
		{
			// std::cout << "executing " << msg.function_name
				// << " (id: " << msg.id << ") with"
				// << (msg.answer_required ? "" : "out")
				// << " reply on " << env_comm_id
				// << " (crc: " << (int) msg.crc << ")" << std::endl;

			assert(msg.check_crc());

			rpc_message_t result = rec.execute_rpc(msg);
			assert(!result.answer_required);

			if (msg.answer_required) {
				result.compute_crc();
			}

			if (0 == env_comm_id) {
				if (msg.answer_required) {
					rec.execute_rpc(result);
				}
			} else {
				if (msg.answer_required) {
					buffer_t b = taa.serialize(result);
					background_comm.send_message(
						env_comm, 0, mpi_tag, std::move(b));
					// print_vector("execute_send_reply:", b);
				} else {
					ping(env_comm, 0, mpi_tag);
				}
			}
		}


		/// only schedule queued tasks to workers in idle state
		void feed_hungry_workers()
		{
			assert(0 == env_comm_id);

			while (scheduler->is_process_without_work())
			{
				if (tasks.empty()) { return; }

				rpc_message_t task = tasks.front();
				tasks.pop_front();
				schedule(task);
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
		on_demand_scheduler(
			MPI_Comm env_comm,
			int mpi_tag,
			remote_execution_context_t &rec
		) :
			scheduler(nullptr),
			tasks(),
			taa(),
			num_waiting_for_reply(0),
			background_comm(),
			env_comm(env_comm),
			env_comm_id(mpi_get_comm_rank(env_comm)),
			env_comm_size(mpi_get_comm_size(env_comm)),
			mpi_tag(mpi_tag),
			rec(rec)
		{
			if (0 == env_comm_id) {
				scheduler =
					uptr_scheduler_t(new scheduler_t(env_comm_size - 1));
			}
		}


		/// schedule a task or add it to the waiting queue
		void schedule(auto task, int target = 0)
		{
			assert(0 == env_comm_id);

			int target_id = target;
			if (0 == target) {
				target_id = 1 + scheduler->get_id_to_schedule_task_on();
			}

			/*int crc = */ task.compute_crc();

			// std::cout << "ods scheduling " << task.function_name
				// << " (id: " << task.id << ") with"
				// << (task.answer_required ? "" : "out")
				// << " reply on " << target
				// << " (crc: " << crc << ")" << std::endl;

			if (0 == target_id) {
				tasks.push_back(task);
			} else {
				buffer_t b = taa.serialize(task);
				background_comm.send_message(
					env_comm, target_id, mpi_tag, std::move(b));
				// print_vector("schedule:", b);
				num_waiting_for_reply++;
			}
		}


		/// number of waiting tasks in queue
		std::size_t num_tasks_waiting() const {
			return tasks.size();
		}


		/// number of already scheduled but pending tasks
		std::size_t num_tasks_pending() const {
			return num_waiting_for_reply;
		}


		/// check if any work is left or everything completed
		bool is_work_outstanding() const {
			return ((0 < num_tasks_pending())
				|| (0 < num_tasks_waiting()))
				|| !background_comm.communication_is_done();
		}


		/// do available work and optionally wait for work if nothing todo
		size_t do_work(bool wait_for_work = false)
		{
			if ((0 == env_comm_id) && (1 < env_comm_size))
			{
				feed_hungry_workers();

				MPI_Status status;
				while (is_message_available(env_comm, &status, mpi_tag))
				{
					process_msg(status);
					feed_hungry_workers();
				}

				while (scheduler->worker_available())
				{
					if (tasks.empty()) { return 0; }

					rpc_message_t task = tasks.front();
					tasks.pop_front();
					schedule(task);

					if (is_message_available(env_comm, &status, mpi_tag))
					{
						process_msg(status);
					}
				}
				
				background_comm.test_for_messages();
				
				// std::cout << "communications_in_queue: "
					// << background_comm.communications_in_queue()
					// << std::endl;

				if (wait_for_work) {
					if (background_comm.communication_is_done()) {
						if (!tasks.empty()) {
							rpc_message_t task = tasks.front();
							tasks.pop_front();
							execute(task);
						}
					} else {
						background_comm.wait_for_some_messages();
					}
				}
			} else {
				MPI_Status status;
				while (is_message_available(env_comm, &status, mpi_tag))
				{
					// std::cout << "message available" << std::endl;
					tasks.push_back(get_msg(status));
				}

				background_comm.test_for_messages();

				/*
				if (tasks.empty()) {
					if (wait_for_work && (1 < env_comm_size)) {
						tasks.push_back(wait_for_msg());
						background_comm.test_for_messages();
					}
				}
				*/

				if (wait_for_work && (1 < env_comm_size)) {
					if (!background_comm.communication_is_done()) {
						while (tasks.empty()) {
							if (is_message_available(
								env_comm, &status, mpi_tag))
							{
								tasks.push_back(get_msg(status));
								break;
							}

							background_comm.test_for_one_message();
							if (background_comm.communication_is_done())
							{
								break;
							}
						}
					}
					
					if (tasks.empty()) {
						tasks.push_back(wait_for_msg());
					}
				}

				if (!tasks.empty()) {
					auto start =
						std::chrono::high_resolution_clock::now();

					execute(tasks.front());

					auto end =
						std::chrono::high_resolution_clock::now();
					tasks.pop_front();

					return std::chrono::duration_cast<
						std::chrono::milliseconds>(end - start).count();
				}
			}

			return 0;
		}
};

#endif

