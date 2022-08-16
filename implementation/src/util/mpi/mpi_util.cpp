
#include "mpi_util.hpp"
#include <cassert>

int mpi_get_comm_rank(const MPI_Comm &comm)
{
	int mpi_id = -1;
	assert(MPI_SUCCESS == MPI_Comm_rank(comm, &mpi_id));
	assert(0 <= mpi_id);
	return mpi_id;
}


int mpi_get_comm_size(const MPI_Comm &comm)
{
	int wg_size = -1;
	assert(MPI_SUCCESS == MPI_Comm_size(comm, &wg_size));
	assert(0 <= wg_size);
	return wg_size;
}

bool is_message_available(
	const MPI_Comm &comm, MPI_Status *status, int tag)
{
	int received_flag = 0;
	assert(MPI_SUCCESS ==
		MPI_Iprobe(MPI_ANY_SOURCE, tag, comm, &received_flag, status));
	return 0 != received_flag;
}

void wait_for_message(const MPI_Comm &comm, MPI_Status *status, int tag)
{
	assert(MPI_SUCCESS == MPI_Probe(MPI_ANY_SOURCE, tag, comm, status));
}

int get_message_size(MPI_Status *status)
{
	int length = -1;
	assert(MPI_SUCCESS == MPI_Get_count(status, MPI_BYTE, &length));
	assert(0 <= length);
	return length;
}

void get_message(
	const MPI_Comm &comm,
	MPI_Status *status,
	std::vector<uint8_t> &content)
{
	int length = get_message_size(status);
	content.resize(length);

	assert(MPI_SUCCESS == MPI_Recv(
		content.data(),
		length,
		MPI_BYTE,
		status->MPI_SOURCE,
		status->MPI_TAG,
		comm,
		status));

	// assert(get_message_size(status) == length);
}

void send_message(
	const MPI_Comm &comm,
	int dest,
	int tag,
	const std::vector<uint8_t> &content)
{
	assert(MPI_SUCCESS == MPI_Send(
		content.data(), content.size(), MPI_BYTE, dest, tag, comm));
}

void ping(const MPI_Comm &comm, int dest, int tag)
{
	assert(MPI_SUCCESS ==
		MPI_Send(nullptr, 0, MPI_BYTE, dest, tag, comm));
}

void accept_ping(const MPI_Comm &comm, MPI_Status *status)
{
	assert(MPI_SUCCESS == MPI_Recv(
		nullptr,
		0,
		MPI_BYTE,
		status->MPI_SOURCE,
		status->MPI_TAG,
		comm,
		status));
}

