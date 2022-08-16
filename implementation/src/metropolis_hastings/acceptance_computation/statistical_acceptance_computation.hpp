#ifndef STATISTICAL_ACCEPTANCE_COMPUTATION_HPP
#define STATISTICAL_ACCEPTANCE_COMPUTATION_HPP

#include <random>
#include <cassert>
#include <functional>

#include "acceptance_probability.hpp"

#include "../../util/statistics.hpp"
#include "../../util/seed.hpp"
#include "../../config.hpp"

/// compute coincidence intervall of acceptance probability
template<typename prec_t, class acc_prob_comp_t>
auto compute_p_range(
	const prec_t cif,
	acc_prob_comp_t &acc_prob_fct,
	const basic_statistical_metrics<prec_t> &new_statistics,
	const basic_statistical_metrics<prec_t> &old_statistics)
{
	prec_t new_value = new_statistics.average;
	prec_t old_value = old_statistics.average;

	prec_t new_sigma = new_statistics.stddev;
	prec_t old_sigma = old_statistics.stddev;

	prec_t new_iv[2] = {
		new_value + cif*new_sigma, new_value - cif*new_sigma};
	prec_t old_iv[2] = {
		old_value + cif*old_sigma, old_value - cif*old_sigma};

	// std::cout << "\t" << "statistics:" << std::endl;
	// std::cout << "\t" << "new_iv: [" << new_iv[0] << ", "
		// << new_value << ", " << new_iv[1] << "]" << std::endl;
	// std::cout << "\t" << "old_iv: [" << old_iv[0] << ", "
		// << old_value << ", " << old_iv[1] << "]" << std::endl;

	prec_t max_x[2] = {
		std::max(new_iv[0], new_iv[1]),
		std::min(-old_iv[0], -old_iv[1])};
	prec_t min_x[2] = {
		std::min(new_iv[0], new_iv[1]),
		std::max(-old_iv[0], -old_iv[1])};

	prec_t p[2] = {
		acc_prob_fct(-max_x[1], max_x[0]),
		acc_prob_fct(-min_x[1], min_x[0])};

	prec_t p_max = std::max(p[0], p[1]);
	prec_t p_min = std::min(p[0], p[1]);
	prec_t p_avg = acc_prob_fct(old_value, new_value);

	/* debug output
	std::cout << "\t" << "stddev: [" << new_statistics.stddev << ", "
	 	<< old_statistics.stddev << "]" << std::endl;
	std::cout << "\t" << "new range: [" << new_statistics.min << ", "
		<< new_statistics.max << "]" << std::endl;
	std::cout << "\t" << "old range: [" << old_statistics.min << ", "
		<< old_statistics.max << "]" << std::endl;
	std::cout << "\t" << "n: [" << new_statistics.n << ", "
	 	<< old_statistics.n << "]" << std::endl;
	std::cout << "\t" << "new_iv: [" << new_iv[0] << ", "
		<< new_value << ", " << new_iv[1] << "]" << std::endl;
	std::cout << "\t" << "old_iv: [" << old_iv[0] << ", "
	 	<< old_value << ", " << old_iv[1] << "]" << std::endl;
	std::cout << "\t" << "max_x: [" << max_x[0] << ", "
		<< max_x[1] << "]" << std::endl;
	std::cout << "\t" << "min_x: [" << min_x[0] << ", "
	 	<< min_x[1] << "]" << std::endl;
	std::cout << "\t" << "x: [" << max_x[0] + max_x[1] << ", "
	 	<< new_value - old_value << ", "
	 	<< min_x[0] + min_x[1] << "]" << std::endl;
	std::cout << "\t" << "r: [" << p[0] << ", "
	 	<< p[1] << "]" << std::endl;
	std::cout << "\t" << "new: [" << new_value << ", "
	 	<< new_sigma << "]" << std::endl;
	std::cout << "\t" << "old: [" << old_value << ", "
	 	<< old_sigma << "]" << std::endl;
	std::cout << "\t" << "p: [" << p_min << ", "
	 	<< p_avg << ", " << p_max << "]" << std::endl;
	std::cout << std::endl;
	*/

	assert(p_min <= p_avg);
	assert(p_avg <= p_max);

	return std::make_tuple(p_avg, p_min, p_max);
}


/// decides over acceptance of a new markov chain state based on a coincidence intervall
template <
	typename prng_t,
	typename cfg_vector_t,
	typename prec_t,
	typename value_comp_t,
	typename acc_prob_comp_t
		= exponential_acceptance_probability<prec_t>>
requires (
	value_comp_t::is_value_computation_implementation
	&& acc_prob_comp_t::is_acceptance_probability_implementation
)
class statistical_acceptance_computation
{
	private:
		/// probability distribution to get cutoff values for acceptance
		std::uniform_real_distribution<prec_t> prob_distribution;

		/// compute a state of a markov chain
		typedef struct {
			/// data to compute each value
			value_comp_t::prepared_computation pcom;

			/// pseudo random number generator used in this step
			prng_t prng;

			/// number of computed values
			std::vector<prec_t> values;

			/// how often more values have been requested
			std::size_t iteration;

			/// statistical properties of computed values
			basic_statistical_metrics<prec_t> statistics;
		} state_data_t;

		/// next acceptance probability cutoff value
		prec_t nextz;

		/// acceptance probability cutoff value
		prec_t z;


		/// markov chain state information of current and next state
		state_data_t state_data[2];

		/// application state to accept the first state anyway
		bool is_first_cfg_vector;

		/// last result
		bool accept;

		/// index into state_data of the next/new state
		uint8_t new_index;


		/// application state whether a value computation is scheduled
		bool computation_scheduled_state;

		/// number of remaining computations
		std::size_t remaining_computations;


		/// sigma factor for confidence
		double coincidence_interval_factor;

		/// probability cutoff off small coincidence intervalls to use average instead
		double testniveau;

		/// number of values to add in each iteration
		std::size_t values_per_iteration;

		/// maximum number of iterations until average is used
		std::size_t max_iterations;

		/// minimum number of initial values required for decision
		std::size_t values_required;

		/// internally used function to schedule computations
		std::function<
			void(void *scheduler_ptr, std::size_t i, uint8_t index)
		> schedule_computation;


		/// derive a pseudo random number generator for each step
		auto derive_prng(prng_t &main_prng) {
			// std::cout << "derive prng: " << main_prng << std::endl;
			seed_t seed = seed_t(main_prng);
			// std::cout << "seed: " << seed << std::endl;
			auto sseq = seed.get_seed_seq();
			// std::cout << "sseq: ";
			// sseq.param(std::ostream_iterator<int>(std::cout, ", "));
			// std::cout << std::endl;
			return prng_t(sseq);
		}

	public:
		constexpr static bool is_acceptance_computation = true;

		std::string get_description() {
			return std::string("cac");
		}

		/// constructor
		statistical_acceptance_computation(
				// prng_t &prng,
				// double coincidence_interval_factor = 1.0,
				// double testniveau = 0.005,
				// std::size_t values_per_iteration = 2400,
				// std::size_t max_iterations = 100,
				// std::size_t values_required = 10000

				// testing
				// double coincidence_interval_factor = 1.0,
				// double testniveau = 0.01,
				// std::size_t values_per_iteration = 240*2,
				// std::size_t max_iterations = 12,
				// std::size_t values_required = 2400

				// configurable
				double coincidence_interval_factor = cfg_sac_confidence_interval_scaling_factor,
				double testniveau = cfg_sac_testniveau,
				std::size_t values_per_iteration = cfg_sac_num_additional_values_per_iteration,
				std::size_t max_iterations = cfg_sac_max_iterations,
				std::size_t values_required = cfg_sac_num_initial_values

				// works
				// double coincidence_interval_factor = 1.0,
				// double testniveau = 0.01,
				// std::size_t values_per_iteration = 240*2,
				// std::size_t max_iterations = 100,
				// std::size_t values_required = 10000
		) :
			prob_distribution(0.0, 1.0),
			nextz(0),
			z(0),
			state_data(),
			is_first_cfg_vector(true),
			accept(false),
			new_index(0),
			computation_scheduled_state(false),
			remaining_computations(0),
			// main_prng(prng),
			coincidence_interval_factor(coincidence_interval_factor),
			testniveau(testniveau),
			values_per_iteration(values_per_iteration),
			max_iterations(max_iterations),
			values_required(values_required),
			schedule_computation(nullptr)
		{}


		// statistical_acceptance_computation(
			// statistical_acceptance_computation &&other) = default;
		// statistical_acceptance_computation(
			// const statistical_acceptance_computation &other) = delete;

		// ~statistical_acceptance_computation() {}


		/// skip one step for fast state recovery of random number generator
		void skip_computation(prng_t &main_prng) {
			derive_prng(main_prng);
		}


		/// setup computation of new state values
		template<typename scheduler_t,
			typename result_projection_t = std::identity,
			typename result_function_t = std::function<void(prec_t)>>
		void start_computation(
			prng_t &main_prng,
			const cfg_vector_t &v,
			scheduler_t &scheduler,
			value_comp_t &value_comp,
			result_projection_t &result_projection = {})
		{
			assert(!computation_scheduled_state);
			computation_scheduled_state = true;

			ignore(&scheduler);


			// std::cout << "main_prng: " << main_prng << std::endl;
			state_data[new_index].prng = derive_prng(main_prng);
			// std::cout << "derived prng: "
				// << state_data[new_index].prng() << ", "
				// << state_data[new_index].prng() << ", "
				// << state_data[new_index].prng() << ", "
				// << state_data[new_index].prng << std::endl;

			prob_distribution.reset();
			z = nextz;
			nextz = prob_distribution(state_data[new_index].prng);
			// std::cout << "start_computation: z: "
				// << z << ", v: " << v << std::endl;

			value_comp.prepare_computation_with_cfg_vector(
				v, state_data[new_index].pcom);

			state_data[new_index].iteration = 0;
			state_data[new_index].statistics =
				basic_statistical_metrics<prec_t>();

			// reset statistics but not already computed values
			uint8_t old_index = 1 - new_index;
			state_data[old_index].statistics =
				basic_statistical_metrics<prec_t>();
			accept = false;

			schedule_computation =
				[&](void *scheduler_ptr, std::size_t i, uint8_t index)
			{
				scheduler_t &scheduler =
					*(static_cast<scheduler_t *>(scheduler_ptr));

				result_function_t result_fct
					= [&, i, index](auto... results)
				{
					remaining_computations--;
					auto result = result_projection(results...);
					static_assert(
						std::is_same<prec_t, decltype(result)>::value
						&& "result_projection has wrong result type!");
					// std::cout << "c R state_data[" << (int) index
						// << "].values[" << i << "] = "
						// << result << std::endl;
					state_data[index].values[i] = result;
				};

				auto cmd = value_comp(
					state_data[index].prng,
					scheduler.rec.rpmanager,
					state_data[index].pcom);
				cmd = scheduler.rec.on_result(cmd, result_fct);

				remaining_computations++;
				scheduler.schedule(cmd);
			};
		}


		/// check if all scheduled value computations finished
		bool still_waiting_for_computation() {
			return (0 < remaining_computations);
		}


		/// compute values until decision is clear (recall until it returns true)
		template<typename scheduler_t>
		bool continue_computation(
			scheduler_t &scheduler,
			value_comp_t &value_comp,
			acc_prob_comp_t &acc_prob_fct)
		{
			assert(computation_scheduled_state);
			ignore(&value_comp);

			if (is_first_cfg_vector) {
				return true; // nothing to do
			}

			if (0 < remaining_computations) {
				return false; // not yet done
			}

			uint8_t old_index = 1 - new_index;
			uint8_t indices[2] = {new_index, old_index};

			// update statistics
			std::size_t num_values =
				state_data[new_index].iteration * values_per_iteration;
			if (0 < num_values) {
				for (auto j = 0; j < 2; j++) {
					uint8_t index = indices[j];

					auto it_start =
						state_data[index].values.begin()
						+ state_data[index].statistics.counted();
					auto it_end =
						state_data[index].values.begin() + num_values;
					state_data[index].statistics(it_start, it_end);

					#if DEBUG_ASSERTIONS
					assert(num_values
						== state_data[index].statistics.counted());
					#endif
				}

				// test for evidence of result
				std::size_t num_values_available =
					std::min(state_data[new_index].statistics.n,
					state_data[old_index].statistics.n);
				
				if (num_values_available >= values_required) {
					auto [p_avg, p_min, p_max] = compute_p_range(
						coincidence_interval_factor,
						acc_prob_fct,
						state_data[new_index].statistics,
						state_data[old_index].statistics);

					// std::cout << "\t" << "p: [" << p_min << ", "
						// << p_avg << ", " << p_max << "]" << std::endl;

					// test for statistical evidence of accept/decline
					if (z <= p_min) {
						// std::cout << "clear accept ("
							// << z << ", " << p_min << ")" << std::endl;
						accept = true;
						return true;
					}

					if (z > p_max) {
						// std::cout << "clear decline ("
							// << z << ", " << p_max << ")" << std::endl;
						accept = false;
						return true;
					}

					const bool max_iterations_reached =
						max_iterations <= state_data[new_index].iteration;
					if (max_iterations_reached) {
						// std::cout << "max iterations reached ("
							// << z << ", " << p_avg << ")" << std::endl;
						accept = (z <= p_avg); // false
						return true; // done with best guess
					}

					const bool testniveau_unterschritten =
						std::abs(p_max - p_min) < testniveau;
					if (testniveau_unterschritten) {
						// std::cout << "testniveau_unterschritten ("
							// << z << ", " << p_avg << ")" << std::endl;
						accept = (z <= p_avg); // false
						return true; // done with best guess
					}
				}
			}

			// not yet done => schedule next iteration(s) for more data
			const bool new_values_for_old_index_are_required =
				state_data[new_index].iteration
				>= state_data[old_index].iteration;

			assert(state_data[new_index].iteration
				<= state_data[old_index].iteration);

			for (auto j = 0;
				j < (new_values_for_old_index_are_required ? 2 : 1); j++)
			{
				uint8_t index = indices[j];
				state_data[index].iteration++;

				std::size_t num_values =
					state_data[index].iteration*values_per_iteration;
				if (num_values > state_data[index].values.size()) {
					state_data[index].values.resize(num_values);
				}

				for (std::size_t i = num_values - values_per_iteration;
					i < num_values; i++)
				{
					schedule_computation(
						static_cast<void *>(&scheduler), i, index);
				}
			}

			return false;
		}


		/** \brief get result of wheter to accept the new state
		 * 
		 * won't cleanup computation of accepted state, as it may be
		 * required to continue with it in the next iteration
		 */
		bool finish_computation(
			value_comp_t &value_comp, acc_prob_comp_t &acc_prob_fct)
		{
			assert(0 == remaining_computations);
			assert(computation_scheduled_state);
			computation_scheduled_state = false;

			ignore(&acc_prob_fct);

			uint8_t old_index = 1 - new_index;
			if (is_first_cfg_vector) {
				is_first_cfg_vector = false;
				new_index = old_index;
				return true;
			} else {
				if (accept) {
					// std::cout << "accept "
						// << state_data[new_index].statistics.average
						// << std::endl;
					value_comp.finish_computation(
						state_data[old_index].pcom);
					new_index = old_index;
				} else {
					// std::cout << "decline "
						// << state_data[new_index].statistics.average
						// << std::endl;
					// std::cout << "keep value "
						// << state_data[old_index].statistics.average
						// << std::endl;
					value_comp.finish_computation(
						state_data[new_index].pcom);
				}
				return accept;
			}
		}


		/// cleanup any unfinished computation
		void cleanup(value_comp_t &value_comp) {
			if (!is_first_cfg_vector) {
				uint8_t old_index = 1 - new_index;
				value_comp.finish_computation(state_data[old_index].pcom);
			}
		}


		/// get the value of the current state
		prec_t get_current_value() {
			assert(!computation_scheduled_state);
			uint8_t index = 1 - new_index;
			return (0 == state_data[index].statistics.n) ?
				NAN : state_data[index].statistics.average;
		}

		/// get the value of the last computed state
		prec_t get_last_computed_value() {
			assert(!computation_scheduled_state);
			uint8_t index = accept ? 1 - new_index : new_index;
			return (0 == state_data[index].statistics.n) ?
				NAN : state_data[index].statistics.average;
		}

		/// get the value of the previous state
		prec_t get_previous_computed_value() {
			assert(!computation_scheduled_state);
			uint8_t index = new_index;
			return (0 == state_data[index].statistics.n) ?
				NAN : state_data[index].statistics.average;
		}

/*
		prec_t get_value() {
			assert(!computation_scheduled_state);
			uint8_t index = accept ? 1 - new_index : new_index;
			return (0 == state_data[index].statistics.n) ?
			 	NAN : state_data[index].statistics.average;
		}
*/
};


#endif
