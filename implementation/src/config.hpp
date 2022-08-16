#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstddef>
#include <string>
#include <random>
#include <chrono>

/// precision type used for computations
using prec_t = double;
static_assert(std::numeric_limits<prec_t>::has_quiet_NaN);

// typedef std::mt19937 prn_engine_t;
// using seed_t = seed_type<prn_engine_t::state_size, uint32_t>;

/// type of the random number generator used
typedef std::mt19937 prn_engine_t;
// using seed_t = seed_type<prn_engine_t::state_size, uint32_t>;
// using seed_type_generator_t = seed_type_generator<typename seed_t::base_t>;


// regarding probsat
/// probsat command line
const std::string probsat_cmd
	= "./probSAT/probSAT --fct 0 --eps 0.9 --cb 2.06 --runs 1";

/// maximum number of flips per probsat execution (zero to disable limit)
constexpr uint64_t probsat_max_flips = 20'000'000;

/// maximum time for a single probsat execution (zero to disable limit)
constexpr const std::chrono::minutes probsat_max_exec_time{1};

/// interpretion of timeout (false => NaN & ignore value, true => count as max_flips)
constexpr const bool interpret_timeout_as_max_flips_reached = false;

/// seed type for probsat calls
using probsat_seed_t = int32_t;

// regarding the statistical acceptance computation
/// scaling factor of the confidence intervals
extern double cfg_sac_confidence_interval_scaling_factor;

/// confidence intervals smaller than this value are replaced with their average
extern double cfg_sac_testniveau;

/// number of initial values required to compute confidence intervals
extern std::size_t cfg_sac_num_initial_values;

/// values which are added in each iteration the confidence intervals is improved
extern std::size_t cfg_sac_num_additional_values_per_iteration;

/// maximum number of improvement iterations
extern std::size_t cfg_sac_max_iterations;


// other
/// print runtime statistics for performance debugging
extern bool cfg_print_debug_execution_statistics;

#endif

