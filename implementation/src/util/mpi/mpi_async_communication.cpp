
#include <cassert>
#include <algorithm>

#include "mpi_async_communication.hpp"


void async_comm::wait_for_one_message()
{
	std::size_t size = outstanding_requests.size();
	if (0 >= size) { return; }

	int index = MPI_UNDEFINED;
	assert(MPI_SUCCESS == MPI_Waitany(
		size, outstanding_requests.data(), &index, MPI_STATUSES_IGNORE));

	if (MPI_UNDEFINED != index) {
		assert((0 <= index) && ((std::size_t) index < size));
		outstanding_requests.erase(outstanding_requests.begin() + index);
		outstanding_data.erase(outstanding_data.begin() + index);
	}
}


void async_comm::wait_for_all_messages()
{
	std::size_t size = outstanding_requests.size();
	if (0 >= size) { return; }

	assert(MPI_SUCCESS == MPI_Waitall(
		size, outstanding_requests.data(), MPI_STATUSES_IGNORE));
}


void async_comm::wait_for_some_messages()
{
	std::size_t size = outstanding_requests.size();
	if (0 >= size) { return; }

	int count_finished = 0;
	std::vector<int> indices_finished(size);

	assert(MPI_SUCCESS == MPI_Waitsome(size, outstanding_requests.data(),
		&count_finished, indices_finished.data(), MPI_STATUS_IGNORE));

	indices_finished.resize(count_finished);
	sort(indices_finished.begin(), indices_finished.end(), std::greater<>());

	for (int i = 0; i < count_finished; i++) {
		auto index = indices_finished[i];
		assert(index != MPI_UNDEFINED);
		outstanding_requests.erase(outstanding_requests.begin() + index);
		outstanding_data.erase(outstanding_data.begin() + index);
	}
}


void async_comm::test_for_one_message()
{
	std::size_t size = outstanding_requests.size();
	if (0 >= size) { return; }

	int index = MPI_UNDEFINED;
	int completed = 0;

	assert(MPI_SUCCESS == MPI_Testany(
		size,
		outstanding_requests.data(),
		&index,
		&completed,
		MPI_STATUS_IGNORE));

	if (completed) {
		assert(index != MPI_UNDEFINED);
		assert(MPI_REQUEST_NULL == outstanding_requests[index]);
		// assert(MPI_SUCCESS == MPI_Wait(&outstanding_requests[index], MPI_STATUS_IGNORE));
		outstanding_requests.erase(outstanding_requests.begin() + index);
		outstanding_data.erase(outstanding_data.begin() + index);
	}
}


void async_comm::test_for_messages()
{
	std::size_t size = outstanding_requests.size();

	if (0 >= size) { return; }
	if (1 == size) { return test_for_one_message(); }

	int count_finished = 0;
	std::vector<int> indices_finished(size);
	assert(MPI_SUCCESS == MPI_Testsome(size, outstanding_requests.data(),
		&count_finished, indices_finished.data(), MPI_STATUS_IGNORE));

	indices_finished.resize(count_finished);
	sort(indices_finished.begin(), indices_finished.end(), std::greater<>());

	for (int i = 0; i < count_finished; i++) {
		auto index = indices_finished[i];
		assert(index != MPI_UNDEFINED);
		assert(MPI_REQUEST_NULL == outstanding_requests[index]);
		// assert(MPI_SUCCESS == MPI_Wait(
			// &outstanding_requests[index], MPI_STATUS_IGNORE));
		outstanding_requests.erase(outstanding_requests.begin() + index);
		outstanding_data.erase(outstanding_data.begin() + index);
	}
}


auto async_comm::communications_in_queue() const
{
	auto num_waiting = outstanding_requests.size();
	#if DEBUG_ASSERTIONS
	assert(num_waiting == outstanding_data.size());
	#endif
	return num_waiting;
}


bool async_comm::communication_is_done() const
{
	bool is_empty = outstanding_requests.empty();
	#if DEBUG_ASSERTIONS
	assert(is_empty == outstanding_data.empty());
	#endif
	return is_empty;
}


async_comm::~async_comm()
{
	// wait_for_all_messages();
	#if DEBUG_ASSERTIONS
	assert(outstanding_requests.empty() == outstanding_data.empty());
	assert(communication_is_done());
	#endif
}



void async_comm_out::send_message(
	const MPI_Comm &comm, int dest, int tag, data_t &&content)
{
	MPI_Request request;
	assert(MPI_SUCCESS == MPI_Isend(
		content.data(),
		content.size(),
		MPI_BYTE,
		dest,
		tag,
		comm,
		&request));

	int completed = 0;
	assert(MPI_SUCCESS == MPI_Test(
		&request, &completed, MPI_STATUS_IGNORE));

	if (!completed) {
		outstanding_requests.emplace_back(std::move(request));
		outstanding_data.emplace_back(std::move(content));
	} else {
		assert(MPI_REQUEST_NULL == request);
		// assert(MPI_SUCCESS == MPI_Wait(&request, MPI_STATUS_IGNORE));
	}
}



void fake_async_comm_out::send_message(
	const MPI_Comm &comm, int dest, int tag, auto content)
{
	assert(MPI_SUCCESS == MPI_Send(
		content.data(), content.size(), MPI_BYTE, dest, tag, comm));
}

