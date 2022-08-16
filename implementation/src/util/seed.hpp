#ifndef SEED_HPP
#define SEED_HPP


#include <iostream>
#include <random>
#include <array>
#include <iterator>
#include <cassert>
#include <fstream>


/// wrapper to initialize multiple seeds from one distribution
template<typename base_type>
struct seed_type_generator
{
	/// seed distribution
	std::uniform_int_distribution<base_type> dist;
	
	/// constructor
	seed_type_generator() :
		dist(
			std::numeric_limits<base_type>::min(),
			std::numeric_limits<base_type>::max()
		)
	{}
};


/// generic seed sequence for random generators
template<std::size_t size, typename base_type>
class seed_type
{
	private:
		/// initialize this seed from a random device using a distribution
		template<typename rdev_t>
		void init(
			rdev_t &rng_dev,
			seed_type_generator<base_type> *sgen_ptr = nullptr)
		{
			seed_type_generator<base_type> sgen;
			sgen_ptr = sgen_ptr ? sgen_ptr : &sgen;
			for (std::size_t i = 0; i < seed_data.size(); i++) {
				seed_data[i] = sgen.dist(rng_dev);
			}
		}

	public:
		/// seed sequence number type
		using base_t = base_type;

		/// seed sequence data
		std::array<base_type, size> seed_data;

		/// provides access to the seed sequence
		inline auto get_seed_seq() {
			return std::seed_seq(
				std::begin(seed_data), std::end(seed_data));
		}

		/// (default) constructor with unitialized or random seed
		seed_type(bool generate_random_seed = false) : seed_data()
		{
			if (generate_random_seed) {
				std::random_device rdev;
				init(rdev);
			}
		}
		
		/// construct seed from string
		seed_type(std::string seed_str) : seed_data() {
			std::stringstream sstr(seed_str);
			sstr >> *this;
		}

		/// construct using a random number generator and a seed_type_generator
		template<typename rdev_t> requires // hack to check for RNG:
			(!std::is_same<void, typename rdev_t::result_type>::value)
		seed_type(
			rdev_t &rng_dev,
			seed_type_generator<base_type> *sgen = nullptr
		) :
			seed_data()
		{
			init(rng_dev, sgen);
		}

		/// short representation
		base_type short_rep() {
			return seed_data[0];
		}

		/// compare operator for seeds
		bool operator==(const seed_type<size, base_type> &rhs) const {
			return seed_data == rhs.seed_data;
		}

		/// compare operator for seeds		
		bool operator!=(const seed_type<size, base_type> &rhs) const {
			return seed_data != rhs.seed_data;
		}

		/// std::ostream output operator for seeds
		friend std::ostream &operator<<(
			std::ostream &os, const seed_type &st)
		{
			std::copy(
				st.seed_data.begin(),
				st.seed_data.end(),
				std::ostream_iterator<base_type>(os, " "));
			return os;
		}

		/// std::istream input operator for seeds
		friend std::istream &operator>>(std::istream &is, seed_type &st)
		{
			for (std::size_t i = 0; i < st.seed_data.size(); i++) {
				base_type v;
				is >> v;
				st.seed_data[i] = v;
			}

			return is;
		}
};


/// bitsery serialization of seeds
template <typename S, std::size_t size, typename base_type>
void serialize(S& s, seed_type<size, base_type> &seed)
{
	std::size_t sz = size;
	std::size_t bts = sizeof(base_type);

	static_assert(8 == sizeof(std::size_t));

	s.value8b(sz);
	s.value8b(bts);
	s.container(seed.seed_data, size);

	assert(size == sz);
	assert(sizeof(base_type) == bts);
}


/// manage a global random but persistent seed in a file
template<typename seed_t>
auto get_persistent_global_seed(std::string seed_filename)
{
	std::fstream seed_file(
		seed_filename, std::ios::in | std::ios::out | std::ios::app);
	assert(seed_file.is_open());

	seed_file.seekp(0, seed_file.end);
	const auto fsize = seed_file.tellp();

	// generate a random seed
	seed_t global_seed = seed_t(true);

	if (0 == fsize) {
		// and save it if none was available
		std::cout << "c writing seed" << std::endl;
		seed_file << global_seed;
		seed_file.flush();
		assert(seed_file.good());
	} else {
		// or load if one had been saved before
		seed_file.seekg(0, seed_file.beg);
		std::cout << "c reading seed" << std::endl;
		seed_file >> global_seed;
		// assert(seed_file.eof());
		assert(seed_file.tellg() + 1 == fsize);
	}

	assert(!seed_file.bad());
	seed_file.close();

	return global_seed;
}


#endif
