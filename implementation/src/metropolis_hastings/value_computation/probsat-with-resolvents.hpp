#ifndef PROBSAT_WITH_RESOLVENTS_HPP
#define PROBSAT_WITH_RESOLVENTS_HPP

#include <filesystem>
#include <string>
#include <cstddef>

#include "../configuration/configuration_vector.hpp"

#include "utility/resolvents_manager.hpp"
#include "utility/probsat-execution.hpp"

#include "../../util/util.hpp"
#include "../../util/shared_tmpfile.hpp"


/// wrapper to create and solve probsat instances with resolvents
template<typename cfg_vector_t, typename prec_t, bool use_bitfield>
requires (
	std::is_same<binary_configuration_vector<use_bitfield>,
	cfg_vector_t>::value
)
class probsat_resolvents
{
	public:
		using prec_type = prec_t;
		using cfg_vector_type = cfg_vector_t;
		constexpr static bool is_value_computation_implementation = true;

	private:
		/// number of variables
		std::size_t cnf_num_vars;

		/// number of clauses
		std::size_t cnf_num_clauses;

		/// cnf formula
		std::string cnf_file;

		/// header of the cnf formula
		std::string cnf_file_header;

		/// seed distribution type
		using seed_distr_t = std::uniform_int_distribution<probsat_seed_t>;

	public:
		/// string to adapt workgroup description for parallel execution
		std::string workgroup_description;


		/// represents a prepared probsat computation for a given configuration
		typedef struct {
			/// name of the temporary file
			shared_tmpfile stmpfile;

			/// seed distribution
			seed_distr_t seed_distr;
		} prepared_computation;


		/// resolvents manager
		resolvents_manager resolvents;


		/// constructor loading cnf formula and resolvents
		probsat_resolvents(
			std::string cnf_filename,
			std::string resolvents_fn
		) :
			cnf_num_vars(0),
			cnf_num_clauses(0),
			cnf_file(),
			cnf_file_header(),
			resolvents()
		{
			std::ptrdiff_t fsize = slurp_file(cnf_file, cnf_filename);
			if ((0 > fsize) || ((std::size_t) fsize != cnf_file.size()))
			{
				throw std::runtime_error(
					"cant read cnf file " + cnf_filename);
			}

			std::string cfg_marker = "p cnf ";
			std::size_t pos = cnf_file.find(cfg_marker);
			std::size_t eol = cnf_file.find("\n", pos+1, 1);
			if ((std::string::npos == pos) || (std::string::npos == eol))
			{
				throw std::runtime_error(
					"can't find config line starting with '"
					+ cfg_marker + "' in cnf file " + cnf_filename);
			}
			cnf_file[pos] = 'c'; // comment out the cnf line
			std::string buf = cnf_file.substr(
				pos+cfg_marker.length(), eol-pos-cfg_marker.length());
			std::istringstream iss(buf);
			iss >> cnf_num_vars;
			iss >> cnf_num_clauses;

			cnf_file_header = cnf_file.substr(0, eol+1);
			cnf_file = cnf_file.substr(eol+1);

			if (!resolvents.load(resolvents_fn)) {
				throw std::runtime_error(
					"cant read resolvents file " + resolvents_fn);
			}
		}
		
		// ~probsat_resolvents() {
			// std::cout << "~probsat_resolvents" << std::endl;
		// }

		/// disable copy constructor
		probsat_resolvents(const probsat_resolvents &other) = delete;


		/// prepare probsat computations for a given config vector
		void prepare_computation_with_cfg_vector(
			const cfg_vector_t &v, prepared_computation &pcom)
		{
			pcom.seed_distr.reset();

			pcom.stmpfile = std::move(shared_tmpfile(
				"sat-with-resolvents-" + workgroup_description));
			create_cnf_file_from_cfg_vector(v, pcom.stmpfile.fptr);
		}


		/// create a remote function call for one execution
		template<typename prng_t, typename rpmanager_t>
		auto operator()(
			prng_t &prng,
			rpmanager_t &rpmanager,
			prepared_computation &pcom) const
		{
			std::string satfile = pcom.stmpfile.fname;
			probsat_seed_t seed = pcom.seed_distr(prng);

			// std::cout << "c PWR preparing "
				// << satfile << " with seed " << seed << std::endl;

			return rpmanager.prepare_call(
				"execute_probsat", satfile, seed);
		}

		void finish_computation(prepared_computation &pcom) {
			pcom.stmpfile.remove();
		}

	private:

		/// add resolvents to the cnf formula based on a given config vector
		FILE *create_cnf_file_from_cfg_vector(
			const cfg_vector_t &v, FILE *fptr) const
		{
			// TODO
			assert(v.data.size() <= resolvents.get_num_resolvents());
			// assert(v.data.size() == resolvents.get_num_resolvents());

			// FILE *fptr = tmpfile();
			assert(nullptr != fptr);

			std::size_t num_clauses = cnf_num_clauses;
			for (typename cfg_vector_t::size_type i = 0;
				i < v.data.size(); i++)
			{
				if (0 != v.data[i]) {
					num_clauses++;
				}
			}

			assert(cnf_file_header.size() ==
				fwrite(
					cnf_file_header.data(),
					1,
					cnf_file_header.size(),
					fptr)
				|| perror_("fwrite failed"));
			std::string cfg_line = std::string("p cnf ")
				+ std::to_string(cnf_num_vars) + std::string(" ")
				+ std::to_string(num_clauses) + std::string("\n");

			cfg_line = "c after adding resolvents:\n" + cfg_line;

			assert(cfg_line.size() ==
				fwrite(cfg_line.data(), 1, cfg_line.size(), fptr)
				|| perror_("fwrite failed"));

			assert(cnf_file.size() ==
				fwrite(cnf_file.data(), 1, cnf_file.size(), fptr)
				|| perror_("fwrite failed"));

			typename cfg_vector_t::size_type num_clauses_to_write
				= num_clauses - cnf_num_clauses;
			for (typename cfg_vector_t::size_type i = 0;
				i < v.data.size(); i++)
			{
				if (0 != v.data[i]) {
					std::string rv = resolvents.get_resolvent(i);
					if (1 < num_clauses_to_write) {
						rv += "\n";
					}

					assert(rv.size() ==
						fwrite(rv.data(), 1, rv.size(), fptr));
					num_clauses_to_write--;
				}
			}

			assert(0 == num_clauses_to_write);
			
			assert(0 == fflush(fptr) || perror_("fflush failed"));

			return std::move(fptr);
		}
};

#endif

