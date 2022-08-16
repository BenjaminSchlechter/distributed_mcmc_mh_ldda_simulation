#ifndef MPI_ASYNC_COMMUNICATION_HPP
#define MPI_ASYNC_COMMUNICATION_HPP

#include <vector>
#include <mpi.h>

/// base helper class for async MPI communcation
class async_comm
{
	protected:
		/// message data type
		using data_t = std::vector<uint8_t>;

		/// open transfers
		std::vector<MPI_Request> outstanding_requests;

		/// data of incomplete transfers
		std::vector<data_t> outstanding_data;

	public:
		/// wait until one open message transfer is completed
		void wait_for_one_message();

		/// wait until all open message transfers are completed
		void wait_for_all_messages();

		/// wait until one ore more open message transfers completed
		void wait_for_some_messages();

		/// poll and complete one open message transfer if possible
		void test_for_one_message();

		/// poll to complete all open transfers which finished
		void test_for_messages();

		/// number of open transfers
		auto communications_in_queue() const;

		/// check if no transfer which has to complete is left
		bool communication_is_done() const;

		/// destructor
		~async_comm();
};

/// derived helper class providing async message sending
class async_comm_out : public async_comm
{
	public:
		/// send a message asynchronous using MPI
		void send_message(
			const MPI_Comm &comm, int dest, int tag, data_t &&content);
};

/// derived helper class for synchronous messaging (debug communication)
class fake_async_comm_out : public async_comm
{
	public:
		void send_message(
			const MPI_Comm &comm, int dest, int tag, auto content);
};


#endif
