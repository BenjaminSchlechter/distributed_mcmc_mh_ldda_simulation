#ifndef ACCEPTANCE_PROPABILITY_HPP
#define ACCEPTANCE_PROPABILITY_HPP

#include <string>

/// compute an acceptance probability used by the metropolis hastings algorithm
template <typename prec_t>
class exponential_acceptance_probability
{
	private:
		/// exponential falloff (new - old = T (old was better) => P(accept) = 1/e)
		prec_t T;

	public:
		using prec_type = prec_t;
		constexpr static bool is_acceptance_probability_implementation = true;

		std::string get_description() {
			return std::string("exp-T-") + std::to_string(T);
		}

		/// constructor
		exponential_acceptance_probability(prec_t T_) : T(T_) {}

		/// compute acceptance probability given a new and old value
		prec_t operator()(prec_t old_value, prec_t new_value) const {
			prec_t result = exp( -(new_value - old_value) / T );
			result = std::max(0.0, std::min(result, 1.0));
			
			// std::cout << "new: " << new_value << std::endl;
			// std::cout << "old: " << old_value << std::endl;
			// std::cout << "x: " << (new_value - old_value) << std::endl;
			// std::cout << "result: " << result << std::endl;

			return result;
		}
};

#endif
