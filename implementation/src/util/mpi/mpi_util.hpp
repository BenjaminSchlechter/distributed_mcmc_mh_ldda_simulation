#ifndef MPI_UTIL_HPP
#define MPI_UTIL_HPP

#include <mpi.h>
#include <vector>

/// query communicator information: rank of node
int mpi_get_comm_rank(const MPI_Comm &comm);

/// query communicator information: number of nodes in communicator
int mpi_get_comm_size(const MPI_Comm &comm);

/// poll for available messages matching tag and set status
bool is_message_available(
	const MPI_Comm &comm,
	MPI_Status *status,
	int tag = MPI_ANY_TAG);

/// wait until message matching tag is available and set status
void wait_for_message(
	const MPI_Comm &comm,
	MPI_Status *status,
	int tag = MPI_ANY_TAG);

/// query size of message matching status (source & tag)
int get_message_size(MPI_Status *status);

/// recieve content of message matching status (source & tag) into vector
void get_message(
	const MPI_Comm &comm,
	MPI_Status *status,
	std::vector<uint8_t> &content);

/// send content of vector as message
void send_message(
	const MPI_Comm &comm,
	int dest,
	int tag,
	const std::vector<uint8_t> &content);

/// send empty message (used as ping)
void ping(const MPI_Comm &comm, int dest, int tag);

/// accept empty message matching status (source & tag)
void accept_ping(const MPI_Comm &comm, MPI_Status *status);

#endif
