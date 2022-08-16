#ifndef SHARED_TMPFILE
#define SHARED_TMPFILE

#include <cstddef>
#include <string>
#include <stdio.h>

/** \brief temporary file implementation
 * 
 * provides temporary files identified by an uuid and purpose string
 * which can therefore be reused.
 */
class shared_tmpfile
{
	private:
		/// tmpfile id
		std::size_t id;

	public:
		/// name of tmp file
		std::string fname;

		/// FILE * pointer
		FILE *fptr;

		/// dummy default constructor, does not provide any file access
		shared_tmpfile();

		/** \brief create a temporary file for purpose (string)
		 * 
		 * the temporary file opened will contain an uuid and the
		 * purpose string and therefore can be reused by the application
		 */
		shared_tmpfile(std::string purpose);
		
		/// disable copy constructor
		shared_tmpfile(const shared_tmpfile &other) = delete;

		// copy & swap idiom

		/// swap implementation for copy & swap idiom
		friend void swap(shared_tmpfile &a, shared_tmpfile &b);

		/// move constructor by using swap
		shared_tmpfile(shared_tmpfile &&other) noexcept;

		/// assignment operator by using swap
		shared_tmpfile &operator=(shared_tmpfile other);

		/// remove the tmp file manually
		void remove();

		/// deconstructor, removes the tmp file if required
		~shared_tmpfile();
};

#endif
