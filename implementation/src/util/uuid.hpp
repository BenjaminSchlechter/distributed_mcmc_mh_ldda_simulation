#ifndef UUID_HPP
#define UUID_HPP

#include <cstddef>
#include <vector>
#include <cassert>

/** \brief uuid using crtp
 * 
 * A Universally Unique Identifier Implementation using the curiously
 * recurring template pattern. UUID Type can be specified as template
 * parameter. Also some id's can be reserved.
 */
template<class crtp, typename uuid_type = std::size_t, uuid_type reserved_ids = 1>
class uuid
{
	private:
		/// next new uuid
		static uuid_type new_uuid;

		/// list of unused/free ids smaller new_uuid
		static std::vector<uuid_type>
			unused;

	public:
		uuid() = delete;
		
		/// get an unused uuid
		static uuid_type get()
		{
			uuid_type id;

			if (unused.empty()) {
				id = new_uuid;
				new_uuid++;
				assert(0 != new_uuid + reserved_ids);
			} else {
				id = unused.back();
				unused.pop_back();
			}

			// std::cout << "get " << id << std::endl;

			return id;
		}

		/// return an uuid for reuse
		static void free(const uuid_type id) {
			// std::cout << "free " << id << std::endl;
			unused.push_back(id);
		}
};

// static parameter initialization

template<class crtp, typename uuid_type, uuid_type reserved_ids>
uuid_type uuid<crtp, uuid_type, reserved_ids>::new_uuid = 0;

template<class crtp, typename uuid_type, uuid_type reserved_ids>
std::vector<uuid_type> uuid<crtp, uuid_type, reserved_ids>::unused;

#endif
