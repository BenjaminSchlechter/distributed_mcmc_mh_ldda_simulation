#ifndef PROBSAT_EXECUTION_HPP
#define PROBSAT_EXECUTION_HPP


#include <chrono>
#include <string>
#include <iostream>
#include <cstdio>
#include <cstddef>
#include <sys/select.h>

#include "../../../config.hpp"
#include "../../../util/util.hpp"

#include "../../../util/basic_serialization.hpp"


/// probsat execution results
namespace probsat_return_cause {

	/// probsat execution return code
	enum reason : uint8_t {
		ERROR = 0,
		TIMEOUT = 1,
		MAX_FLIPS= 2,
		SUCCESS = 3,
		NUM_REASONS = 4
	};

	/// convert probsat return code into string
	const static std::string as_string[reason::NUM_REASONS] = {
		"ERROR",
		"TIMEOUT",
		"MAX_FLIPS",
		"SUCCESS"
	};

	/// serialization of probsat return cause
	template<typename S>
	void serialize(S &s, reason &r) {
		uint8_t v = r;
		generic_serialize(s, v);
		assert(v <= probsat_return_cause::NUM_REASONS);
		r = static_cast<reason>(v);
	}
};


/// execute probsat with the given parameters
uint8_t execute_probsat_(
	uint64_t &num_flips,
	const std::string &filename,
	const probsat_seed_t seed,
	const uint64_t max_flips = 0,
	const std::chrono::seconds &max_exec_time
		= std::chrono::seconds::zero(),
	std::string *debug_probsat_output = nullptr);


/// function wrapper for remote probsat execution
std::pair<uint64_t, probsat_return_cause::reason>
	execute_probsat(std::string filename, probsat_seed_t seed);


/// class to gather statistics about probsat executions per markov chain
template<typename prec_t = prec_t>
struct result_statistics
{
	/// number of executions
	std::size_t probsat_executions = 0;

	/// count return reasons
	std::size_t reasons[probsat_return_cause::NUM_REASONS] = {0};

	/// number of total flips executed
	double total_flips_executed = 0;


	/// add a result to the statistic
	prec_t operator()(
		std::pair<uint64_t, probsat_return_cause::reason> result)
	{
		prec_t num_flips = static_cast<prec_t>(result.first);
		probsat_executions++;
		assert(result.second < probsat_return_cause::NUM_REASONS);
		reasons[result.second]++;

		if (std::numeric_limits<uint64_t>::max() == num_flips) {
			num_flips = std::numeric_limits<prec_t>::quiet_NaN();
		} else {
			total_flips_executed += num_flips;
		}

		if ((interpret_timeout_as_max_flips_reached)
			&& (0 < probsat_max_flips))
		{
			if (result.second == probsat_return_cause::TIMEOUT) {
				num_flips = probsat_max_flips;
			}
		}

		if (result.second != probsat_return_cause::SUCCESS) {
			std::cerr << "probsat executed with " <<
				probsat_return_cause::as_string[result.second]
				<< " and " << num_flips << " flips" << std::endl;
		}

		// return std::log(num_flips);
		return num_flips;
	};

	/// print statistics
	friend std::ostream &operator<<(
		std::ostream &os, const result_statistics &rs)
	{
		namespace prc = probsat_return_cause;
		// new version is easier parseable from the command line
		os << rs.probsat_executions << " executions, "
			<< rs.reasons[prc::SUCCESS] << " successfull, "
			<< rs.reasons[prc::MAX_FLIPS] << " max_flips_reached, "
			<< rs.reasons[prc::TIMEOUT] << " terminated_by_timeout, "
			<< rs.reasons[prc::ERROR] << " with_errors, "
			<< rs.total_flips_executed << " flips_in_total_(at_least)";
/* old version
		os << rs.probsat_executions << " executions ("
			<< rs.reasons[prc::SUCCESS] << " successfull, "
			<< rs.reasons[prc::MAX_FLIPS] << " max flips reached, "
			<< rs.reasons[prc::TIMEOUT] << " terminated by timeout, "
			<< rs.reasons[prc::ERROR] << " with errors, "
			<< "and at least " <<
				rs.total_flips_executed << " flips in total)";
*/
		return os;
	}

};


#endif
