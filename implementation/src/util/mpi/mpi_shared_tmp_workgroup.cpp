
#include <random>
#include <cassert>
#include <filesystem>

#include "mpi_shared_tmp_workgroup.hpp"

#include "mpi_types.hpp"
#include "mpi_util.hpp"
#include "../util.hpp"


std::string mpi_shared_tmp_dir_workgroup::
	build_tmp_dir_path(std::string app_name)
{
	std::string tmp_path = std::filesystem::temp_directory_path();
	tmp_path += "/mpi_split_by_shared_tmp/" + app_name + "-";

	// add shared random number to path
	uint32_t random = 0;

	if (0 == parent_comm_id) {
		std::random_device r;
		random = r();
	}

	assert(MPI_SUCCESS == MPI_Bcast(
		&random, 1, get_mpi<decltype(random)>::type(), 0, parent_comm));

	tmp_path += std::to_string(random);

	return tmp_path;
}


int mpi_shared_tmp_dir_workgroup::identify_workgroup_head()
{
	namespace fs = std::filesystem;

	// create random tmp path
	fs::create_directories(tmp_path);

	// ensure its empty
	assert(fs::is_empty(tmp_path));
	assert(MPI_SUCCESS == MPI_Barrier(parent_comm));

	// create dir with id inside
	assert(fs::create_directory(
		tmp_path + "/" + std::to_string(parent_comm_id)));
	assert(MPI_SUCCESS == MPI_Barrier(parent_comm));

	// find minimum id in local tmp dir
	int min_id = std::numeric_limits<int>::max();
	for (const auto & entry : fs::directory_iterator(tmp_path))
	{
		min_id = std::min(
			min_id, from_string<int>(entry.path().filename()));
	}

	// sanity checks
	assert(0 <= min_id);
	assert(min_id < parent_comm_size);

	// remove tmp directories if all are finished
	// note: /tmp/mpi_split_by_shared_tmp/ will persist
	MPI_Barrier(parent_comm);
	if (parent_comm_id == min_id) {
		std::filesystem::remove_all(tmp_path);
	}
	
	return min_id;
}


bool mpi_shared_tmp_dir_workgroup::is_workgroup_head() const {
	return parent_comm_id == parent_comm_workgroup_head_id;
}


mpi_shared_tmp_dir_workgroup::mpi_shared_tmp_dir_workgroup(
	MPI_Comm parent_comm,
	std::string app_name
) :
	parent_comm(parent_comm)
{
	parent_comm_id = mpi_get_comm_rank(parent_comm);
	parent_comm_size = mpi_get_comm_size(parent_comm);

	if (1 == parent_comm_size)
	{ // shortcut in case of a single process
		assert(0 == parent_comm_id);
		parent_comm_workgroup_head_id = 0;
		workgroup_comm = parent_comm;
		workgroup_comm_id = 0;
		workgroup_comm_size = 1;
		mpi_comm_leaders = parent_comm;
		workgroup_count = 1;
		workgroup_id = 0;
		return;
	}

	tmp_path = build_tmp_dir_path(app_name);
	parent_comm_workgroup_head_id = identify_workgroup_head();
	
	assert(MPI_SUCCESS == MPI_Comm_split(
		parent_comm,
		parent_comm_workgroup_head_id,
		parent_comm_id,
		&workgroup_comm));

	workgroup_comm_id = mpi_get_comm_rank(workgroup_comm);
	workgroup_comm_size = mpi_get_comm_size(workgroup_comm);

	const bool is_head = is_workgroup_head();
	assert(MPI_SUCCESS == MPI_Comm_split(
		parent_comm,
		is_head ? 0 : 1,
		parent_comm_id,
		&mpi_comm_leaders));
	
	if (!is_head) {
		assert(MPI_SUCCESS == MPI_Comm_disconnect(&mpi_comm_leaders));
		mpi_comm_leaders = MPI_COMM_NULL;
	}
	assert(MPI_SUCCESS == MPI_Barrier(parent_comm));

	int bcast[2];
	if (is_head) {
		bcast[0] = mpi_get_comm_size(mpi_comm_leaders);
		bcast[1] = mpi_get_comm_rank(mpi_comm_leaders);
	}

	assert(MPI_SUCCESS == MPI_Bcast(
		&bcast, 2, get_mpi<int>::type(), 0, workgroup_comm));

	workgroup_count = bcast[0];
	workgroup_id = bcast[1];
	
	if (0 == parent_comm_id) { assert(is_head); }
}


mpi_shared_tmp_dir_workgroup::~mpi_shared_tmp_dir_workgroup()
{
	int was_finalized = 0;
	assert(MPI_SUCCESS == MPI_Finalized( &was_finalized ));
	if (!was_finalized) {
		assert(MPI_SUCCESS == MPI_Barrier(parent_comm));

		if (is_workgroup_head()) {
			assert(MPI_SUCCESS ==
				MPI_Comm_disconnect(&mpi_comm_leaders));
			mpi_comm_leaders = MPI_COMM_NULL;
		}

		assert(MPI_SUCCESS == MPI_Barrier(parent_comm));
		assert(MPI_SUCCESS == MPI_Comm_disconnect(&workgroup_comm));
		workgroup_comm = MPI_COMM_NULL;
	}
}

