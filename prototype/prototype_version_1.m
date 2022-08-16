
pkg load statistics

# steepness
% s = 80;
s = 40
%s = 4;

base_filename = "prototype_ldda"

cfg_plot_uniform_sampling = 0
cfg_plot_large_deviation_distributions_samples = 1

set(0, "defaulttextfontsize", 24)
set(0, "defaultaxesfontsize", 16)

legend_str = cell(20,1);
legend_index = 1;

% \frac{\left(e^{s\cdot x}-1\right)}{e^{s}-1}
% \frac{1}{2}+\frac{\left(e^{s\cdot\left(2\cdot x-1\right)}-1\right)}{2\cdot\left(e^{s}-1\right)}-\frac{\left(e^{-s\cdot\left(2\cdot x-1\right)}-1\right)}{2\cdot\left(e^{s}-1\right)}
% \frac{1}{2}+\frac{\left(e^{s\cdot\left(2\cdot x-1\right)}-e^{-s\cdot\left(2\cdot x-1\right)}\right)}{2\cdot\left(e^{s}-1\right)}

% f_ = @(x) 1/2 + (exp(s*(2*x-1)) - exp(-s*(2*x-1)))/(2*e^s - 1);
% f = @(x) min(1, max(0, f_(x))); # ensure range

% f_int = @(x) 1/2 * ( (exp(s*(2*x-1)) + exp(s - 2*s*x)) / (2*s*(exp(s) - 1)) + x );
% f_int = @(x) 1/2 * ( x - (exp(s*(2*x-1)) + exp(s - 2*s*x)) / (s - 2*s*exp(s)));

% 1/2 * (1 + (exp(-s) + exp(s))/(2*s*(exp(s) -1)))


% \frac{1}{2}+\frac{\left(\left(e^{s\cdot\left(2\cdot x-1\right)}-1\right)-\left(e^{-s\cdot\left(2\cdot x-1\right)}-1\right)\right)}{\left(2\cdot e^{s}-2\right)\cdot\left(2\cdot e^{-s}+1\right)}
% f = @(x) 1/2 + (exp(s*(2*x-1)) - exp(-s*(2*x-1))) / (2*(exp(s) - 1) * (2*exp(-s) + 1));
% identical:
f = @(x) 1/2 + sinh(s - 2*s*x)/(cosh(s) - 3*sinh(s) - 1);
% f_int_ = @(x) 1/2 * ( (exp(-2*s*x) .* (exp(2*s) + exp(4*s*x))) / (2*s*(exp(2*s) + exp(s) - 2)) + x );
% f_int = @(x) 2*(f_int_(x) - f_int_(0))


# number of samples
num_samples = 100
svec = [1:num_samples];
reference = svec;
iterative = svec;

# number of variables (= length of bitvector)
number_of_variables = 150
l = number_of_variables;
to_num = @(rv) sum(rv .* 2.^([0:length(rv)-1]))/(2^l-1);


for i = 1:num_samples
	rv = unidrnd(2,1,l)-1;
	reference(i) = f(to_num(rv));
endfor

markov_chain_length = 500
% num_trials = 2;
num_trials = markov_chain_length;

theta = -0.01
T = theta;

legend_location = "northwest";
if (T < 0)
	legend_location = "southeast";
endif


% wf = @(x) (x*99 + 1);
wf = @(x) x;

for i = 1:num_samples
	rv_it = unidrnd(2,1,l)-1;
	iterative(i) = f(to_num(rv_it));
	for j = 1:num_trials
		rv_new = rv_it;
		index_to_change = unidrnd(length(rv_new),1);
		rv_new(index_to_change) = 1 - rv_new(index_to_change);
		iterative(i) = f(to_num(rv_new));

		z = rand();
		p = min(1, exp( -(wf(f(to_num(rv_new))) - wf(f(to_num(rv_it)))) / T ));
		if (z <= p)
			rv_it = rv_new;
		endif
	endfor
endfor


hist_resolution = 25;
hist_counter = [1:hist_resolution] * 0;
hist_ref = [1:hist_resolution] * 0;
for i = 1:num_samples
	index_r = round(iterative(i)*(hist_resolution-1)) + 1;
	hist_counter(index_r) = hist_counter(index_r) + 1;
	index_i = round(reference(i)*(hist_resolution-1)) + 1;
	hist_ref(index_i) = hist_ref(index_i) + 1;
endfor
hist_counter = hist_counter/num_samples;
hist_ref = hist_ref/num_samples;

hist_correct = [1:hist_resolution] * 0;
for i = 1:hist_resolution
	w = (i-0.5)/hist_resolution;
	hist_correct(i) = min(1, max(0, hist_counter(i)/num_trials * exp(w*T)));
	% hist_correct(i) = min(1, max(0, hist_counter(i)/num_trials * exp(w/T)));
endfor

hvec = ([1:hist_resolution]-0.5)/hist_resolution *num_samples;

hold on;

plot(svec, f((svec-1)/(length(svec)-1)));
	% plot(svec, f_int((svec-1)/(length(svec)-1)));
legend_str(legend_index) = "model"; legend_index++;

if (1 == cfg_plot_uniform_sampling)
	plot(svec, sort(reference), "x");
	legend_str(legend_index) = "uniform sampled"; legend_index++;
endif

if (1 == cfg_plot_large_deviation_distributions_samples)
	plot(svec, sort(iterative), "o");
	legend_str(legend_index) = sprintf("samples from large deviation\ndistributions algorithm \Theta = %d", T); legend_index++;
endif

l = legend(legend_str(1:legend_index-1), "location", legend_location);
set(l, 'interpreter', 'latex');

xlabel("definition range / sample id")

axis([0 num_samples 0 1])

fname = sprintf("%s_n%d_s%d_t%d_l%d.svg", base_filename, num_samples, s, theta, markov_chain_length)
print("-dsvg", fname)

% plot(hvec, hist_counter, "s");
% legend("f", "reference", "mc", "hist");
% plot(hvec, hist_correct, "d");
% legend("f", "reference", "mc", "hist", "hist corrected");

% plot(svec, f((svec-1)/(length(svec)-1)));
% draw_ref = @(x,y) plot(x, y, 'x');
% draw_it = @(x,y) plot(x, y, 'o');
% draw_h = @(x,y) semilogy(x, y, 's');
% draw_hc = @(x,y) semilogy(x, y, 'd');
% plotyy(svec, sort(reference), hvec, hist_counter, draw_ref, draw_h);
% plotyy(svec, sort(iterative), hvec, hist_correct, draw_it, draw_hc);
% legend("f", "reference", "mc", "hist", "hist corrected");

% pause;
% hold off;

% hist(round(iterative*10)/10);
% pause;


% plot(svec, f((svec-1)/(length(svec)-1)));

% plot(hvec, hist_ref, "o");
% plot(hvec, hist_counter, "x");
% plot(hvec, hist_correct, "s");
pause


