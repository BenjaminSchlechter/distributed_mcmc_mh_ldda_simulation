#ifndef SIMPLE_ACCEPTANCE_COMPUTATION_HPP
#define SIMPLE_ACCEPTANCE_COMPUTATION_HPP


#include <random>
#include <cassert>
#include <functional>

#include "acceptance_probability.hpp"

#include "../../util/seed.hpp"
#include "../../config.hpp"

/// decides over acceptance of a new state in a markov chain using a exact value
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
class simple_acceptance_computation
{
	private:
		/// probability distribution to get cutoff values for acceptance
		std::uniform_real_distribution<prec_t> prob_distribution;

		/// compute and represent a state of a markov chain
		typedef struct {
			/// data to compute each value
			value_comp_t::prepared_computation pcom;

			/// pseudo random number generator used in this step
			prng_t prng;

			/// computed value of this markov chain state
			prec_t value;
		} state_data_t;

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


		/// derive a pseudo random number generator for each step
		auto derive_prng(prng_t &main_prng) {
			seed_t seed = seed_t(main_prng);
			auto sseq = seed.get_seed_seq();
			return prng_t(sseq);
		}
		
	public:
		constexpr static bool is_acceptance_computation = true;

		std::string get_description() {
			return std::string("sac");
		}

		/// constructor
		simple_acceptance_computation() :
			prob_distribution(0.0, 1.0),
			state_data(),
			is_first_cfg_vector(true),
			accept(false),
			new_index(0),
			computation_scheduled_state(false),
			remaining_computations(0)
		{}


		/// skip one step for fast state recovery of random number generator
		void skip_computation(prng_t &main_prng) {
			derive_prng(main_prng);
		}


		/// schedule computation of new state value
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

			state_data[new_index].prng = derive_prng(main_prng);

			result_function_t result_fct = [&](auto... results){
				remaining_computations--;
				auto result = result_projection(results...);
				static_assert(
					std::is_same<prec_t, decltype(result)>::value
					&& "result_projection has wrong result type!");
				state_data[new_index].value = result;
			};

			value_comp.prepare_computation_with_cfg_vector(
				v, state_data[new_index].pcom);

			auto cmd = value_comp(
				state_data[new_index].prng,
				scheduler.rec.rpmanager,
				state_data[new_index].pcom);
			cmd = scheduler.rec.on_result(cmd, result_fct);

			remaining_computations++;
			scheduler.schedule(cmd);
		}


		/// check whether this computation is still outstanding
		bool still_waiting_for_computation() {
			return (0 < remaining_computations);
		}


		/// call this function to continue the computation until it returns true
		template<typename scheduler_t>
		bool continue_computation(
			scheduler_t &scheduler,
			value_comp_t &value_comp,
			acc_prob_comp_t &acc_prob_fct)
		{
			assert(computation_scheduled_state);
			ignore(&scheduler, &value_comp, &acc_prob_fct);
			return 0 == remaining_computations;
		}


		/// finish the computation to know wheter to accept the new state
		bool finish_computation(
			value_comp_t &value_comp, acc_prob_comp_t &acc_prob_fct)
		{
			assert(0 == remaining_computations);
			assert(computation_scheduled_state);
			computation_scheduled_state = false;

			value_comp.finish_computation(state_data[new_index].pcom);

			uint8_t old_index = 1 - new_index;
			if (is_first_cfg_vector) {
				is_first_cfg_vector = false;
				new_index = old_index;
				accept = true;
				return true;
			} else {
				prec_t z = prob_distribution(state_data[new_index].prng);

				prec_t p = acc_prob_fct(
					state_data[old_index].value,
					state_data[new_index].value);

				bool accept = (z <= p);
				if (accept) {
					new_index = old_index;
				}

				return accept;
			}
		}


		/// finish any outstanding computations
		void cleanup(value_comp_t &value_comp) {
			ignore(value_comp);
		}


		/// get the value of the current state
		prec_t get_current_value() {
			assert(!computation_scheduled_state);
			uint8_t index = 1 - new_index;
			return state_data[index].value;
		}

		/// get the value of the last computed state
		prec_t get_last_computed_value() {
			assert(!computation_scheduled_state);
			uint8_t index = accept ? 1 - new_index : new_index;
			return state_data[index].value;
		}

		/// get the value of the previous state
		prec_t get_previous_computed_value() {
			assert(!computation_scheduled_state);
			uint8_t index = new_index;
			return state_data[index].value;
		}

/*
		prec_t get_value() {
			assert(!computation_scheduled_state);
			return state_data[1 - new_index].value;
		}
*/
};

#endif
