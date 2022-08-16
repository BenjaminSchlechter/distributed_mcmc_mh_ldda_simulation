#ifndef RESCHEDULING_HPP
#define RESCHEDULING_HPP

#include <cstddef>
#include <functional>
#include <vector>
#include <cassert>
#include <iostream>

#include "../util/util.hpp"
#include "multiway_partitioning.hpp"

/// class is used to reschedule workload between workgroups
class rescheduling_manager
{
	private:
		/// rescheduling state
		bool rescheduling_required;

		/// number of workgroups
		int num_workgroups;

		/// number of markov chains which are to be scheduled
		std::size_t number_of_active_mcs;

		std::function<bool(std::size_t, std::size_t)> sort_func;

		/// use multiway partitioning (largest differencing method) for rescheduling
		multiway_partitioning<
			std::pair<std::size_t, int>,
			std::size_t,
			std::function<bool(std::size_t, std::size_t)>> mwpart;

		/// current workload on each workgroup
		std::vector<std::size_t> workload;

	public:
		/// constructor
		rescheduling_manager(
			int num_workgroups,
			std::size_t num_mcs,
			bool use_mc_limit,
			bool rescheduling_required = false
		) :
			rescheduling_required(rescheduling_required),
			num_workgroups(num_workgroups),
			number_of_active_mcs(num_mcs),
			sort_func(std::less<std::size_t>()),
			mwpart(num_workgroups, sort_func),
			workload(num_workgroups)
		{ assert(0 < num_workgroups); ignore(use_mc_limit); }


		/// start a new scheduling by reseting internal state
		void start_rescheduling()
		{
			assert(!rescheduling_required);

			mwpart.reset(num_workgroups);

			workload.reserve(num_workgroups);
			for (int i = 0; i < num_workgroups; i++) {
				workload[i] = 0;
			}

			rescheduling_required = true;
		}


		/// checks rescheduling state
		bool is_rescheduling_in_progress() const
		{
			return rescheduling_required;
		}


		/// add progress of a markov chain on a workgroup after start of rescheduling
		void add_progress_for_id(
			std::size_t id, std::size_t progress, int wg_id)
		{
			assert(rescheduling_required);
			assert(0 <= wg_id);

			std::cout << "adding reschedule progress " << progress
				<< " for " << id << " on " << wg_id << std::endl;

			// progress_list.insert(id, std::make_pair(progress, wg_id));
			mwpart.add(progress, std::make_pair(id, wg_id));

			workload[wg_id] += progress;
		}


		/// remove a markov chain from the scheduling if finished
		void remove_mc(std::size_t id)
		{
			assert(rescheduling_required);

			ignore(id);
			if (!rescheduling_required) {
				number_of_active_mcs--;
			}
		}

		/// check if all progress information for rescheduling is available
		bool information_complete() const
		{
			assert(rescheduling_required);
			return mwpart.dataset.size() >= number_of_active_mcs;
		}

		/// check if rescheduling is appropriate
		bool is_rescheduling_unnecessary(
			std::size_t reschedule_requires_absolute_difference) const
		{
			assert(rescheduling_required);
			assert(information_complete());

			auto [min, max] =
				std::minmax_element(begin(workload), end(workload));

			std::size_t diff = max - min;
			bool got_required_abs_diff =
				diff >= reschedule_requires_absolute_difference;

			return !got_required_abs_diff;
		}

		/// cancel a rescheduling (i.e. if unnecessary)
		void cancel_rescheduling(auto do_rescheduling)
		{
			// assert(rescheduling_required);
			// assert(information_complete());
			rescheduling_required = false;
			for (int target_wg = 0;
				target_wg < num_workgroups; target_wg++)
			{
				do_rescheduling(0, target_wg, target_wg);
			}
		}

		/// perform a rescheduling
		void reschedule(auto do_rescheduling)
		{
			assert(rescheduling_required);
			// assert(progress_list.size() == number_of_active_mcs);
			assert(mwpart.dataset.size() == number_of_active_mcs);

			// std::cout << "before rescheduling:\n" << mwpart;

			mwpart.sort();
			mwpart.partitionate();
			auto result = mwpart.result();
			// result.sort(sort_t());
			// todo: improve? (i.e. with complete Karmarkar–Karp algorithm)
			// an anytime algorithm would be possible as well
			result.final_sort(sort_func);

			// todo: find optimal mapping

			// std::cout << "after rescheduling:\n" << mwpart;

			rescheduling_required = false;

			const auto [min, max] =
				std::minmax_element(begin(workload), end(workload));
			std::size_t original_diff = *max - *min;

			// std::cout << "rescheduling from diff: "
				// << original_diff << " [" << *min << ", " << *max << "] "
				// << " to " << result.difference(num_workgroups)
				// << " [" << result.min_sum << ", "
				// << result.max_sum << "]" << std::endl;

			if (result.difference(num_workgroups) >= original_diff) {
				// std::cout << "rescheduling probably unnecessary!" << std::endl;
				cancel_rescheduling(do_rescheduling);
				return;
			}

			assert(result.subsets.size() == (std::size_t) num_workgroups);
			auto it = result.subsets.begin();
			for (int target_wg = 0;
				target_wg < num_workgroups; target_wg++)
			{
				std::size_t mcs_to_get = 0;
				for (auto entry : it->elements) {
					if (entry.data.second != target_wg) {
						std::size_t id = entry.data.first;
						int original_wg = entry.data.second;
						// std::cout << "move mc " << id
							// << " with progress " << entry.score
							// << " from workgroup " << original_wg
							// << " to " << target_wg << std::endl;
						do_rescheduling(id, original_wg, target_wg);
						mcs_to_get++;
					}
				}

				do_rescheduling(mcs_to_get, target_wg, target_wg);
				it++;
			}
			assert(it == result.subsets.end());
		}
};


#endif
