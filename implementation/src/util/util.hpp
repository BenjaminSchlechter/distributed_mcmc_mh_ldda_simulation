#ifndef UTIL_HPP
#define UTIL_HPP


#include <string>
#include <cstddef>
#include <sstream>
#include <tuple>
#include <iostream>
#include <vector>
#include <cassert>

#include <unistd.h> 
#include <fcntl.h> 


/// template to ignore parameters to prevent unused compiler warnings
template<typename t, typename... args>
void ignore(const t&, args...) { }


/// read a file into a string and get its size (or -1 on error)
std::ptrdiff_t slurp_file(
	std::string &content_str, const std::string filename);


/// template to convert string into datatype using a stringstream
template<typename t>
t from_string(std::string s) {
	t value;
	std::istringstream iss(s);
	iss >> value;

	if (!iss) {
		std::string errmsg = "can't parse '" + s + "'";
		throw std::runtime_error(errmsg);
	}

	return value;
}


/// simple debugging utility to print a std::vector<uint8_t>
void print_vector(std::string msg, const std::vector<uint8_t> &v);


/** \brief get the tail of a tuple
 * 
 * get the tail of a tuple as tuple using apply and lambda function,
 * the code for this function is from:
 * https://stackoverflow.com/questions/8569567/get-part-of-stdtuple
 */
template <typename head_t, typename... tail_t>
auto tail(const std::tuple<head_t, tail_t...> &tuple) {
    return apply([](auto &head, auto &... tail) { ignore(head); return std::make_tuple(tail...); }, tuple);
}


bool perror_(auto msg) {
	perror(msg);
	return false;
}


/** \brief function to compute CRC-8 of data
 * 
 * Math based upon lecture "Angewandte Diskrete Mathematik" at Ulm
 * University and the following link:
 * https://de.wikipedia.org/wiki/Zyklische_Redundanzpr%C3%BCfung
 * 
 * some commonly used CRC-8 polynoms:
 * p=0b100011101, CRC-8 (SAE-J1850): x^8 + x^4 + x^3 + x^2 + 1 discovers burst errors up to length 8.
 * p=0b100000111 CRC-8 (ITU-T): x^8 + x^2 + x + 1 discovers burst errors up to length 8 and every uneven number of errors.
 * p=0b100110001 CRC-8 (Dallas/Maxim 1-Wire Bus): x^8 + x^5 + x^4 + 1 discovers burst errors up to length 8 and every uneven number of errors.
 */
uint8_t crc8(const uint8_t *data, const uint32_t length, uint8_t crc = 0);


/// serialize boolean vector to a string
auto vector_to_string(const std::vector<bool> &v);


/// deserialize boolean vector from a string
auto vector_from_string(auto content);

/** \brief check wether a filedescriptor is still valid
 * 
 * code for this function is taken from:
 * https://stackoverflow.com/questions/12340695/how-to-check-if-a-given-file-descriptor-stored-in-a-variable-is-still-valid
 */
bool fd_is_valid(int fd);


/*
// https://stackoverflow.com/questions/41301536/get-function-return-type-in-template
template<typename R, typename... A>
R get_return_type(R(*)(A...));

template<typename C, typename R, typename... A>
R get_return_type(R(C::*)(A...));
*/


#endif
