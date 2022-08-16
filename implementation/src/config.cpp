
#include "config.hpp"

// regarding the statistical acceptance computation
/// scaling factor of the confidence intervals
double cfg_sac_confidence_interval_scaling_factor = 1.0;

/// confidence intervals smaller than this value are replaced with their average
double cfg_sac_testniveau = 0.01;

/// number of initial values required to compute confidence intervals
std::size_t cfg_sac_num_initial_values = 2400;

/// values which are added in each iteration the confidence intervals is improved
std::size_t cfg_sac_num_additional_values_per_iteration = 240;

/// maximum number of improvement iterations
std::size_t cfg_sac_max_iterations = 12;

bool cfg_print_debug_execution_statistics = false;
