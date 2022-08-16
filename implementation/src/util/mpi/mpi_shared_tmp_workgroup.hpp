#ifndef MPI_SHARED_WORKGROUP_HPP
#define MPI_SHARED_WORKGROUP_HPP

#include <string>
#include <mpi.h>

/** \brief creates workgroups based on an available shared tmp directory
 * 
 * Creates subworkgroups of a MPI communicator based on an available
 * shared tmp directory. Provides inter and intra workgroup communicators.
 */
class mpi_shared_tmp_dir_workgroup
{
	public:
		/// parent MPI communicator
		MPI_Comm parent_comm;

		/// node id in parent comm
		int parent_comm_id;

		/// number of nodes in parent comm
		int parent_comm_size;

		/// workgroup head id
		int parent_comm_workgroup_head_id;


		/// intra workgroup communicator
		MPI_Comm workgroup_comm;

		/// node if in intra workgroup comm
		int workgroup_comm_id;

		/// number of nodes in workgroup
		int workgroup_comm_size;


		/// inter workgroup communicator, workgroup heads only
		MPI_Comm mpi_comm_leaders;

		/// number of workgroups
		int workgroup_count;

		/// id of this workgroup
		int workgroup_id;

	private:
		/// tmp path to check
		std::string tmp_path;

		/// create tmp path to partition workgroups
		std::string build_tmp_dir_path(std::string app_name);
		
		/// helper function to get id of workgroup head in parent communicator
		int identify_workgroup_head();

	public:
		/// checks if this node is head of a workgroup
		bool is_workgroup_head() const;

		/// constructor (parent communicator, application name)
		mpi_shared_tmp_dir_workgroup(
			MPI_Comm parent_comm = MPI_COMM_WORLD,
			std::string app_name = "main");
		
		/// destructor
		~mpi_shared_tmp_dir_workgroup();
};

#endif
