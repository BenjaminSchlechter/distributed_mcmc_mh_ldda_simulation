#ifndef CHANGE_GENERATOR_HPP
#define CHANGE_GENERATOR_HPP


#include <random>
#include <cassert>
#include <sstream>

#include "../../util/basic_serialization.hpp"


/// struct representing a single change at a config vector
template<typename size_type, typename value_t>
struct change {
	/// index of changed position
	size_type index;

	/// new value
	value_t new_value;

	/// equality test
	auto operator==(const change<size_type, value_t> &rhs) const {
		return (index == rhs.index) && (new_value == rhs.new_value);
	}

	/// inequality test
	auto operator!=(const change<size_type, value_t> &rhs) const {
		return (*this == rhs);
	}
};


/// class to generate changes in config vectors
template <
	typename cfg_vector_t,
	bool enforce_change,

	typename value_t = std::conditional<
		std::is_same<bool, typename cfg_vector_t::value_type>::value,
		short,
		typename cfg_vector_t::value_type
	>::type,

	typename value_distr_type = std::uniform_int_distribution<value_t>,

	decltype(value_distr_type().min()) value_distr_minv
		= cfg_vector_t::min_value,

	decltype(value_distr_type().max()) value_distr_maxv
		= enforce_change ?
			cfg_vector_t::max_value - 1 :
			cfg_vector_t::max_value
>
class simple_change_generator
{
	public:
		constexpr static bool is_change_generator = true;
		using index_t = cfg_vector_t::size_type;

	private:
		/// index distribution
		std::uniform_int_distribution<index_t> index_distr;

		/// value distribution
		value_distr_type value_distr;

	public:
		std::string get_description() {
			return enforce_change ? "scg-ec" : "scg-ac";
		}

		/// constructor
		simple_change_generator(const cfg_vector_t &v) :
			index_distr(0, v.data.size() - 1),
			value_distr(value_distr_minv, value_distr_maxv)
		{
			assert(0 < v.data.size());
			static_assert(
				cfg_vector_t::min_value < cfg_vector_t::max_value);
		}

		/// create change for a given config vector
		auto operator()(auto &prng, const cfg_vector_t &v)
		{
			index_t index = index_distr(prng);
			value_t value = value_distr(prng);

			if constexpr(enforce_change) {
				if (value >= v.data[index]) {
					value++;
				}
			}

			return change<index_t, typename cfg_vector_t::value_type>{
				index,
				static_cast<cfg_vector_t::value_type>(value)
			};
		}
};


/// serialize the state of a simple change generator
template <typename S, typename simple_change_generator_t>
	requires (simple_change_generator_t::is_change_generator)
void serialize(S& s, simple_change_generator_t &change_gen) {
	std::string descr = change_gen.get_description();
	s.text1b(descr, 7);
	assert(6 == descr.length());
	assert(change_gen.get_description() == descr);

	std::stringstream sstr;
	sstr << change_gen.index_distr;
	sstr << change_gen.value_distr;
	std::string distributions = sstr.str();
	assert(!sstr.fail());

	generic_serialize(s, distributions);

	sstr.str(distributions);
	sstr >> change_gen.index_distr;
	sstr >> change_gen.value_distr;
	assert(!sstr.fail());
	assert(sstr.eof());
}


#endif
