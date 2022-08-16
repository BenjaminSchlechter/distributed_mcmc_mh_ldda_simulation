#ifndef CONFIGURATION_MANAGER_HPP
#define CONFIGURATION_MANAGER_HPP


#include <string>
#include <random>
#include <cassert>


/// initialize a configuration vector using a constant
template <typename cfg_vector_t>
class constant_configuration_generator
{
	public:
		constexpr static bool is_configuration_generator = true;

	private:
		using value_t = cfg_vector_t::value_type;

		/// constant value used for initialization
		value_t cvalue;

	public:
		std::string get_description() {
			return std::string("ci-") + std::to_string(cvalue);
		}

		/// constructor
		constant_configuration_generator(value_t constant_value)
			: cvalue(constant_value)
		{
			assert(cfg_vector_t::min_value <= constant_value);
			assert(cfg_vector_t::max_value >= constant_value);
		}

		/// apply initialization method to config vector
		void operator()(auto &prng, cfg_vector_t &v) const {
			ignore(prng);

			for (typename cfg_vector_t::size_type i = 0;
				i < v.data.size(); i++)
			{
				v.data[i] = cvalue;
			}
		}
};


/// initialize a configuration vector using random values
template <typename cfg_vector_t>
class uniform_configuration_generator
{
	private:
		using value_t = std::conditional<
			std::is_same<bool, typename cfg_vector_t::value_type>::value,
			short,
			typename cfg_vector_t::value_type>::type;

	public:
		constexpr static bool is_configuration_generator = true;

		std::string get_description() {
			return std::string("ui");
		}

		/// constructor
		uniform_configuration_generator() {}

		/// apply initialization method to config vector
		template <typename rng_t>
		void operator()(rng_t &prng, cfg_vector_t &v) const {
			std::uniform_int_distribution<value_t>
				distr(cfg_vector_t::min_value, cfg_vector_t::max_value);
			for (typename cfg_vector_t::size_type i = 0;
				i < v.data.size(); i++)
			{
				value_t value = distr(prng);
				v.data[i] = static_cast<cfg_vector_t::value_type>(value);
			}
		}
};


#endif
