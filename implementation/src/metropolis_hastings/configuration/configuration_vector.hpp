#ifndef CONFIGURATION_VECTOR_HPP
#define CONFIGURATION_VECTOR_HPP


#include <vector>
#include <type_traits>

#include "../../util/bitfield.hpp"
#include "../../util/basic_serialization.hpp"


/// wrapper for a binary configuration vector used in the MCMC simulation
template<bool use_bitfield = false>
struct binary_configuration_vector
{
	public:
		/// value type
		using value_type = bool;

		/// value range: minimum
		static constexpr value_type min_value = false;

		/// value range: maximum
		static constexpr value_type max_value = true;

		/// type of the vector representation (bitfield or std::vector)
		using vector_type =
			std::conditional< use_bitfield,
				bitfield<std::size_t>,
				std::vector<value_type>
			>::type;
		// using vector_type = std::vector<value_type>;

		/// size type of the configuration vector
		using size_type = decltype(vector_type().size());

		/// configuration vector data
		vector_type data;



		/// default constructor
		binary_configuration_vector() : data(0) {}

		/// construct binary config vector of size length
		binary_configuration_vector(size_type length) : data(length) {}


		/// copy constructor
		binary_configuration_vector(
			const binary_configuration_vector &other)
		{
			data = other.data;
		}


		/// swap function for copy and swap idiom
		friend void swap(
			binary_configuration_vector<use_bitfield> &l,
			binary_configuration_vector<use_bitfield> &r)
		{
			std::swap(l.data, r.data);
		}


		/// assignment operator
		binary_configuration_vector &operator=(
			const binary_configuration_vector<use_bitfield> &other)
		{
			data = other.data;
			return *this;
		}

		/*
		/// assignment operator by using swap
		binary_configuration_vector &operator=(
		 	const binary_configuration_vector<use_bitfield> &other)
		{
			swap(*this, other);
			return *this;
		}
		*/


		/// move semantics by using swap
		binary_configuration_vector(binary_configuration_vector &&other)
		{
			swap(*this, other);
		}


		/// stream operator to print binary config vector to std::ostream
		friend std::ostream &operator<<(
			std::ostream &os,
			const binary_configuration_vector<use_bitfield> &bf)
		{
			os << "binary_configuration_vector:";
			for (size_type i = 0; i < bf.data.size(); i++) {
				os << " " << (bf.data[i] ? "1" : "0");
			}
			return os;
		}


		/// count number of ones in config vector
		std::size_t count_ones() const
		{
			std::size_t ones = 0;

			for (size_type i = 0; i < data.size(); i++) {
				if (data[i]) { ones++; }
			}

			return ones;
		}

		/// test for equality of binary config vectors
		bool operator==(
			const binary_configuration_vector<use_bitfield> &rhs) const
		{
			if (data.size() == rhs.data.size()) {
				for (size_type i = 0; i < data.size(); i++) {
					if (data[i] != rhs.data[i]) { return false; }
				}
				return true;
			}
			return false;
		}
		
		/// test for inequality of binary config vectors
		bool operator!=(
			const binary_configuration_vector<use_bitfield> &rhs) const
		{
			return !(*this == rhs);
		}
};


/// serialize a binary config vector
template<typename S, bool use_bitfield>
void serialize(
	S &s, binary_configuration_vector<use_bitfield> &cfg_vector)
{
	serialize_marker(s, "bcv", 4);

	bool ub = use_bitfield;
	s.value1b(ub);
	assert(use_bitfield == ub);

	serialize(s, cfg_vector.data);
}


#endif
