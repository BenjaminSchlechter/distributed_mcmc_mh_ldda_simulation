#ifndef MULTIWAY_PARTITIONING_HPP
#define MULTIWAY_PARTITIONING_HPP

#include <cstddef>
#include <functional>
#include <list>
#include <iostream>
#include <cassert>
#include <random>

/** \brief class to solve the multiway number partitioning problem
 * 
 * Multi-way partitioning using the largest differencing method.
 * This is an efficient approximation with statistically better
 * results than a greedy algorithm. Implementation based upon:
 * https://en.wikipedia.org/wiki/Largest_differencing_method
 * https://en.wikipedia.org/wiki/Multiway_number_partitioning
 */
template<
	typename data_t,
	typename score_t = std::size_t,
	class sort_func_t = std::greater<score_t>>
class multiway_partitioning
{
	public:

		/// single entry associating a score with some data
		struct entry_t {
			/// score of the entry (used for partitioning, i.e. workload progress)
			score_t score;

			/// associated data (i.e. workload identifier)
			data_t data;
		};


		/// subset of a partition
		struct subset_t {

			/// sum of scores of all elements
			score_t sum;
			
			/// list of elements in this subset
			std::list<entry_t> elements;

			/// default constructor
			subset_t() : sum(0), elements() {}

			/// construct subset of size one by passing one entry
			subset_t(entry_t first_entry) :
				sum(first_entry.score), elements({first_entry}) {}

			/// merge two subsets
			void merge_subsets(subset_t &other, auto cmp_function)
			{
				sum += other.sum;
				// merge_two_lists(elements, other.elements);
				elements.sort(cmp_function);
				other.elements.sort(cmp_function);
				elements.merge(other.elements, cmp_function);
			}

			/// std::ostream operator to print a subset
			friend std::ostream &operator<<(
				std::ostream &os, const subset_t &s)
			{
				os << "{";
				if (!s.elements.empty()) {
					bool first = true;
					for (auto el : s.elements) {
						if (!first) { os << ", "; }
						os << el.score;
						first = false;
					}
				}
				os << "} [S: " << s.sum
					<< ", l: " << s.elements.size() << "]";
				return os;
			}
		};


		/// a simple partition containing multiple subsets
		struct partitioning_t {
			/// subsets of this partition
			std::vector<subset_t> subsets; // number of partitions = subset.size

			/// minimal sum of entry scores in any subset contained
			score_t min_sum;

			/// maximum sum of entry scores in any subset contained
			score_t max_sum;

			/** \brief difference between minimum and maximum sum
			 *         of scores in any subset contained
			 * 
			 * if partition size is less than the target number of partitions
			 * its required by the algorithm to assume subsets with size zero
			 * are contained
			 */
			auto difference(std::size_t num_partitions) const
			{
				if (subsets.size() < num_partitions) {
					return max_sum - std::min(min_sum, (std::size_t) 0);
				}
				return max_sum - min_sum;
			}

			/// construct an empty partition
			partitioning_t(std::size_t num_partitions)
				: subsets(), min_sum(0), max_sum(0)
			{
				subsets.reserve(num_partitions);
			}

			/// construct a partition with one subset
			partitioning_t(std::size_t num_partitions, auto first_subset) :
				subsets(),
				min_sum(std::min((score_t) 0, first_subset.sum)),
				max_sum(std::max((score_t) 0, first_subset.sum))
			{
				subsets.reserve(num_partitions);
				subsets.push_back(first_subset);
			}

			/// merge two partitions
			void merge_parts(
				partitioning_t &other,
				std::size_t num_partitions,
				auto cmp_function)
			{
				// simulate balanced partitioning, might be suboptimal!
				auto subset_cmp_f = [](subset_t &l, subset_t &r) {
					if (l.elements.size() != r.elements.size()) {
						return l.elements.size() > r.elements.size();
					} else {
						return l.sum < r.sum;
					}
				};

				std::sort(subsets.begin(), subsets.end(), subset_cmp_f);
				std::sort(
					other.subsets.begin(),
					other.subsets.end(),
					subset_cmp_f);

				std::size_t ossize = other.subsets.size();
				std::size_t tssize = subsets.size();
				std::size_t end =
					std::min(num_partitions, subsets.size() + ossize);
				std::size_t start = end - ossize;
				std::size_t border = std::min(end, tssize);

				assert(other.subsets.size() <= end);
				assert(other.subsets.size() >= end-start);
				assert(subsets.size() >= border);

				subsets.resize(end);
				
				min_sum = std::numeric_limits<score_t>::max();
				max_sum = std::numeric_limits<score_t>::min();

				for (std::size_t i = 0; i < start; i++) {
					min_sum = std::min(min_sum, subsets[i].sum);
					max_sum = std::max(max_sum, subsets[i].sum);
				}

				for (std::size_t i = start; i < border; i++) {
					std::size_t other_i = end - i - 1;
					subsets[i].merge_subsets(
						other.subsets[other_i], cmp_function);
					min_sum = std::min(min_sum, subsets[i].sum);
					max_sum = std::max(max_sum, subsets[i].sum);
				}

				for (std::size_t i = border; i < end; i++) {
					std::size_t other_i = end - i - 1;
					subsets[i] = other.subsets[other_i];
					min_sum = std::min(min_sum, subsets[i].sum);
					max_sum = std::max(max_sum, subsets[i].sum);
				}
			}

			/// sort subsets in this partition by the sum of scores of their elements
			void sort(auto sort_func)
			{
				auto cmp_f =
					[sort_func](const subset_t &l, const subset_t &r)
				{
					return sort_func(l.sum, r.sum);
				};

				std::sort(subsets.begin(), subsets.end(), cmp_f);
			}

			/// modified sort to consider set size and sum of scores in each subset
			void final_sort(auto sort_func)
			{
				auto cmp_f =
					[sort_func](const subset_t &l, const subset_t &r)
				{
					if (l.elements.size() != r.elements.size()) {
						return l.elements.size() < r.elements.size();
					} else {
						return sort_func(l.sum, r.sum);
					}
				};

				std::sort(subsets.begin(), subsets.end(), cmp_f);
			}

			/// print a partition to std::ostream
			friend std::ostream &operator<<(
				std::ostream &os, const partitioning_t &p)
			{
				os << "(";
				bool first = true;
				for (auto s : p.subsets) {
					if (!first) { os << "; "; }
					os << s;
					first = false;
				}
				// warning: capacity should be correct,
				// but num_partitions is not available
				os << ") [D: "
					<< p.difference(p.subsets.capacity()) << "?]";
				return os;
			}
		};

		/// list of partitionings for the largest differencing method
		std::list<partitioning_t> dataset;

		/// number of target partitions
		std::size_t num_partitions;

		/// function to compare scores
		sort_func_t sort_func;

		/// constructor
		multiway_partitioning(
			std::size_t num_partitions,
			auto sort_func = std::greater<score_t>()
		) :
			dataset(),
			num_partitions(num_partitions),
			sort_func(sort_func)
		{
			assert(1 < num_partitions);
		}

		/// reset object to initial state
		void reset(std::size_t n_partitions) {
			num_partitions = n_partitions;
			dataset.clear();
		}

		/// add an element
		void add(auto score, auto data) {
			dataset.push_back(
				partitioning_t(num_partitions, subset_t({score, data})));
		}

		/// sort datasets (initial step after dataset is complete)
		void sort()
		{
			auto cmp_f =
				[this](const partitioning_t &l, const partitioning_t &r)
			{
				return sort_func(
					l.difference(num_partitions),
					r.difference(num_partitions));
			};

			// std::sort(dataset.begin(), dataset.end(), cmp_f);
			dataset.sort(cmp_f);
		}
		
		/// returns true if only one partition is left
		bool is_finished() {
			return 1 == dataset.size();
		}

		/** \brief execute one step of the largest differencing method
		 * 
		 * each iteration fuses two partitions in the dataset until
		 * one partition is left.
		 */
		bool iterate()
		{
			if (1 < dataset.size()) {
				partitioning_t s1 = dataset.front();
				dataset.pop_front();
				partitioning_t s2 = dataset.front();
				dataset.pop_front();

				auto cmp_entry_f =
					[this](const entry_t &l, const entry_t &r)
				{
					return sort_func(l.score, r.score);
				};

				s1.merge_parts(s2, num_partitions, cmp_entry_f);
				std::list<partitioning_t> nel = { s1 };

				auto cmp_f =
				[this](const partitioning_t &l, const partitioning_t &r)
				{
					return sort_func(
						l.difference(num_partitions),
						r.difference(num_partitions));
				};
				dataset.merge(nel, cmp_f);
			}

			return is_finished();
		}
		
		/// execute the largest differencing method by iterating until done
		void partitionate() {
			while (false == iterate()) {}
		}
		
		/// returns the resulting partition
		auto result() {
			assert(is_finished());
			return dataset.front();
		}

		/// print all partitionings for debugging purpose
		friend std::ostream &operator<<(
			std::ostream &os, const multiway_partitioning &mp)
		{
			os << "multiway_partitioning for "
				<< mp.num_partitions << " partitions: ";

			os << "{\n";
			for (auto p : mp.dataset) {
				os << "\t" << p << "\n";
			}
			os << "}" << std::endl;

			return os;
		}
};

/// function to test the multiway partitioning
void test_multiway_partitioning()
{
	const bool print_steps_in_between = false;

	// config for first example
	std::size_t num_elements = 11;
	std::size_t num_partitions = 3;

	// followed by iterations-1 random tests
	std::size_t iterations = 1;	

	// random test configuration
	std::mt19937 prng(42);
	std::uniform_int_distribution<std::size_t> udistr(1, 9);
	std::uniform_int_distribution<std::size_t> nedistr(10, 100);
	std::uniform_int_distribution<std::size_t> npdistr(2, 8);

	for (std::size_t it = 0; it < iterations; it++) {
		if (0 < it) {
			num_elements = nedistr(prng);
			num_partitions = npdistr(prng);
		}

		std::cout << "sorting: "
			<< num_elements << " elements into "
			<< num_partitions << " partitions" << std::endl;
		using sort_t = std::greater<std::size_t>;
		// using sort_t = std::less<std::size_t>;
		multiway_partitioning<std::size_t, std::size_t, sort_t>
			mp(num_partitions, sort_t());

		for (std::size_t i = 0; i < num_elements; i++) {
			std::size_t s = udistr(prng);
			mp.add(s, i);
		}

		mp.sort();

		std::cout << mp;

		if constexpr (print_steps_in_between) {
			bool is_finished;
			do {
				is_finished = mp.iterate();
				std::cout << mp;
			} while (!is_finished);
			// std::cout << mp;
		} else {
			mp.partitionate();
		}

		auto result = mp.result();
		// result.sort(sort_t());
		
		result.final_sort(sort_t());
		std::cout << result << std::endl;

		std::size_t min_length = std::numeric_limits<std::size_t>::max();
		std::size_t max_length = std::numeric_limits<std::size_t>::min();
		for (const auto &sset : result.subsets) {
			std::size_t length = sset.elements.size();
			max_length = std::max(max_length, length);
			min_length = std::min(min_length, length);
		}

		std::cout << "lengths [" << min_length << ", "
			<< max_length << "]" << std::endl;
		assert(max_length >= min_length);
		assert(1 >= max_length - min_length);

		// result.improve_partitioning();
	}
}

#endif
