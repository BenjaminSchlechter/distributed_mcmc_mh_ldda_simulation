
#include "shared_tmpfile.hpp"
#include "uuid.hpp"

#include <filesystem>
#include <cassert>
#include <iostream>

shared_tmpfile::shared_tmpfile() : id(0), fname(), fptr(nullptr) {}

shared_tmpfile::shared_tmpfile(std::string purpose) :
	id(uuid<shared_tmpfile>::get()+1),
	fname(),
	fptr(nullptr)
{
	std::string tmp_path = std::filesystem::temp_directory_path();
	fname = tmp_path + "/tmpfile-" + purpose + "_" + std::to_string(id);

	// std::cout << "created shared_tmpfile: " << fname << std::endl;
	fptr = fopen(fname.c_str(), "wbx+");
	if (nullptr == fptr) {
		std::string errmsg = "fopen error (shared_tmpfile "
			+ std::to_string(id) + "):";
		perror(errmsg.c_str());
		
		if (false) {
			fptr = fopen(fname.c_str(), "wb+");
			assert(fptr);
			std::cerr
				<< "note: recovered by reopening with wb+" << std::endl;
		} else {
			assert(false);
		}
	}
}
		
void swap(shared_tmpfile &a, shared_tmpfile &b)
{
	std::swap(a.id, b.id);
	std::swap(a.fname, b.fname);
	std::swap(a.fptr, b.fptr);
}

shared_tmpfile::shared_tmpfile(shared_tmpfile &&other) noexcept :
	shared_tmpfile()
{
	swap(*this, other);
}

shared_tmpfile &shared_tmpfile::operator=(shared_tmpfile other)
{
	swap(*this, other);
	return *this;
}

void shared_tmpfile::remove()
{
	if (0 < id) {
		// std::cout << "removing stmpfile " << id
			// << ": " << fname << std::endl;
		uuid<shared_tmpfile>::free(id-1);
		id = 0;
	}

	if (nullptr != fptr) {
		assert(0 == fclose(fptr));
		fptr = nullptr;
		assert(std::filesystem::remove(fname));
	}

	fname = "";
}

shared_tmpfile::~shared_tmpfile() {
	remove();
}

