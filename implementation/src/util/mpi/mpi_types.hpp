#ifndef MPI_TYPES_HPP
#define MPI_TYPES_HPP

#include <mpi.h>

/// the MPI datatype for type T can be queried by get_mpi<T>::type
template<typename t>
struct get_mpi {
};

// exclude template specialications
/// \cond HIDDEN_TEMPLATE_SPECIALIZATION

// with definitions like the following all was working fine in the past... :/
// template<>
// struct get_mpi<std::byte> {
	// static constexpr auto type = MPI_BYTE;
// };

// but now hacks are required to make it work again, see:
// https://github.com/open-mpi/ompi/issues/10017
// therefore it was switched to runtime evaluation...


template<>
struct get_mpi<std::byte> {
	static auto type() { return MPI_BYTE; }
};

template<>
struct get_mpi<char> {
	static auto type() { return MPI_SIGNED_CHAR; }
};

template<>
struct get_mpi<unsigned char> {
	static auto type() { return MPI_UNSIGNED_CHAR; }
};

template<>
struct get_mpi<short> {
	static auto type() { return MPI_SHORT; }
};

template<>
struct get_mpi<unsigned short> {
	static auto type() { return MPI_UNSIGNED_SHORT; }
};

template<>
struct get_mpi<int> {
	static auto type() { return MPI_INT; }
};

template<>
struct get_mpi<unsigned int> {
	static auto type() { return MPI_UNSIGNED; }
};

template<>
struct get_mpi<long int> {
	static auto type() { return MPI_LONG; }
};

template<>
struct get_mpi<unsigned long int> {
	static auto type() { return MPI_UNSIGNED_LONG; }
};

template<>
struct get_mpi<long long int> {
	static auto type() { return MPI_LONG_LONG_INT; }
};

template<>
struct get_mpi<float> {
	static auto type() { return MPI_FLOAT; }
};

template<>
struct get_mpi<double> {
	static auto type() { return MPI_DOUBLE; }
};

template<>
struct get_mpi<long double> {
	static auto type() { return MPI_LONG_DOUBLE; }
};

// old code:


// template<>
// struct get_mpi<std::byte> {
	// static constexpr auto type = MPI_BYTE;
// };

// template<>
// struct get_mpi<char> {
	// static constexpr auto type = MPI_SIGNED_CHAR;
// };

// template<>
// struct get_mpi<unsigned char> {
	// static constexpr auto type = MPI_UNSIGNED_CHAR;
// };

// template<>
// struct get_mpi<short> {
	// static constexpr auto type = MPI_SHORT;
// };

// template<>
// struct get_mpi<unsigned short> {
	// static constexpr auto type = MPI_UNSIGNED_SHORT;
// };

// template<>
// struct get_mpi<int> {
	// static constexpr auto type = MPI_INT;
// };

// template<>
// struct get_mpi<unsigned int> {
	// static constexpr auto type = MPI_UNSIGNED;
// };

// template<>
// struct get_mpi<long int> {
	// static constexpr auto type = MPI_LONG;
// };

// template<>
// struct get_mpi<unsigned long int> {
	// static constexpr auto type = MPI_UNSIGNED_LONG;
// };

// template<>
// struct get_mpi<long long int> {
	// static constexpr auto type = MPI_LONG_LONG_INT;
// };

// template<>
// struct get_mpi<float> {
	// static constexpr auto type = MPI_FLOAT;
// };

// template<>
// struct get_mpi<double> {
	// static constexpr auto type = MPI_DOUBLE;
// };

// template<>
// struct get_mpi<long double> {
	// static constexpr auto type = MPI_LONG_DOUBLE;
// };

/// \endcond

#endif
