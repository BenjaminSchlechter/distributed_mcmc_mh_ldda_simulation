#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include <cmath>

/** \brief calculate multiple statistic metrics efficiently at once
 * 
 * Calculates minimum, maximum, average, standard deviation, variance,
 * sum and count in O(n). Non finite numbers are counted separately
 * and are excluded in the metrics. Updates are efficiently possible!
 * The math for standard deviation and variance is from:
 * https://coderodde.wordpress.com/2016/04/12/computing-standard-deviation-in-one-pass/
 * 
 * 
 */
template<typename prec_t>
class basic_statistical_metrics
{
	public:
		/// number of finite numbers counted
		std::size_t n;

		/// sum if all numbers
		prec_t sum_xi;

		/// sum if all squared numbers
		prec_t sum_xi_sq;

		/// variance
		prec_t varianz;

		/// standard deviation
		prec_t stddev;

		/// arithmetic mean (average)
		prec_t average;

		/// minimum
		prec_t min;

		/// maximum
		prec_t max;

		/// count of excluded non finite numbers
		std::size_t not_finite; 
		
		/// constructor
		basic_statistical_metrics() :
			n(0),
			sum_xi(0),
			sum_xi_sq(0),
			varianz(0),
			stddev(0),
			average(0),
			min(std::numeric_limits<prec_t>::max()),
			max(std::numeric_limits<prec_t>::min()),
			not_finite(0) {}
		
		/// overall counted numbers, finite and non finite
		std::size_t counted() {
			return n + not_finite;
		}
		
		/// Iterator to add numbers in range to the statistic
		template <typename it_t>
		void operator ()(it_t begin, it_t end)
		{
			for (auto i = begin; i != end; i++)
			{
				if (std::isfinite(*i)) {
					min = std::min(min, *i);
					max = std::max(max, *i);
					n++;
					prec_t x = *i;
					sum_xi += x;
					sum_xi_sq += x*x;
				} else {
					not_finite++;
				}
			}

			average = sum_xi / n;
			varianz = (sum_xi_sq - sum_xi*sum_xi / n) / (n - 1);
			stddev = sqrt(varianz);
		}
};

// prec_t mean = std::accumulate(list.begin(), list.end(), (prec_t) 0.0) / list.size();

#endif
