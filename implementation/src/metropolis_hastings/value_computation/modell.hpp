#ifndef MODELL_HPP
#define MODELL_HPP

#include <random>
#include <cassert>


/// helper function for remote call simulation implementing identity function
template<typename prec_t>
prec_t modell_identity(prec_t p) { return p; }


/// helper function for remote call simulation implementing a multiplication
template<typename prec_t>
prec_t modell_multiply(prec_t a, prec_t b) { return a*b; }


/// convert a bitvector into a number
template<typename cfg_vector_t, typename prec_t>
static prec_t bitvector_to_num(const cfg_vector_t &v)
{
	prec_t r = 0.0;
	using size_type = cfg_vector_t::size_type;
	for (size_type i = 0; i < v.data.size(); i++) {
		r *= 2.0;
		r += v[i];
	}
	
	return r / powl(2.0, v.data.size());
}


/// an enhanced modell with added irregularities for probsat simulation
template<typename modell_t>
requires (modell_t::is_value_computation_implementation)
class inaccurate_modell
{
	public:
		using cfg_vector_t = modell_t::cfg_vector_type;
		using prec_type = modell_t::prec_type;
		using prec_t = prec_type;
		constexpr static bool is_value_computation_implementation
			= modell_t::is_value_computation_implementation;

		/// represents a prepared computation for a given configuration
		typedef struct {
			/// embedding the accurate modell
			modell_t::prepared_computation pcom;

			/// error distribution
			std::lognormal_distribution<prec_t> error_distr;
		} prepared_computation;

	private:
		/// using the accurate modell internally
		modell_t modell_;

		// lognormal distributed errors
		// std::lognormal_distribution<prec_t> error_distr; // (mu, sigma)

		/** \brief error strength to apply (errors follow a lognormal
		 *         distribution with sigma error_strength and mu 0)
		 */
		const prec_t error_strength;

		/// error offset to correct = exp(mu + sigma**2 /2) - 1
		const prec_t error_offset;

	public:
	
		/// constructor
		inaccurate_modell(modell_t &modell_, prec_t error_strength_) :
			modell_(modell_),
			error_strength(error_strength_),
			// error_distr(0, error_strength_),
			error_offset(exp(error_strength_*error_strength_/2) - 1)
		{}

		/// prepare computation for a given config vector
		void prepare_computation_with_cfg_vector(
			const cfg_vector_t &v, prepared_computation &pcom)
		{
			modell_.prepare_computation_with_cfg_vector(v, pcom.pcom);
			pcom.error_distr =
				std::lognormal_distribution<prec_t>(0, error_strength);
		}

		/// create a remote function call for one execution
		template<typename prng_t, typename rpmanager_t>
		auto operator()(
			prng_t &prng,
			rpmanager_t &rpmanager,
			prepared_computation &pcom) const
		{
			// rpmanager.execute_rpc(modell_(prng, rpmanager, pcom.pcom))

			prec_t f = pcom.pcom.f;
			prec_t errf = pcom.error_distr(prng) - error_offset;

			return rpmanager.prepare_call("modell_multiply", f, errf);
		}

		/// finish a given computation
		void finish_computation(prepared_computation &pcom) {
			modell_.finish_computation(pcom.pcom);
		}
};


/// a modell with exponential falloff as probsat substitution for simulation
template<typename cfg_vector_t, typename prec_t>
class modell
{
	public:
		using prec_type = prec_t;
		using cfg_vector_type = cfg_vector_t;
		constexpr static bool is_value_computation_implementation = true;

		/// represents a prepared computation for a given configuration
		typedef struct {
			/// value to return
			prec_t f;
		} prepared_computation;

	private:
		/// slope factor
		const prec_t s;

		friend inaccurate_modell<modell<cfg_vector_t, prec_t>>;

	public:
		/// constructor
		modell(prec_t s_) : s(s_) {}


		/// prepare computation for a given config vector
		void prepare_computation_with_cfg_vector(
			const cfg_vector_t &v, prepared_computation &pcom)
		{
			prec_t x = bitvector_to_num<cfg_vector_t, prec_t>(v);
			assert(0 <= x);
			assert(x <= 1);

			static const prec_t nf = (cosh(s) - 3.*sinh(s) - 1);
			pcom.f = 1./2. + sinh(s - 2.*s*x) / nf;
		}

		/// create a remote function call for one execution
		template<typename prng_t, typename rpmanager_t>
		auto operator()(
			prng_t &prng,
			rpmanager_t &rpmanager,
			prepared_computation &pcom) const
		{
			ignore(prng);

			return rpmanager.prepare_call("modell_identity", pcom.f);
		}

		/// finish a given computation (dummy)
		void finish_computation(prepared_computation &pcom) {
			ignore(pcom);
		}
};




#endif
