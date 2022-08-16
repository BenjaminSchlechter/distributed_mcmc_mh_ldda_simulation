#ifndef BITFIELD_HPP
#define BITFIELD_HPP

#include <cstdint>
#include <cassert>
#include <type_traits>
#include <ostream>

/** \brief bitfield to represent a bitvector compact
 * 
 * The configuration vector can be stored in a compact bitvector
 */
template<typename length_t>
class bitfield {
	private:
		/// number of bits
		length_t length;

		// helper values to allow returning references to memory

		/// used internally for returning a reference (see update function)
		length_t last_mod_index;

		/// used internally for returning a reference (see update function)
		bool last_mod_value;

		/// pointer to the actual data
		uint8_t *array;


		// helper functions
		
		/// compute number of bytes required to store l bits
		static inline length_t to_bytelength(const length_t l) {
			return (l+7) >> 3; 
		}

		/// get bit at position index as bool
		inline bool get(const length_t index) const
		{
			#if DEBUG_ASSERTIONS
			assert(index < length);
			#endif

			return 1 & (array[index >> 3] >> (index & 0b111));
		}

		/// set bit at position index to value (bool)
		inline void set(const length_t index, const bool value)
		{
			update();

			#if DEBUG_ASSERTIONS
			assert(index < length);
			#endif

			const uint8_t mask = 1 << (index & 0b111);
			if (value) {
				array[index >> 3] |= mask;
			} else {
				array[index >> 3] &= ~mask;
			}
		}

		// apply changes by returned reference to array
		inline void update() {
			if (last_mod_index < length) {
				// std::cout << "a["<<(int)last_mod_index<<"] = "
					// << (int)last_mod_value << std::endl;
				set(last_mod_index, last_mod_value);
				last_mod_index = length;
			}
		}

	public:
		// constructors

		/// default constructor, empty bitvector of size 0
		bitfield() :
			length(0),
			last_mod_index(length),
			last_mod_value(false),
			array(nullptr) {}
		
		/// constructor for bitvectors of size n
		bitfield(length_t n) :
			length(n), last_mod_index(length), last_mod_value(false),
			array(length > 0 ? new uint8_t[to_bytelength(length)] : nullptr) {}

		// bitfield(const bitfield &other) = delete;

		/// copy constructor
		bitfield(const bitfield &other) :
			length(other.length),
			last_mod_index(other.last_mod_index),
			last_mod_value(other.last_mod_value),
			array(length > 0 ? new uint8_t[to_bytelength(length)] : nullptr)
		{
			for (length_t i = 0; i < to_bytelength(length); i++) {
				array[i] = other.array[i];
			}
		}
		
		/// deconstructor (is freeing memory)
		~bitfield() {
			if (nullptr != array) {
				#if DEBUG_ASSERTIONS
					assert(0 < length);
				#endif
				delete[] array;
				array = nullptr;
			} else {
				#if DEBUG_ASSERTIONS
					assert(0 == length);
				#endif
			}
		}

		// copy & swap idiom

		/// swap implementation for copy & swap idiom
        friend void swap(bitfield &a, bitfield &b)
			noexcept(std::is_nothrow_swappable_v<length_t>
				&&   std::is_nothrow_swappable_v<bool>
				&&   std::is_nothrow_swappable_v<uint8_t*>)
		{
			#if DEBUG_ASSERTIONS
			assert(a.length == b.length);
			#endif
			// or remove const from length
            // std::swap(a.length, b.length);
            std::swap(a.last_mod_index, b.last_mod_index);
            std::swap(a.last_mod_value, b.last_mod_value);
            std::swap(a.array, b.array);
        }

		/// move semantics by using swap
        bitfield(bitfield &&other) : bitfield() {
            swap(*this, other);
        }

		/// assignment operator by using swap
        bitfield& operator=(bitfield other) {
            swap(*this, other);
            return *this;
        }

		/** \brief resize bitvector length
		 * 
		 * resize bitvector length only reallocates memory if new space
		 * requirement is larger than the current allocated memory
		 */
		void resize(length_t new_length)
		{
			update();

			if (to_bytelength(new_length) > to_bytelength(length)) {
				assert(0 < to_bytelength(new_length));

				uint8_t *old_array = array;

				array = new uint8_t[to_bytelength(new_length)];
				
				for (length_t i = 0; i < to_bytelength(length); i++) {
					array[i] = old_array[i];
				}

				if (old_array) {
					delete[] old_array;
				}
			}

			length = new_length;
			last_mod_index = length;
		}

		/// get amount of bytes required to store *current* length bits
		length_t get_bytelength() const {
			return to_bytelength(length);
		}

		/// get raw pointer to the array of bits
		uint8_t *data() {
			update();
			return array;
		}

		// access operators
		
		/** \brief access position index by returning a boolean reference
		 * 
		 * access position index by returning a boolean reference to
		 * a class member. changes are applied later when using
		 * other functions. WARNING: may cause trouble when used for
		 * multiple indices at once as references cant be invalidated!
		 */
		inline bool& operator[](const length_t index)
		{
			update();

			#if DEBUG_ASSERTIONS
			assert(index < length);
			#endif

			last_mod_index = index;
			last_mod_value = get(index);
			return last_mod_value;
		}
		
		/// return value at position index as bool
		inline bool operator[](const length_t index) const {
			// dont apply changes, so this function can be const
			if (index == last_mod_index) {
				return last_mod_value;
			}

			return get(index);
		}
		
		/// get the number of bits which can be stored
		length_t size() const {
			return length;
		}
		
		// output
		
		/// print the bitvector to std::ostream
		std::ostream &print(std::ostream &os, const char *separator = " ")
		{
			update();
			
			const length_t bl = to_bytelength(length);

			auto sep = "";
			for (length_t i = 0; i < bl-1; i++) {
				os << sep;
				
				uint8_t d = array[i];
				for (uint8_t j = 0; j < 8; j++) {
					os << ((d & 1) ? "1" : "0");
					d >>= 1;
				}
				
				sep = separator;
			}
			
			if (0 < bl) {
				os << sep;
				for (length_t i = (bl-1)*8; i < length; i++) {
					os << (get(i) ? "1" : "0");
				}
			}
			
			return os;
        }
};

/// std::ostream << operator is using print to display the bitvector
template<typename length_t>
std::ostream &operator<<(std::ostream& os, bitfield<length_t> bf) {
	return bf.print(os);
}

/// bitstream serialization (using bitsery)
template<typename S, typename length_t>
void serialize(S &s, bitfield<length_t> &bf) {
	bf.update();

	length_t size = bf.length;
	serialize_basic_type<S, length_t>(s, size);

	bf.resize(size);

	uint8_t *dataptr = bf.data();
	for (std::size_t i = 0; i < bf.get_bytelength(); i++) {
		s.value1b(dataptr[i]);
	}

	assert(bf.size() == size);
}

#endif
