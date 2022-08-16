
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cassert>

#include "resolvents_manager.hpp"

bool resolvents_manager::load(std::string filename) {
	std::ifstream file(filename, std::ifstream::in);

	if (false == file.is_open() ) {
		std::string errmsg = std::string("can't open ") + filename;
		perror(errmsg.c_str());
		return false;
	}
	
	while (file.good()) {
		std::string line;
		std::getline(file, line);
		if ((false == line.starts_with("c ")) && (!line.empty())) {
			assert(line.ends_with(" 0"));
			resolvent.push_back(line);
		}
	}
	
	file.close();
	
	return true;
}

std::size_t resolvents_manager::get_num_resolvents() const {
	return resolvent.size();
}

std::string resolvents_manager::get_resolvent(std::size_t i) const {
	return resolvent[i];
}

