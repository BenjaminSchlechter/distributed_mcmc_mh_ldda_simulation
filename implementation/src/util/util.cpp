
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "util.hpp"

std::ptrdiff_t slurp_file(std::string &content_str, const std::string filename) {
	auto fd = open(filename.c_str(), O_RDONLY);
	if (0 > fd) {
		std::string errmsg = std::string("can't get file descriptor for ") + filename;
		perror(errmsg.c_str());
		return -1;
	}

    struct stat stat_buf;
    auto r = fstat(fd, &stat_buf);
    if (0 != r) {
		std::string errmsg = std::string("can't get file size with fstat on ") + filename;
		perror(errmsg.c_str());
		return -1;
	}

	content_str.resize(stat_buf.st_size);
	if (stat_buf.st_size != read(fd, content_str.data(), stat_buf.st_size)) {
		std::string errmsg = std::string("error reading ") + std::to_string(stat_buf.st_size) + std::string(" bytes from ") + filename;
		perror(errmsg.c_str());
		return -1;
	}

	close(fd);
	return stat_buf.st_size;
}


void print_vector(std::string msg, const std::vector<uint8_t> &v)
{
	std::cout << msg;
	for (uint8_t i : v) {
		std::cout << " " << (int) i;
	}
	std::cout << std::endl;
}


uint8_t crc8(const uint8_t *data, const uint32_t length, uint8_t crc)
{
	constexpr const uint16_t polynom = 0b100110001;
	constexpr const uint8_t p_mask = polynom & 0xFF;

    for (uint32_t i = 0; i < length; i++) {
		const uint8_t d = data[i];
		for (int8_t j = 7; j >= 0; j--) {
			uint8_t e = ((crc >> 7)^(d >> j)) & 1;
			crc = (crc << 1) ^ p_mask*e;
		}
    }

	return crc;
}


/// serialize boolean vector to a string
auto vector_to_string(const std::vector<bool> &v)
{
	std::stringstream isstr;
	isstr << v.size();
	for (auto b : v) {
		isstr << " " << b;
	}
	assert(!isstr.fail());
	return isstr.str();
}


/// deserialize boolean vector from a string
auto vector_from_string(auto content)
{
	std::stringstream osstr(content);
	std::size_t size;
	osstr >> size;

	std::vector<bool> v(size);

	for (std::size_t i = 0; i < size; i++) {
		bool value;
		osstr >> value;
		v[i] = value;
	}

	assert(!osstr.fail());
	assert(osstr.eof());
	return v;
}


bool fd_is_valid(int fd)
{
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

