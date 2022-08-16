#ifndef RESOLVENTS_MANAGER_HPP
#define RESOLVENTS_MANAGER_HPP

#include <string>
#include <vector>

/// class to manage resolvents
class resolvents_manager {
	private:
		/// list of resolvents
		std::vector<std::string> resolvent;

	public:
		/// constructor
		resolvents_manager() = default;

		/// load file with resolvents
		bool load(std::string filename);

		/// get number of resolvents
		std::size_t get_num_resolvents() const;

		/// access resolvent at index i
		std::string get_resolvent(std::size_t i) const;
};

#endif
