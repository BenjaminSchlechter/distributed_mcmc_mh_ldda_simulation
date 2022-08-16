#ifndef MCMC_HPP
#define MCMC_HPP


#include <cstddef>
#include <random>
#include <sstream>

#include "../config.hpp"
#include "../util/basic_serialization.hpp"


/// represents a single state of a markov chain
template<typename cfg_vector_t>
class marcov_chain_state
{
	public:
		/// type of the config vector used
		using cfg_vector_type = cfg_vector_t;

		/// current iteration
		std::size_t iteration;

		/// configuration vector
		cfg_vector_t cfg_vector;

		/// pseudo random number engine (state)
		prn_engine_t prn_engine;


		/// default constructor
		marcov_chain_state(std::size_t cfg_vector_length = 0) :
			iteration(std::numeric_limits<std::size_t>::max()),
			cfg_vector(cfg_vector_length),
			prn_engine() {}

		/// normal constructor to set cfg vector length and random number engine
		marcov_chain_state(
			std::size_t cfg_vector_length,
			auto prn_engine
		) :
			iteration(0),
			cfg_vector(cfg_vector_length),
			prn_engine(prn_engine)
		{}

		/// constructor to set all class members
		marcov_chain_state(
			std::size_t iteration,
			cfg_vector_t &cfg_vector,
			prn_engine_t &prn_engine
		) :
			iteration(iteration),
			cfg_vector(cfg_vector),
			prn_engine(prn_engine) {}
		
		/// copy constructor
		marcov_chain_state(const marcov_chain_state &other) :
			iteration(other.iteration),
			cfg_vector(other.cfg_vector),
			prn_engine(other.prn_engine) {}

		/// movement constructor using swap
		marcov_chain_state(marcov_chain_state &&other) {
			swap(*this, other);
		}
		
		/// assignment operator
		marcov_chain_state &operator=(
			const marcov_chain_state<cfg_vector_t> &other)
		{
			iteration = other.iteration;
			cfg_vector = other.cfg_vector;
			prn_engine = other.prn_engine;
			return *this;
		}

		/// swap function
		friend void swap(
			marcov_chain_state<cfg_vector_t> &l,
			marcov_chain_state<cfg_vector_t> &r)
		{
			std::swap(l.iteration, r.iteration);
			std::swap(l.cfg_vector, r.cfg_vector);
			std::swap(l.prn_engine, r.prn_engine);
		}

		/// equality test
		bool operator==(
			const marcov_chain_state<cfg_vector_t> &rhs) const
		{
			return (iteration == rhs.iteration)
				&& (cfg_vector == rhs.cfg_vector)
				&& (prn_engine == rhs.prn_engine);
		}

		/// inequality test
		bool operator!=(
			const marcov_chain_state<cfg_vector_t> &rhs) const
		{
			return !(*this == rhs);
		}

		/// ostream operator to print state
		friend std::ostream &operator<<(
			std::ostream &os,
			const marcov_chain_state<cfg_vector_t> &mcs)
		{
			os  << "state of iteration " << mcs.iteration << ":\n"
				<< mcs.cfg_vector << "\n" << mcs.prn_engine << std::endl;
			return os;
		}

		/// function to convert pseudo random number into string
		std::string get_prn_engine_as_string() const {
			std::stringstream isstr;
			isstr << prn_engine;
			assert(!isstr.fail());
			return isstr.str();
		}

		/// function to restore pseudo random number from string
		void set_prn_engine_from_string(auto prn_state) {
			std::stringstream osstr(prn_state);
			osstr >> prn_engine;
			assert(!osstr.fail());
			assert(osstr.eof());
		}
};


/// function to serialize a marcov_chain_state
template<typename S, typename cfg_vector_t>
void serialize(S &s, marcov_chain_state<cfg_vector_t> &mc)
{
	std::string prn_state = mc.get_prn_engine_as_string();

	// serialize_marker(s, "ms1", 4);
	serialize_basic_type<S, decltype(mc.iteration)>(s, mc.iteration);

	// serialize_marker(s, "ms2", 4);
	serialize(s, mc.cfg_vector);

	// serialize_marker(s, "ms3", 4);
	generic_serialize(s, prn_state);
	mc.set_prn_engine_from_string(prn_state);

	// serialize_marker(s, "ms4", 4);
}

/*
template<typename S, typename cfg_vector_t>
void serialize(S &s, marcov_chain_state<cfg_vector_t> &mc)
{
	std::cout << "start" << std::endl;
	std::string prn_state;

	std::stringstream isstr;
	isstr << mc.prn_engine;
	prn_state = isstr.str();

	// generic_serialize(s, mc.iteration, mc.cfg_vector, prn_state);
	// generic_serialize(s, mc.iteration);
	s.value8b(mc.iteration);

	// if (mc.cfg_vector.data.length()
	// cfg_vector(cfg_vector_length)

	// std::cout << "mc.cfg_vector size a: " << mc.cfg_vector.data.size() << std::endl;

	serialize(s, mc.cfg_vector);

	// std::cout << "mc.cfg_vector size b: " << mc.cfg_vector.data.size() << std::endl;

	std::string marker1 = "m42";
	s.text1b(marker1, 4);
	assert(std::string("m42") == marker1);

	std::size_t strlength = prn_state.size();
	generic_serialize(s, strlength);
	prn_state.reserve(strlength);
	s.text1b(prn_state, strlength);
	assert((prn_state.size() == strlength) || print_trace());

	std::string marker2 = "fm!";
	s.text1b(marker2, 4);
	assert(std::string("fm!") == marker2);

	std::stringstream osstr(prn_state);
	osstr >> mc.prn_engine;
	
	std::cout << "ok" << std::endl;
}
*/

/*
// output statistic
class mcmc_statistic
{
	private:
		std::size_t mc_id;

	public:
		mcmc_statistic(std::size_t mc_id) : mc_id(mc_id) {}
};

// able to fast reload application state
class mcmc_history
{
	private:
		std::size_t mc_id;
		std::fstream hfile;
		uint8_t no_counter = 0;

Problem: do changes need consistent prn generator?

	full save:
		store "F"
		size of marcov_chain_state
		marcov_chain_state
			iteration
			cfg_vector
			prn_engine

		go to the start of the file and save last position of "F"

	incremental save:
		accepted?
			if yes:
				store "n" save no counter
				store "y"
				save changes in cfg vector (size + changes)
			if no:
				increment no counter
				prevent overflow (i.e. store "n" and save no counter)

	restore:
		load last full save
		restore current iteration cfg_vector and prn_engine state by fast forwarding
		continue as from beginning


		void save_no() {
			hfile.put("n");
			hfile.put(no_counter);
			no_counter = 0;
		}

		void save_full(marcov_chain_state &mc_state)
		{
			const std::size_t offset = hfile.tellp();

			// write "F", size, mc_state
			hfile.put("F");

			type_abstraction_adapter_t bitsery_taa;
			buffer_t data = bitsery_taa.serialize(mc_state);

			make_generic_serializeable<std::size_t> gssize = data.size();
			buffer_t size = bitsery_taa.serialize(gssize);
			assert(size.size() == sizeof(std::size_t)); // otherwise load won't work!

			assert(!hfile.write(size.data(), size.size()).fail());
			assert(!hfile.write(data.data(), data.size()).fail());
			hfile.flush();
			assert(!hfile.fail());

			// write offset at start of file
			const std::size_t end = hfile.tellp();
			hfile.seekp(0, hfile.beg);
			make_generic_serializeable<std::size_t> gsoffset = offset;
			buffer_t offs = bitsery_taa.serialize(gsoffset);
			assert(offs.size() == sizeof(std::size_t));
			assert(!hfile.write(offs.data(), offs.size()).fail());
			hfile.seekp(end, hfile.beg);

			hfile.flush();
			assert(!hfile.fail());
		}

		void load_full(marcov_chain_state &mc_state)
		{
			char c = 0;
			assert(hfile.get(c) && c == 'F');
			type_abstraction_adapter_t bitsery_taa;

			// read size
			make_generic_serializeable<std::size_t> gssize = 0;
			std::vector inbuf(sizeof(std::size_t));
			assert(!hfile.read(inbuf.data(), inbuf.size()).fail());
			assert(bitsery_taa.deserialize(inbuf.data(), gssize));

			// read mc_state
			inbuf.resize(gssize.data);
			assert(!hfile.read(inbuf.data(), inbuf.size()).fail());
			assert(bitsery_taa.deserialize(inbuf.data(), mc_state));
		}

	public:
		void add_changes(bool accepted, auto &changes) {
			if (accepted) {
				if (0 < no_counter) { save_no(); }
				hfile.put("y");
				// type_abstraction_adapter_t bitsery_taa;
				// buffer_t data = bitsery_taa.serialize(changes);
				// assert(!hfile.write(data.data(), data.size()).fail());
				changes.save_to(hfile);
				hfile.flush();
			} else {
				no_counter++;
				if (std::numeric_limits<decltype(no_counter)>::max() == no_counter) {
					save_no();
					hfile.flush();
				}
			}

			assert(!hfile.fail());			
		}

		mcmc_history(std::size_t mc_id, marcov_chain_state &mc_state) :
			mc_id(mc_id)
			hfile(data_directory + "/" + std::to_string(mc_id) + ".mch",
				std::ios::in | std::ios::out | ios::binary | std::ios::app),
			no_counter(0)
		{
			assert(hfile.is_open());

			hfile.seekp(0, hfile.end);
			const auto fsize = hfile.tellp();

			if (0 == fsize) {
				// write offset and first full save
				std::size_t s = 0;
				assert(!hfile.write(&s, sizeof(std::size_t)).fail());
				save_full(mc_state);
			} else {
				// read offset of last full save
				hfile.seekg(0, hfile.beg);
				type_abstraction_adapter_t bitsery_taa;
				make_generic_serializeable<std::size_t> gsoffset = 0;
				std::vector inbuf(sizeof(std::size_t));
				assert(hfile.read(inbuf.data(), inbuf.size()).good());
				assert(bitsery_taa.deserialize(inbuf.data(), gsoffset));

				// read first save
				hfile.seekg(sizeof(std::size_t), hfile.beg);
				marcov_chain_state_t mc_state_(mc_state.cfg_vector.data.size(), prn_engine_t());
				load_full(mc_state_);
				assert(mc_state_ == mc_state && "initial state must match!");

				// read last full save
				hfile.seekg(gsoffset.data, seed_file.beg);
				load_full(mc_state);

				// fast forward
				while () {
					
				}
			}

			assert(!hfile.fail());
		}
};
*/

#endif
