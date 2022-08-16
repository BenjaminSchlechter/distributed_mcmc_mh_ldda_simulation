#ifndef ALGORITHM_HPP
#define ALGORITHM_HPP


#include <set>
#include <iostream>
#include <cassert>

#include "../config.hpp"


/// generic implementation of the metropolis hastings algorithm for one markov chain
template<
	typename marcov_chain_state_t,
	typename change_generator_t,
	typename acceptance_computation_t,
	typename computation_modell_t,
	typename acc_prob_comp_t,
	typename result_statistics_t,
	typename result_function_t,
	const bool enforce_change,
	const bool skip_unchanged_vectors
	>
class metropolis_hastings_algorithm
{
	private:
		using cfg_vector_t = marcov_chain_state_t::cfg_vector_type;

		/// id of the markov chain (for output)
		std::size_t id;

		/// current state of the markov chain
		marcov_chain_state_t mc_state;

		/// pseudo random number generator of last state
		decltype(mc_state.prn_engine) last_prng;

		/// change generator for the config vector
		change_generator_t change_gen;

		/// next config vector
		cfg_vector_t next_cfg_vector;

		/// acceptance computation
		acceptance_computation_t acc_computation;

		/// internal computation state
		bool is_processing;

		/// computation modell (i.e. probsat)
		computation_modell_t &computation_modell;

		/// acceptance function
		acc_prob_comp_t &acc_prob_fct;

		/// execution statistics of computation modell
		result_statistics_t execution_statistics;


		/// generate the next config vector
		void generate_next_cfg_vector()
		{
			next_cfg_vector = mc_state.cfg_vector;
			
			bool change_happened = enforce_change;
			do {
				using index_t = decltype(
					change_gen(mc_state.prn_engine,
					next_cfg_vector).index);

				// generate changes
				std::set<index_t> changed_positions;

				for (decltype(mcmc_num_changes) i = 0;
					i < mcmc_num_changes; i++)
				{
					auto cfg_change =
						change_gen(mc_state.prn_engine, next_cfg_vector);
					next_cfg_vector.data[cfg_change.index]
						= cfg_change.new_value;

					if ((!change_happened) && (skip_unchanged_vectors))
					{
						changed_positions.insert(cfg_change.index);
					}
				}

				// check if config vector is changed
				if ((!change_happened) && (skip_unchanged_vectors)) {
					for(auto p : changed_positions) {
						if (next_cfg_vector.data[p]
							!= mc_state.cfg_vector.data[p])
						{
							change_happened = true;
							break;
						}
					}
				}
			} while ((!change_happened) && (skip_unchanged_vectors));
		}

	public:
	
		/// constructor
		metropolis_hastings_algorithm(
			std::size_t id,
			auto mc_state_,
			computation_modell_t &computation_modell,
			acc_prob_comp_t &acc_prob_fct
		) :
			id(id),
			mc_state(mc_state_),
			last_prng(mc_state.prn_engine),
			change_gen(mc_state.cfg_vector),
			next_cfg_vector(mc_state.cfg_vector),
			acc_computation(/*mc_state.prn_engine*/),
			is_processing(false),
			computation_modell(computation_modell),
			acc_prob_fct(acc_prob_fct),
			execution_statistics()
		{}

		// metropolis_hastings_algorithm(
			// metropolis_hastings_algorithm &&other) = default;
		// metropolis_hastings_algorithm(
			// const metropolis_hastings_algorithm &other) = delete;

		// ~metropolis_hastings_algorithm() {}


		/// get snapshot of last state
		marcov_chain_state_t get_last_state() {
			assert(!is_processing);

			return marcov_chain_state_t(
				mc_state.iteration - 1, mc_state.cfg_vector, last_prng);
		}


		/// prepare an iteration
		template<typename scheduler_t>
		void prepare_iteration(scheduler_t &scheduler) {
			// std::cout << "prepare" << std::endl;
			assert(!is_processing);
			is_processing = true;

			last_prng = mc_state.prn_engine;
			// std::cout << "test prepare_iteration "
				// << mc_state.iteration << ": " << last_prng << std::endl;
			

			acc_computation.template start_computation
				<scheduler_t, result_statistics_t, result_function_t>
				(mc_state.prn_engine,
				next_cfg_vector,
				scheduler,
				computation_modell,
				execution_statistics);
		}


		/// check if still waiting or progress can be made
		bool still_waiting_for_computation() {
			return acc_computation.still_waiting_for_computation();
		}


		/// continue an iteration
		template<typename scheduler_t>
		bool continue_iteration(scheduler_t &scheduler) {
			// std::cout << "continue" << std::endl;
			assert(is_processing);
			bool is_finished = acc_computation.continue_computation(
				scheduler, computation_modell, acc_prob_fct);
			// scheduler.do_work();
			return is_finished;
		}


		/// finish an iteration
		void finish_iteration(bool last_iteration = false) {
			// std::cout << "finish" << std::endl;
			assert(is_processing);
			is_processing = false;

			bool accept = acc_computation.finish_computation(
				computation_modell, acc_prob_fct);
			if (accept) {
				mc_state.cfg_vector = next_cfg_vector;
			}

			if (1 == mc_state.iteration) {
				auto initial_value = accept ?
					acc_computation.get_previous_computed_value() :
					acc_computation.get_current_value();
				std::cout << "mc " << id
					<< " iteration " << mc_state.iteration
					<< " value " << initial_value
					<< " accepted " << initial_value << std::endl;
			}

			if (0 < mc_state.iteration) {
				std::cout << "mc " << id
					<< " iteration " << (mc_state.iteration + 1)
					<< " value " << acc_computation.get_current_value()
					<< (accept ? " accepted " : " declined ")
					<< acc_computation.get_last_computed_value()
					<< std::endl;
			}

			#if DEBUG_ASSERTIONS
			if (accept) {
				assert(mc_state.cfg_vector == next_cfg_vector);
			} else {
				assert((mc_state.cfg_vector != next_cfg_vector)
					|| !(skip_unchanged_vectors || enforce_change));
			}
			#endif

			if (last_iteration) {
				is_processing = true; // prevent further execution
				cleanup();
			} else {
				generate_next_cfg_vector();
			}

			mc_state.iteration++;
		}


		/// skip one step for fast forwarding
		void skip_computation(bool accept) {
			acc_computation.skip_computation(mc_state.prn_engine);
			if (accept) {
				mc_state.cfg_vector = next_cfg_vector;
			}
			generate_next_cfg_vector();
			mc_state.iteration++;
		}


		/// cleanup of unfinished computations to end markov chain
		void cleanup() {
			acc_computation.cleanup(computation_modell);
			if (cfg_print_debug_execution_statistics) {
				print_execution_statistic();
			}
		}


		/// check if iteration was finished and next can be started
		bool can_start_next_iteration() const {
			return !is_processing;
		}


		/// get current iteration
		std::size_t get_iteration() const {
			return mc_state.iteration;
		}


		/// print statistics
		void print_execution_statistic() {
			std::cout << "mc " << id << " execution_statistics: "
				<< execution_statistics << std::endl;
		}


		/// automate iteration calls to make progress
		template<typename scheduler_t>
		std::size_t operator()(
			scheduler_t &scheduler, bool last_iteration = false)
		{
			if (false == is_processing) {
				prepare_iteration(scheduler);
			} else {
				bool is_finished = continue_iteration(scheduler);
				if (is_finished) {
					finish_iteration(last_iteration);
				}
			}

			return mc_state.iteration;
		}
};

#endif
