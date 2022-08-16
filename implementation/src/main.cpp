
// Due to debugging at the end of the development phase, this file
// was not yet cleaned up and well documented (work in progress state)
// There was a bug after rescheduling when the Markov chains were transferred

// mpirun -np 6 --use-hwthread-cpus --oversubscribe main
// seq 1 6 |parallel -I{} -j 1 "bash -c 'mpirun -np {} main > {}.txt'"
// seq 7 12 |parallel -I{} -j 1 "bash -c 'mpirun -np {} --use-hwthread-cpus --oversubscribe main > {}.txt'"
// for n in $(seq 9); do mv $n.txt 0$n.txt; done;
// grep -m 1 duration *.txt
// grep -m 1 duration *.txt |cut -d " " -f 3 |gnuplot -p -e 'plot "/dev/stdin" with lines'
// grep -m 1 duration *.txt |awk '{print $i*$3; $i++}' |gnuplot -p -e 'plot "/dev/stdin" with lines'

// grep -m 1 duration *.txt |awk '{print '$(grep duration 01.txt|cut -d " " -f 3)'/($3/$i) - 1; $i++}' |gnuplot -p -e 'plot "/dev/stdin" with lines'
// grep -m 1 duration *.txt |awk '{print '$(grep duration 01.txt|cut -d " " -f 3)'/$3; $i++}' |gnuplot -p -e 'plot "/dev/stdin" with lines'

// echo ... |sed "s/ / + /g" |bc
// echo ... |sed "s/ /\n/g" |awk '{sum+=$1; sumsq+=$1*$1}END{print NR; print sum/NR; print sqrt(sumsq/(NR-1) - (sum/NR)**2)}'
// echo ... |sed "s/ /\n/g" |sort -nk1 |gnuplot -p -e 'set logscale y 2.718281828459045; plot "/dev/stdin" with lines'
// echo ... |sed "s/ /\n/g" |awk '{print log($1)}' |sort -g |gnuplot -p -e 'plot "/dev/stdin" with lines'

// grep "c value" out-1000.txt |cut -d " " -f 3 |gnuplot -p -e 'plot "/dev/stdin" with lines'
// grep "accepted" out-1000.txt |sed "s/)//g" |cut -d "(" -f 2 |gnuplot -p -e 'plot "/dev/stdin" with lines'

#include <set>
#include <queue>

#include "config.hpp"

/// number of reference chains for each constant initializion value of the config vector
std::size_t num_reference_chains = 1;

/// number of markov chains to generate
std::size_t num_markov_chains = 4;

/// offset to start markov chain generation (uniformly initialized vectors start at 0)
std::size_t markov_chain_offset = 0;

/// target length of the markov chains
std::size_t markov_chain_target_length = 4;

/// take rescheduling into account and evaluate necessity every X seconds
std::size_t reschedule_after_seconds = 0;

/// absolute workload difference required to reschedule
std::size_t reschedule_requires_absolute_difference = 0;

/// maximum runtime until termination
std::size_t max_runtime_in_seconds = 0;

/// checkpoint at termination
bool checkpoint_on_termination = true;

/// checkpoint interval in iterations
std::size_t checkpoint_after_iterations = 0;

/// checkpoint interval in seconds
std::size_t checkpoint_after_seconds = 0;

/// maximum number of changed positions each iteration
uint8_t mcmc_num_changes = 3; 

// prec_t mcmc_T = 0.05;
// prec_t mcmc_T = 0.05;
// prec_t mcmc_T = 0.005;
// 300 200 100 75 50 40 30 20 10 5

/// factor for exponential acceptance probability
prec_t mcmc_T = 30;

// wenn neu - alt = mcmc_T => p(accept) = 1/e
// wenn neu - alt = 3*mcmc_T => p(accept) = 5 %

const bool disable_rescheduling = true;

// #include "util/util.hpp"
// #include "util/mpi/mpi_util.hpp"
#include "util/mpi/mpi_shared_tmp_workgroup.hpp"
#include "util/mpi/mpi_types.hpp"
// #include "util/uuid.hpp"
// #include "util/bitfield.hpp"

const mpi_shared_tmp_dir_workgroup *workgroup_ptr = nullptr;

// #include "distributed_computation/rtti.hpp"
// #include "distributed_computation/type_abstraction.hpp"
// #include "distributed_computation/function_abstraction.hpp"
#include "distributed_computation/rpc.hpp"
#include "distributed_computation/remote_execution_context.hpp"
#include "distributed_computation/on_demand_scheduler.hpp"
#include "distributed_computation/remote_computation_group.hpp"

using buffer_t = std::vector<uint8_t>;
using output_adapter_t = bitsery::OutputBufferAdapter<buffer_t>;
using input_adapter_t = bitsery::InputBufferAdapter<buffer_t>;
using type_abstraction_adapter_t = Bitsery_TypeAbstractionAdapter<buffer_t, input_adapter_t, output_adapter_t>;

using remote_procedure_manager_t = remote_procedure_manager<type_abstraction_adapter_t>;
using remote_execution_context_t = remote_execution_context<type_abstraction_adapter_t>;
using rpc_message_t = remote_procedure_manager_t::rpc_message_t;

remote_procedure_manager_t wg_rpmanager;

struct wg_leader_exec_env {
	remote_procedure_manager_t rpmanager;
	remote_execution_context_t remote_exec_context;
	remote_computation_group<remote_execution_context_t> rcg;
	wg_leader_exec_env(MPI_Comm env_comm, int mpi_tag) :
		rpmanager(),
		remote_exec_context(rpmanager),
		rcg(env_comm, mpi_tag, remote_exec_context)
	{}
};

std::unique_ptr<wg_leader_exec_env> wg_leaders = nullptr;


#include "util/seed.hpp"

// typedef std::mt19937 prn_engine_t;
using seed_t = seed_type<prn_engine_t::state_size, uint32_t>;
using seed_type_generator_t = seed_type_generator<typename seed_t::base_t>;

seed_t global_seed;


#include "metropolis_hastings/configuration/configuration_vector.hpp"
#include "metropolis_hastings/configuration/configuration_generator.hpp"

constexpr const bool use_bitfield = false;
using cfg_vector_t = binary_configuration_vector<use_bitfield>;
// using cfg_vector_t = binary_configuration_vector;

std::size_t cfg_vector_length;

using cfg_generator_t = std::function<void(prn_engine_t &, cfg_vector_t &)>;

uniform_configuration_generator<cfg_vector_t> ugen;
cfg_generator_t default_cfg_generator = cfg_generator_t(ugen);

std::map<std::size_t, cfg_generator_t> mc_starting_with_special_cfg_vector;

#include "metropolis_hastings/configuration/change_generator.hpp"
constexpr const bool enforce_change = false;
constexpr const bool skip_unchanged_vectors = true;
using change_generator_t = simple_change_generator<cfg_vector_t, enforce_change>;

#include "metropolis_hastings/value_computation/utility/probsat-execution.hpp"
#include "metropolis_hastings/value_computation/modell.hpp"
#include "metropolis_hastings/value_computation/probsat-with-resolvents.hpp"

using probsat_modell_t = probsat_resolvents<cfg_vector_t, prec_t, use_bitfield>;
using computation_modell_t = probsat_modell_t;

#include "metropolis_hastings/mcmc.hpp"
using marcov_chain_state_t = marcov_chain_state<cfg_vector_t>;

std::map<std::size_t, marcov_chain_state_t> mc_chains_to_add;

#include "metropolis_hastings/acceptance_computation/simple_acceptance_computation.hpp"
#include "metropolis_hastings/acceptance_computation/statistical_acceptance_computation.hpp"
using acc_prob_comp_t = exponential_acceptance_probability<prec_t>;
// using acceptance_computation_t =
	// simple_acceptance_computation<prn_engine_t, cfg_vector_t, prec_t, probsat_modell_t, acc_prob_comp_t>;
using acceptance_computation_t =
	statistical_acceptance_computation<prn_engine_t, cfg_vector_t, prec_t, probsat_modell_t, acc_prob_comp_t>;

#include "metropolis_hastings/algorithm.hpp"
using metropolis_hastings_algorithm_t =
	metropolis_hastings_algorithm<
		marcov_chain_state_t,
		change_generator_t,
		acceptance_computation_t, computation_modell_t, acc_prob_comp_t,
		result_statistics<prec_t>,
		std::function<void(std::pair<uint64_t, probsat_return_cause::reason>)>,
		enforce_change,
		skip_unchanged_vectors
	>;


#include "distributed_computation/rescheduling.hpp"
std::unique_ptr<rescheduling_manager> reschedule_manager = nullptr;

std::map<std::size_t, int> marcov_chains_to_move = {};

std::set<std::size_t> marcov_chains_finished;

std::ptrdiff_t markov_chains_to_get = 0;
bool rescheduling_in_progress = false;


bool running = true;

void stop_running() {
	running = false;
}

// void test_rcg(int from, int to) {
	// std::cout << "test_rcg: hello from " << from << " to " << to << std::endl;
// }

void got_markov_chain(std::ptrdiff_t n = -1) {
	markov_chains_to_get += n;
	std::cout << "got_markov_chain + " << n << " = " << markov_chains_to_get
		<< " on " << workgroup_ptr->workgroup_id << std::endl;
	if (0 == markov_chains_to_get) {
		assert(rescheduling_in_progress);
		rescheduling_in_progress = false;
		std::cout << "rescheduling finished on wg " << workgroup_ptr->workgroup_id << std::endl;
	}
}

/*
auto serialize_mcs(auto mc_state) {
	type_abstraction_adapter_t bitsery_taa;
	buffer_t b = bitsery_taa.serialize(mc_state);
	// std::cout << "serialize mcs(" << mc_state.iteration << ", ["
		// << mc_state.cfg_vector.data.size() << "], "
		// << mc_state.prn_engine << "}) on " << workgroup_ptr->workgroup_id << std::endl;
	
	std::size_t checksum = 0;
	for (auto v : b) { checksum += (v?1:0); checksum *= 3; }
	std::cout << "serialize: " << checksum << " on " << workgroup_ptr->workgroup_id << std::endl;
	// std::cout << "serialize:"; for (auto v : b) { std::cout << " " << (v?1:0); } std::cout << std::endl;
	return b;
}

void recieve_markov_chain(std::size_t id, buffer_t mcs) {
	ignore(id, mcs);
	std::cout << "TEST recieve" << std::endl;

	std::size_t checksum = 0;
	for (auto v : mcs) { checksum += (v?1:0); checksum *= 3; }
	std::cout << "recieve: " << checksum << " on " << workgroup_ptr->workgroup_id << std::endl;
	// std::cout << "serialize:"; for (auto v : mcs) { std::cout << " " << (v?1:0); } std::cout << std::endl;

	// type_abstraction_adapter_t bitsery_taa;
	// marcov_chain_state_t mc_state(cfg_vector_length, prn_engine_t());
	// assert(bitsery_taa.deserialize(mcs, mc_state));

	// std::cout << "recieve_markov_chain(" << id << ", {" << mc_state.iteration << ", ["
		// << mc_state.cfg_vector.data.size() << "], "
		// << mc_state.prn_engine << "}) on " << workgroup_ptr->workgroup_id << std::endl;

	got_markov_chain();
}
*/

/*
void recieve_markov_chain(const std::size_t &id, const marcov_chain_state_t &mc_state) {
	std::cout << "TEST recieve" << std::endl;

	std::cout << "recieve_markov_chain(" << id << ", {" << mc_state.iteration << ", ["
		<< mc_state.cfg_vector.data.size() << ", " << mc_state.cfg_vector.count_ones() << "], "
		<< mc_state.prn_engine << "}) on " << workgroup_ptr->workgroup_id << std::endl;

	got_markov_chain();
}
*/

void recieve_markov_chain(std::size_t id, const marcov_chain_state_t &mcs) {
	std::cout << "recieve_markov_chain(" << id << ", {" << mcs.iteration << ", ["
		<< mcs.cfg_vector.data.size() << "], ...}) on " << workgroup_ptr->workgroup_id << std::endl;
	mc_chains_to_add.emplace(id, mcs);
	got_markov_chain();
}

void move_markov_chain(std::size_t id, int to) {
	std::cout << "move_markov_chain(" << id << ", " << to << ") on " << workgroup_ptr->workgroup_id << std::endl;
	if (workgroup_ptr->workgroup_id == to) {
		assert(0 == markov_chains_to_get);
		got_markov_chain(id);
	} else {
		std::cout << "XXX marcov_chains_to_move[" << id << "] = " << to << std::endl;
		marcov_chains_to_move[id] = to;
	}
}

bool send_progress_info = false;
void request_progress_info() {
	// std::cout << "request_progress_info() on " << workgroup_ptr->workgroup_id << std::endl;
	send_progress_info = true;
	assert(0 == markov_chains_to_get);
	markov_chains_to_get = 0;
	rescheduling_in_progress = true;
	// std::cout << "rescheduling started on wg " << workgroup_ptr->workgroup_id << std::endl;
}

void pass_progress_info(std::size_t id, std::size_t progress, int wg_id) {
	// std::cout << "request_progress_info(" << id << ", " << progress << ", " << wg_id
		// << ") on " << workgroup_ptr->workgroup_id << std::endl;
	assert(reschedule_manager);
	reschedule_manager->add_progress_for_id(id, progress, wg_id);
}

// TODO: rescheduling done

bool request_rescheduling()
{
	assert(reschedule_manager);
	assert(wg_leaders);

	if (reschedule_manager->is_rescheduling_in_progress()) {
		return false;
	}

	reschedule_manager->start_rescheduling();

	for (int i = 0; i < workgroup_ptr->workgroup_count; i++) {
		auto cmd = wg_leaders->rpmanager.prepare_call("request_progress_info");
		wg_leaders->rcg.schedule(cmd, i);
	}

	return true;
}


void add_new_mc_chain(std::size_t id, seed_t &mc_seed)
{
	auto mc_sseq = mc_seed.get_seed_seq();
	marcov_chain_state_t mc_state(cfg_vector_length, prn_engine_t(mc_sseq));
	
	auto search = mc_starting_with_special_cfg_vector.find(id);
	if (mc_starting_with_special_cfg_vector.end() != search) {
		(search->second)(mc_state.prn_engine, mc_state.cfg_vector);
	} else {
		default_cfg_generator(mc_state.prn_engine, mc_state.cfg_vector);
	}

	mc_chains_to_add.emplace(id, mc_state);

	{
		// type_abstraction_adapter_t bitsery_taa;
		// buffer_t b = bitsery_taa.serialize(mc_state);
		
		// marcov_chain_state_t mc_state_(cfg_vector_length, prn_engine_t());
		// assert(bitsery_taa.deserialize(b, mc_state_));
		// assert(mc_state == mc_state_);

		// reschedule_required = true;
		// auto id_fwidth = std::setw(std::to_string(num_markov_chains).length());
		// std::cout << "c id: " << id_fwidth << id << " with " << cfg_vector << std::endl;
		
		// type_abstraction_adapter_t bitsery_taa;
		// buffer_t b = bitsery_taa.serialize(cfg_vector);
		// cfg_vector_t cmp_cfg_vector(cfg_vector_length);
		// assert(bitsery_taa.deserialize(b, cmp_cfg_vector));
		// assert(cmp_cfg_vector == cfg_vector);
		// std::cout << "c id: " << id_fwidth << id << " with " << cmp_cfg_vector << std::endl;
	}

	{
		// auto prn1 = mc_state.prn_engine;
		// auto prn2 = mc_state.prn_engine;
		// simple_change_generator_t change_gen(mc_state.cfg_vector);
		
		// type_abstraction_adapter_t bitsery_taa;
		// buffer_t b = bitsery_taa.serialize(change_gen);
		// auto c1 = change_gen(prn1, mc_state.cfg_vector);
		// auto c2 = change_gen(prn1, mc_state.cfg_vector);
		// auto c3 = change_gen(prn1, mc_state.cfg_vector);
		// std::cout << c1.index << ": " << c1.new_value << std::endl;
		// std::cout << c2.index << ": " << c2.new_value << std::endl;
		// std::cout << c3.index << ": " << c3.new_value << std::endl;	

		// assert(bitsery_taa.deserialize(b, change_gen));
		// assert(c1 == change_gen(prn2, mc_state.cfg_vector));
		// assert(c2 == change_gen(prn2, mc_state.cfg_vector));
		// assert(c3 == change_gen(prn2, mc_state.cfg_vector));
	}
}

std::string filename_cnf_formula;
std::string filename_resolvents;
std::string data_directory;

int main(int argc, char **argv)
{
	auto start = std::chrono::high_resolution_clock::now();

	try {
		MPI_Init(&argc, &argv);

		{
			int is_initialized;
			MPI_Initialized(&is_initialized);
			assert(0 != is_initialized);
		}

		int version = 0;
		int subversion = 0;
		assert(MPI_SUCCESS == MPI_Get_version(&version, &subversion));
		if (0 == mpi_get_comm_rank(MPI_COMM_WORLD)) {
			std::cout << "c MPI Version " << version << "." << subversion << std::endl;
		} else {
			MPI_Barrier(MPI_COMM_WORLD);
		}

		filename_cnf_formula = "data/three_color_gnp_50vertices_p0.092_seed37_cnfgen.cnf";
		filename_resolvents = "data/three_color_gnp_50vertices_p0.092_seed37_cnfgen.resolvents";
		data_directory = "out/three_color_gnp_50vertices_p0.092_seed37_cnfgen/";

		std::deque<std::string> args;
		for (int i = 1; i < argc; i++) {
			args.push_back(argv[i]);
		}
		while (!args.empty()) {
			if (args.front() == "-r") {
				args.pop_front();
				assert(!args.empty() && "missing resolvents filename");
				filename_resolvents = args.front();
				args.pop_front();
			}
			else if (args.front() == "-c") {
				args.pop_front();
				assert(!args.empty() && "missing cnf_formula filename");
				filename_cnf_formula = args.front();
				args.pop_front();
			}
			else if (args.front() == "-d") {
				args.pop_front();
				assert(!args.empty() && "missing data_directory filename");
				data_directory = args.front();
				args.pop_front();
			}
			else if (args.front() == "-of") {
				args.pop_front();
				assert(!args.empty() && "missing offset factor");
				markov_chain_offset = std::stoi(args.front()) * num_markov_chains;
				args.pop_front();
			}
			else if (args.front() == "-o") {
				args.pop_front();
				assert(!args.empty() && "missing offset value");
				markov_chain_offset = std::stoi(args.front());
				args.pop_front();
			}
			else if (args.front() == "-l") {
				args.pop_front();
				assert(!args.empty() && "missing target length");
				markov_chain_target_length = std::stoi(args.front());
				args.pop_front();
			}
			else if (args.front() == "-n") {
				args.pop_front();
				assert(!args.empty() && "missing num markov chains");
				num_markov_chains = std::stoi(args.front());
				args.pop_front();
			}
			else if (args.front() == "-t") {
				args.pop_front();
				assert(!args.empty() && "missing T value");
				mcmc_T = (double) std::stoi(args.front());
				args.pop_front();
			}
			else if (args.front() == "-f") {
				args.pop_front();
				assert(!args.empty() && "missing num_reference_chains");
				num_reference_chains = std::stoi(args.front());
				args.pop_front();
			}
			else {
				std::cout << "unknown parameter: " << args.front() << std::endl;
				exit(EXIT_FAILURE);
			}
		}

		if (0 == mpi_get_comm_rank(MPI_COMM_WORLD)) {
			std::cout << "cfg filename_cnf_formula: " << filename_cnf_formula << std::endl;
			std::cout << "cfg filename_resolvents: " << filename_resolvents << std::endl;
			std::cout << "cfg data_directory: " << data_directory << std::endl;
			std::cout << "cfg num_reference_chains: " << num_reference_chains << std::endl;
			std::cout << "cfg num_markov_chains: " << num_markov_chains << std::endl;
			std::cout << "cfg markov_chain_offset: " << markov_chain_offset << std::endl;
			std::cout << "cfg markov_chain_target_length: " << markov_chain_target_length << std::endl;
			std::cout << "cfg mcmc_T: " << mcmc_T << std::endl;
			std::cout << "num_reference_chains: " << num_reference_chains << std::endl;
			
			MPI_Barrier(MPI_COMM_WORLD);
		}

		// load sat and resolvents file
		probsat_modell_t probsat_modell(filename_cnf_formula, filename_resolvents); 
		cfg_vector_length = probsat_modell.resolvents.get_num_resolvents();
		// cfg_vector_length = 10;

		// build mpi workgroups based on shared tmp directory
		const auto workgroup = mpi_shared_tmp_dir_workgroup(MPI_COMM_WORLD, "mcmc-sat");
		// const auto workgroup = mpi_shared_tmp_dir_workgroup(MPI_COMM_WORLD, "mcmc-sat" + std::to_string(mpi_get_comm_rank(MPI_COMM_WORLD) % 2));
		workgroup_ptr = &workgroup;
		// const auto workgroup = mpi_shared_tmp_dir_workgroup(MPI_COMM_WORLD, "numa" + std::to_string(mpi_get_comm_rank(MPI_COMM_WORLD)));
		// const auto workgroup = mpi_shared_tmp_dir_workgroup(MPI_COMM_WORLD, mpi_get_comm_rank(MPI_COMM_WORLD) < 3 ? "numa1" : "numa2");
		std::cout << "c parent node " << workgroup.parent_comm_id + 1 << " / " << workgroup.parent_comm_size
				  << " has workgroup rank " << workgroup.workgroup_comm_id + 1 << " / " << workgroup.workgroup_comm_size
				  << " in workgoup " << workgroup.workgroup_id + 1 << " / " << workgroup.workgroup_count << std::endl;

		std::random_device rdev; std::size_t random_nonce = rdev();
		probsat_modell.workgroup_description = "wg" + std::to_string(workgroup.workgroup_id + 1) + "_r" + std::to_string(random_nonce);
		//probsat_modell.workgroup_description = "wg" + std::to_string(workgroup.workgroup_id + 1);

		// register functions
		wg_rpmanager.add_function("stop_running", stop_running);
		wg_rpmanager.add_function("execute_probsat", execute_probsat);
		wg_rpmanager.add_function("modell_identity", modell_identity<prec_t>);
		wg_rpmanager.add_function("modell_multiply", modell_multiply<prec_t>);


		// TODO parse config
		constant_configuration_generator<cfg_vector_t> cgen_false(false);
		constant_configuration_generator<cfg_vector_t> cgen_true(true);
		cfg_generator_t cfg_generator_const_false = cfg_generator_t(cgen_false);
		cfg_generator_t cfg_generator_const_true = cfg_generator_t(cgen_true);
		// mc_starting_with_special_cfg_vector[0] = cfg_generator_const_false;
		//mc_starting_with_special_cfg_vector[1] = cfg_generator_const_false;
		//mc_starting_with_special_cfg_vector[2] = cfg_generator_const_true;
		// mc_starting_with_special_cfg_vector[3] = cfg_generator_const_true;
		for (std::size_t i = 0; i < num_reference_chains; i++) {
			mc_starting_with_special_cfg_vector[i] = cfg_generator_const_false;
			mc_starting_with_special_cfg_vector[num_reference_chains+i] = cfg_generator_const_true;
		}

		acc_prob_comp_t acc_prob_fct(mcmc_T);

		// global seed
		if (0 == workgroup.parent_comm_id)
		{
			std::string seed_filename = data_directory + "/global_seed.txt";
			global_seed = get_persistent_global_seed<seed_t>(seed_filename);
			std::cout << "c global seed: " << global_seed.short_rep() << "..." << std::endl;
		}
		assert(MPI_SUCCESS == MPI_Bcast(
			global_seed.seed_data.data(), global_seed.seed_data.size(),
			get_mpi<seed_t::base_t>::type(), 0, workgroup.parent_comm));


		if (workgroup.is_workgroup_head())
		{ // distributed generation of markov chain seeds
			auto sseq = global_seed.get_seed_seq();
			prn_engine_t seed_engine(sseq);
			seed_type_generator_t sgen;

			const std::size_t num_workgroups = workgroup.workgroup_count;
			const std::size_t workgroup_id = workgroup.workgroup_id;

			std::size_t num_markov_chains_per_work_group = num_markov_chains / num_workgroups;
			std::size_t num_markov_chains_in_this_work_group = num_markov_chains_per_work_group
				+ ((workgroup_id < num_markov_chains % num_workgroups) ? 1 : 0);

			std::size_t markov_chains_id_offset = ((workgroup_id > (num_markov_chains % num_workgroups))
				? workgroup_id - (num_markov_chains % num_workgroups) : 0) * num_markov_chains_per_work_group
				+ std::min(workgroup_id, num_markov_chains % num_workgroups) * (num_markov_chains_per_work_group + 1);
			std::size_t local_markov_chain_offset = markov_chain_offset + markov_chains_id_offset;

			std::cout << "c work group " << workgroup_id << " has " << num_markov_chains_in_this_work_group
				<< " markov chains at id " << markov_chains_id_offset << " (offset " << local_markov_chain_offset << ")" << std::endl;

			// skip seeds for markov_chain_offset markov chains
			for (std::size_t i = 0; i < local_markov_chain_offset; i++) {
				seed_t mc_seed = seed_t(seed_engine, &sgen);
				ignore(mc_seed);
			}

			auto id_fwidth = std::setw(std::to_string(num_markov_chains).length());
			std::size_t mv_id_start = local_markov_chain_offset; // markov_chains_id_offset
			std::size_t mv_id_end = mv_id_start + num_markov_chains_in_this_work_group;
			for (std::size_t id = mv_id_start; id < mv_id_end; id++) {
				seed_t mc_seed = seed_t(seed_engine, &sgen);
				std::cout << "c markov chain " << id_fwidth << id << " with seed "
					<< std::setw(13) << mc_seed.short_rep() << "..."
					<< "\t" << "(on workgroup " << workgroup_id << ")" << std::endl;

				add_new_mc_chain(id, mc_seed);
			}
		}

		// if (0 == workgroup.parent_comm_id) {
			// std::cout << "continue?" << std::endl;
			// std::string s;
			// std::cin >> s;
			// MPI_Barrier(MPI_COMM_WORLD);
		// }

		const bool use_mc_limit = 0 < markov_chain_target_length;

		using priority_id_pair_t = std::pair<std::size_t, std::size_t>;
		auto priority_cmp = [use_mc_limit](priority_id_pair_t left, priority_id_pair_t right) {
			// in case limit is set: sort by work to do, else sort after work done
			return use_mc_limit ? left.first < right.first : left.first > right.first;
		};
		std::priority_queue<
			priority_id_pair_t,
			std::vector<priority_id_pair_t>,
			decltype(priority_cmp)
		> work_queue(priority_cmp);

		std::map<std::size_t, metropolis_hastings_algorithm_t> mh_algorithms;




		remote_execution_context_t wg_remote_exec_context(wg_rpmanager);
		on_demand_scheduler wg_scheduler(workgroup.workgroup_comm, 3, wg_remote_exec_context);

		if (workgroup.is_workgroup_head()) {
			assert(0 == wg_scheduler.env_comm_id); // otherwise renumbering would be required
			
			wg_leaders = std::make_unique<wg_leader_exec_env>(workgroup.mpi_comm_leaders, 4);

			if (1 < workgroup.workgroup_count) {
				if (0 == workgroup.workgroup_id) {
					reschedule_manager = std::make_unique<rescheduling_manager>(
						workgroup.workgroup_count, num_markov_chains, use_mc_limit);
					wg_leaders->rpmanager.add_function("request_rescheduling", request_rescheduling);
					wg_leaders->rpmanager.add_function("pass_progress_info", pass_progress_info);
				}

				wg_leaders->rpmanager.add_function("request_progress_info", request_progress_info);
				wg_leaders->rpmanager.add_function("move_markov_chain", move_markov_chain);
				wg_leaders->rpmanager.add_function("recieve_markov_chain", recieve_markov_chain);
			}


/* WG Leaders Test
			wg_leaders->rpmanager.add_function("test_rcg", test_rcg);

			for (int i = 0; i < workgroup.workgroup_count; i++) {
				auto cmd = wg_leaders->rpmanager.prepare_call("test_rcg", workgroup.workgroup_id, i);

				std::function<void()> result_fct = [i, &workgroup]() {
					std::cout << "wg " << workgroup.workgroup_id << " got ok for " << i << std::endl;
				};
				cmd = wg_leaders->remote_exec_context.on_result(cmd, result_fct);

				wg_leaders->rcg.schedule(cmd, i);
			}

			while (wg_leaders->rcg.is_work_outstanding()) {
				wg_leaders->rcg.do_work();
			}

			MPI_Barrier(workgroup.mpi_comm_leaders);
			if (0 == workgroup.workgroup_id) {
				std::cout << "wg leader test finished" << std::endl;
			}
*/
		}

		std::function<void(std::size_t, int, int)> do_rescheduling =
			[](std::size_t id, int from, int to) {
				auto cmd = wg_leaders->rpmanager.prepare_call("move_markov_chain", id, to);
				wg_leaders->rcg.schedule(cmd, from);
		};


/*
		for (auto & [id, mc_state] : mc_chains_to_add) {
			const bool already_done = use_mc_limit && (mc_state.iteration >= markov_chain_target_length);

			if (already_done) {
				std::cout << "c markov chain " << id << " has already reached"
					<< " computation limit of length " << markov_chain_target_length << std::endl;
			} else {
				std::size_t priority = use_mc_limit
					? markov_chain_target_length - mc_state.iteration
					: mc_state.iteration;

				mh_algorithms.emplace(id, metropolis_hastings_algorithm_t(id, mc_state, probsat_modell, acc_prob_fct));
				// assert(mc_state == mh_algorithms.at(id).get_last_state());

				work_queue.push(std::make_pair(priority, id));
			}
		}*/

		

		std::size_t actual_working_time = 0;
		decltype(std::chrono::high_resolution_clock::now()) start_work, end_work;
		if (workgroup.is_workgroup_head())
		{
			// bool restarted[2] = {false};
			// bool accepted[8] = {true, true, false, true, true, true, true, true};
			// std::size_t skip_from = 1;
			// std::size_t skip_amount = 4;
			do {
				// std::cout << workgroup.workgroup_id << " XXX " << "main loop start" << std::endl;
				
				if (!mc_chains_to_add.empty()) {
					// std::cout << workgroup.workgroup_id << " XXX " << "mc_chains_to_add" << std::endl;
					
					for (auto & [id, mc_state] : mc_chains_to_add) {
						const bool already_done = use_mc_limit && (mc_state.iteration >= markov_chain_target_length);

						if (already_done) {
							// std::cout << "c moved markov chain " << id << " has already reached"
								// << " computation limit of length " << markov_chain_target_length << std::endl;
						} else {
							assert(mc_state.cfg_vector.data.size() == cfg_vector_length);
							// std::cout << "c adding mc " << id << " in iteration " << mc_state.iteration << std::endl;

							std::size_t priority = use_mc_limit
								? markov_chain_target_length - mc_state.iteration
								: mc_state.iteration;

							mh_algorithms.emplace(id, metropolis_hastings_algorithm_t(id, mc_state, probsat_modell, acc_prob_fct));

							work_queue.push(std::make_pair(priority, id));
						}
					}

					mc_chains_to_add.clear();
				}

				// std::cout << workgroup.workgroup_id << " XXX " << "main loop p1" << std::endl;

				if (!work_queue.empty()) {
					// std::cout << workgroup.workgroup_id << " XXX " << "!work_queue empty" << std::endl;
					bool was_moved = false;
					do {
						const auto [priority, id] = work_queue.top();
						was_moved = !mh_algorithms.contains(id);
						if (was_moved) { work_queue.pop(); }
					} while ((!work_queue.empty()) && (was_moved));
				}

				std::size_t iteration = 0;
				if (work_queue.empty()) {
					// std::cout << workgroup.workgroup_id << " XXX " << "work_queue empty" << std::endl;
					running = rescheduling_in_progress || wg_leaders->rcg.is_work_outstanding() || !marcov_chains_to_move.empty();
				} else {
					// std::cout << workgroup.workgroup_id << " XXX " << "process work" << std::endl;
					/*
					const auto [priority, id] = work_queue.top();
					work_queue.pop();
					
					// std::cout << "c processing mc " << id << " with priority " << priority << std::endl;
					
					bool is_last_iteration = use_mc_limit && (1 == priority);
					
					assert(mh_algorithms.contains(id));
					metropolis_hastings_algorithm_t &mha = mh_algorithms.at(id);
					*/
					std::size_t priority, id;
					std::queue<std::pair<std::size_t, std::size_t>> tmp_queue;
					while (true) {
						const auto [priority_, id_] = work_queue.top();
						priority = priority_; id = id_;
						work_queue.pop();
						if (work_queue.empty()) {
							break;
						}

						assert(mh_algorithms.contains(id));
						metropolis_hastings_algorithm_t &mha = mh_algorithms.at(id);
						if (mha.still_waiting_for_computation()) {
							tmp_queue.push(std::make_pair(priority, id));
						} else {
							break;
						}
					}

					while (!tmp_queue.empty()) {
						work_queue.push(tmp_queue.front());
						tmp_queue.pop();
					}
					
					// if (mha.can_start_next_iteration()) {
						// if (restarted[id]) {
							// if (skip_from == mha.get_iteration()) {
								// for (std::size_t i = skip_from; i < skip_from + skip_amount; i++) {
									// std::cout << "skipping " << i << std::endl;
									// mha.skip_computation(accepted[i]);
								// }
							// }
						// }
					// }
					
					//iteration = mha(wg_scheduler, is_last_iteration);
					//wg_scheduler.do_work();

					assert(mh_algorithms.contains(id));
					metropolis_hastings_algorithm_t &mha = mh_algorithms.at(id);

					bool is_last_iteration = use_mc_limit && (1 == priority);
					iteration = mha(wg_scheduler, is_last_iteration);
					wg_scheduler.do_work();

					// if (mha.can_start_next_iteration()) {
						// marcov_chain_state_t mcs = mha.get_last_state();
						// if (1 == mcs.iteration) {
							// if (!restarted[id]) {
								// std::cout << "restart " << id << std::endl;
								// restarted[id] = true;
								// mha.cleanup();
								// mh_algorithms.erase(id);
								// mh_algorithms.emplace(id, metropolis_hastings_algorithm_t(id, mcs, probsat_modell, acc_prob_fct));
							// }
						// }
					// }

					std::size_t new_priority = use_mc_limit
						? markov_chain_target_length - iteration
						: iteration;

					bool is_done = use_mc_limit && (0 == new_priority);
					if (is_done) {
						mha.cleanup();
						mh_algorithms.erase(id);
						marcov_chains_finished.insert(id);
						// TODO: inform wg 0
					} else {
						work_queue.push(std::make_pair(new_priority, id));
					}
				}

				// std::cout << workgroup.workgroup_id << " XXX " << "done with work, communicate" << std::endl;

				wg_leaders->rcg.do_work();

				if (!marcov_chains_to_move.empty()) {
					// std::cout << workgroup.workgroup_id << " XXX " << "mc to move" << std::endl;
					for (auto const& [id, new_wg] : marcov_chains_to_move) {
						// std::cout << "MC TO MOVE: " << id << " on " << workgroup.workgroup_id << std::endl;
						auto search = marcov_chains_finished.find(id);
						if (search != marcov_chains_finished.end()) {
							std::cout << "moving dummy for id " << id << " id to " << new_wg << std::endl;

							marcov_chain_state_t mcs(cfg_vector_length);
							// buffer_t b = serialize_mcs(mcs);
							auto cmd = wg_leaders->rpmanager.prepare_call("recieve_markov_chain", id, mcs);
							wg_leaders->rcg.schedule(cmd, new_wg);
						} else {
							std::cout << "prepare movement for " << id << " on " << workgroup.workgroup_id << std::endl;
							assert(mh_algorithms.contains(id));
							metropolis_hastings_algorithm_t &mha = mh_algorithms.at(id);
							while (!mha.can_start_next_iteration()) {
								bool is_last_iteration = use_mc_limit && (markov_chain_target_length == iteration);
								iteration = mha(wg_scheduler, is_last_iteration);
								wg_scheduler.do_work();
								wg_leaders->rcg.do_work();
							}

							// std::cout << "get last state for " << id << " on " << workgroup.workgroup_id << std::endl;
							marcov_chain_state_t mcs = mha.get_last_state();

							{
								// buffer_t b = serialize_mcs(mcs);
								auto cmd = wg_leaders->rpmanager.prepare_call("recieve_markov_chain", id, mcs);
								wg_leaders->rcg.schedule(cmd, new_wg);
							}

							{
								mha.cleanup();
								mh_algorithms.erase(id);
							}
						}
					}
					marcov_chains_to_move.clear();
				}

				// std::cout << workgroup.workgroup_id << " XXX " << "loop p 622" << std::endl;

				if (send_progress_info) {
					// std::cout << workgroup.workgroup_id << " XXX " << "send_progress_info" << std::endl;
					for (auto const& [id, mh_alg] : mh_algorithms) {
						std::size_t num_it = mh_alg.get_iteration();
						std::size_t progress = num_it; // use_mc_limit ? markov_chain_target_length - num_it : num_it;

						// if (0 == workgroup.workgroup_id) {
							// pass_progress_info(id, progress, workgroup.workgroup_id);
						// } else {
						auto cmd = wg_leaders->rpmanager.prepare_call("pass_progress_info", id, progress, workgroup.workgroup_id);
						wg_leaders->rcg.schedule(cmd, 0);
						// }
					}

					send_progress_info = false;
				}

				// std::cout << workgroup.workgroup_id << " XXX " << "loop p641" << std::endl;

				if ((0 == workgroup.workgroup_id) && (1 < workgroup.workgroup_count)) {
					static bool move_requested = false;
					if ((!move_requested) && (2 == iteration)) {
						if (!disable_rescheduling) {
							request_rescheduling();
							move_requested = true;
						}
					}

					assert(reschedule_manager);
					if (reschedule_manager->is_rescheduling_in_progress()) {
						if (reschedule_manager->information_complete()) {
							std::cout << "rescheduling algorithm start" << std::endl;
							reschedule_manager->reschedule(do_rescheduling);
							std::cout << "rescheduling algorithm end" << std::endl;
						}
					}
				}

				wg_leaders->rcg.do_work();
				// std::cout << workgroup.workgroup_id << " XXX " << "main loop end" << std::endl;

			} while (running);
			
			if (!disable_rescheduling) {
				std::cout << "rescheduling_in_progress: " << rescheduling_in_progress << std::endl;
				std::cout << "marcov_chains_to_move.size(): " << marcov_chains_to_move.size() << std::endl;
			}
			
			std::cout << "wg " << workgroup.workgroup_id << " is done running" << std::endl;

			while (wg_scheduler.is_work_outstanding() || wg_leaders->rcg.is_work_outstanding()) {
				wg_leaders->rcg.do_work();
				wg_scheduler.do_work();
			}

			auto stop_call = wg_rpmanager.prepare_call("stop_running");
			for (int i = 1; i < workgroup.workgroup_comm_size; i++) {
				wg_scheduler.schedule(stop_call, i);
			}
		} else {
			start_work = std::chrono::high_resolution_clock::now();
			while (running) {
				actual_working_time += wg_scheduler.do_work(true);
			}
			end_work = std::chrono::high_resolution_clock::now();
		}


		auto end = std::chrono::high_resolution_clock::now();
		auto overall_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		std::cout << "c duration: " << overall_duration.count() << " milliseconds" << std::endl;

		if (!workgroup.is_workgroup_head()) {
			auto work_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_work - start_work);
			auto init_duration = overall_duration.count() - work_duration.count();
			auto efficiency = 100.0 * (double) actual_working_time / (double) work_duration.count();
			
			std::cout << "c init: " << init_duration << " ms, work: " << work_duration.count() << " ms with "
				<< std::setprecision(2) << std::fixed << efficiency << " % efficiency" << std::endl;
		}

		MPI_Barrier(MPI_COMM_WORLD);

		/*
	} catch (const std::exception& ex) {
		std::cout << ex.what() << std::endl;
	}

	try {
	*/
		{
			int is_initialized;
			MPI_Initialized(&is_initialized);
			assert(is_initialized);
			
			MPI_Barrier(MPI_COMM_WORLD);
			MPI_Finalize();
			
			int is_finalized;
			MPI_Finalized(&is_finalized);
			assert(is_finalized);
		}
	} catch (const std::exception& ex) {
		std::cout << ex.what() << std::endl;
	}

	return EXIT_SUCCESS;
}
