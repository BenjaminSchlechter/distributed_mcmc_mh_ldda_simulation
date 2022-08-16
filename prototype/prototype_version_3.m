
pkg load statistics

% Zufallsvariable: X : \Omega \rightarrow \mathbb{R}
% => zur Modellierung von Ereignisse
% \{X < t\} := \{\omegaup \in \Omega : X(\omegaup) < t\}
% wählt alle Ereignisse mit einer bestimmten Eigenschaft (<t) aus

% \Omega = [0, 1]
% X(\omegaup) = f(\omegaup)

% Indikatorfunktion \mathbb{1}\{A\} = \mathbb{1}_A(\omegaup) = 1 falls \omegaup \in A, 0 sonst

base_filename = "prototype_reconstruction"

set(0, "defaulttextfontsize", 24)
set(0, "defaultaxesfontsize", 16)

show_function = 1;
show_reference = 1;
show_iterative = 0;
calc_iterative = 0;

show_function_verteilungsfunktion = 0;
show_reference_verteilungsfunktion = 0;

show_function_histogram = 0;
show_reference_histogram = 0;
show_iterative_histogram = 0;

show_pdf = 1;
show_pdf_from_inverse_function_samples = 1;

show_inverse_function = 1;
show_inverse_function_from_samples = 1;
show_inverse_function_numerically = 0;
show_inverse_function_by_integration_of_reference_histogram = 0;
show_inverse_function_by_integration_of_rebalanced_histogram = 0;

# 1 normal, 2 semilogy
show_dichtefunktion = 0;
show_dichtefunktion_numerically = 0;
show_dichtefunktion_from_reference_histogram = 0;
show_dichtefunktion_from_iterative_histogram = 0;
show_dichtefunktion_rebalanced = 0;

% show_dichtefunktion = 2;
% show_dichtefunktion_numerically = 2;
% show_dichtefunktion_from_reference_histogram = 2;
% show_dichtefunktion_from_iterative_histogram = 2;
% show_dichtefunktion_rebalanced = 2;

show_dichtefunktion_stdabw = 0;

show_acceptance = 0;

if (0 == 1)
	% show_function = 1;
	% show_reference = 1;
	% show_iterative = 1;
	% calc_iterative = 1;

	% show_function_verteilungsfunktion = 1;
	% show_reference_verteilungsfunktion = 1;

	% show_function_histogram = 0;
	% show_reference_histogram = 0;
	% show_iterative_histogram = 0;

	% show_inverse_function = 1;
	% show_inverse_function_numerically = 1;
	% show_inverse_function_by_integration_of_reference_histogram = 1;
	% show_inverse_function_by_integration_of_rebalanced_histogram = 1;

	# 1 normal, 2 semilogy
	% show_dichtefunktion = 1;
	% show_dichtefunktion_numerically = 0;
	% show_dichtefunktion_from_reference_histogram = 0;
	% show_dichtefunktion_from_iterative_histogram = 1;
	% show_dichtefunktion_rebalanced = 1;

	% show_dichtefunktion = 2;
	% show_dichtefunktion_numerically = 2;
	% show_dichtefunktion_from_reference_histogram = 2;
	% show_dichtefunktion_from_iterative_histogram = 2;
	% show_dichtefunktion_rebalanced = 2;

	% show_dichtefunktion_stdabw = 0;

	% show_acceptance = 0;
endif

legend_str = cell(20,1);
legend_index = 1;

# steepness
% s = 5;
% s = 15;
s = 10;
% s = 25;

% \frac{1}{2}+\frac{\left(\left(e^{s\cdot\left(2\cdot x-1\right)}-1\right)-\left(e^{-s\cdot\left(2\cdot x-1\right)}-1\right)\right)}{\left(2\cdot e^{s}-2\right)\cdot\left(2\cdot e^{-s}+1\right)}
% f = @(x) 1/2 + (exp(s*(2*x-1)) - exp(-s*(2*x-1))) / (2*(exp(s) - 1) * (2*exp(-s) + 1));
% identical:
f = @(x) 1/2 + sinh(s - 2*s*x)/(cosh(s) - 3*sinh(s) - 1);
% f_int_ = @(x) 1/2 * ( (exp(-2*s*x) .* (exp(2*s) + exp(4*s*x))) / (2*s*(exp(2*s) + exp(s) - 2)) + x );
% f_int = @(x) 2*(f_int_(x) - f_int_(0))


% Integral von 0 bis 1 ist 1/2 => keine Dichtefunktion! Aber 2*f ist eine
% mit der zugehörenden Verteilungsfunktion F_2 = integral from 0 to 1 of 2*f
verteilungsfunktion = @(x) x + (2 * sinh(s*x) .* sinh(s - s*x))/(s*(cosh(s) - 3 * sinh(s) - 1));
% x+\frac{2\cdot\sinh(s\cdot x)\cdot\sinh(s-s\cdot x)}{s\cdot(\cosh(s)-3\cdot\sinh(s)-1)}

wkeitsdichtefunktion = @(x) 2*f(x);
% mit Umkehrfunktion f_inv(y/2)
% Umkehrfunktion ableiten:
wkeitsdichtefkt_inv_d = @(y) -(cosh(s) - 3*sinh(s) -1) ./ (4*s * sqrt((-1/2 + y/2).^2 * (cosh(s) - 3*sinh(s) - 1).^2 + 1));
% -\frac{+\cosh(s)-3\cdot\sinh(s)-1}{4\cdot s\cdot\sqrt{(-1/2+x/2)^{2}\cdot(\cosh(s)-3\sinh(s)-1)^{2}+1}}




# Umkehrfunktion von 1/2 + sinh(s - 2*s*x)/(cosh(s) - 3*sinh(s) - 1)
f_inv = @(y) 1/2 - asinh((cosh(s) - 3*sinh(s) - 1)*(y-1/2))/(2*s); # = x
# \frac{1}{2}-\frac{\operatorname{arcsinh}((x-\frac{1}{2})\cdot(\cosh(s)-3\cdot\sinh(s)-1))}{2\cdot s}

# Ableitung d/dy der Umkehrfunktion:
f_inv_d = @(y) -(cosh(s) - 3*sinh(s) -1) ./ (2*s * sqrt((-1/2 + y).^2 * (cosh(s) - 3*sinh(s) - 1).^2 + 1));
# -\frac{+\cosh(s)-3\cdot\sinh(s)-1}{2\cdot s\cdot\sqrt{(-1/2+x)^{2}\cdot(\cosh(s)-3\sinh(s)-1)^{2}+1}}

# zweite Ableitung
f_inv_2d = @(y) ((y - 1/2) * (cosh(s) - 3*sinh(s) - 1)^3) / (2*s*(1 + ((y - 1/2).^2) * ((cosh(s) - 3*sinh(s) - 1).^2)).^(3/2));
% ((x-\frac{1}{2})*(\cosh(s)-3*\sinh(s)-1)^{3})/(2*s*(1+((x-\frac{1}{2})^{2})*((\cosh(s)-3*\sinh(s)-1)^{2}))^{\frac{3}{2}})

# normalisiert
f_inv_dn = @(y) (-2 * s * f_inv_d(y)) / (cosh(s) - 3 * sinh(s) - 1);
% \frac{-2\cdot s\cdot f_inv_d}{\cosh(s)-3\cdot\sinh(s)-1}

# number of samples
% num_samples = 121;
num_samples = 441;
% num_samples = 225;
svec = [1:num_samples];
reference = svec;
iterative = svec;

# number of variables (= length of bitvector)

# direkt als Binärzahl: Problem kleine Änderungen können große Auswirkungen auf Laufzeit haben!
% l = 15;
% to_num = @(rv) sum(rv .* 2.^([0:length(rv)-1]))/(2^l-1);

# zusammengesetzt
l_ = 12;
l = l_*1;
function retval = multi_to_num(rv, l, l_)
	d = l/l_;
	to_num_ = @(rv, n) sum(rv .* 2.^([0:length(rv)-1]))/(2^n-1);
	v = reshape(rv, l_, d);
	retval = sum(arrayfun(@(i) to_num_(transpose(v(:,i)), l_), [1:d]))/d;
endfunction

to_num = @(rv) multi_to_num(rv, l, l_);

for i = 1:num_samples
	rv = unidrnd(2,1,l)-1;
	reference(i) = f(to_num(rv));
	% reference(i) = to_num(rv);
endfor

% works
num_trials = l*10;
num_to_change = 2;
T = 0.075;

% doesnt work
% num_trials = l*10;
% num_to_change = 1;
% T = 0.025;

% works
% num_trials = l*7;
% num_to_change = 2;
% T = 0.06;

% works
% num_trials = l*14;
% num_to_change = 4;
% T = 0.1;

% doesnt work
% num_trials = l*5;
% num_to_change = 2;
% T = 0.05;

% works, but acceptance_rate is 0.8
% num_trials = l*14;
% num_to_change = 4;
% T = 0.2;


sum_accepted = 0;
num_iterations = 0;

acceptance = [1:num_trials] * 0;

if ((1 == show_iterative) || (1 == calc_iterative))
	% wf = @(x) (x*99 + 1);
	wf = @(x) x;

	for i = 1:num_samples
		rv_it = unidrnd(2,1,l)-1;
		% iterative(i) = f(to_num(rv_it));

		for j = 1:num_trials
			rv_new = rv_it;
			for k = 1:num_to_change
				index_to_change = unidrnd(length(rv_new),1);
				rv_new(index_to_change) = 1 - rv_new(index_to_change);
				% rv_new(index_to_change) = unidrnd(2,1,1)-1;
			endfor

			z = rand();
			% p = min(1, exp( -(wf(f(to_num(rv_new))) - wf(f(to_num(rv_it)))) / T ));
			p = exp( -(wf(f(to_num(rv_new))) - wf(f(to_num(rv_it)))) / T );

			% doesnt work
			% p = 1/(wf(f(to_num(rv_new)))/wf(f(to_num(rv_it)))) ^ T;

			% \frac{\exp\left(1-x\right)-1}{e-1}
			% bf = @(x) (exp(1-x)-1)/(exp(1)-1);
			% p = bf(wf(f(to_num(rv_new)))/T) / bf(wf(f(to_num(rv_it)))/T);

			if (z <= p)
				rv_it = rv_new;
				sum_accepted = sum_accepted + 1;
				acceptance(j) = acceptance(j) + 1/num_samples;
			endif
			num_iterations = num_iterations + 1;

			iterative(i) = f(to_num(rv_it));
		endfor
	endfor
endif

# should be ~0.5
acceptance_rate = sum_accepted / num_iterations

hold on;
if (1 == show_acceptance)
	plot([1:num_trials], acceptance);
	plot([1, num_trials], [acceptance_rate, acceptance_rate]);
	legend("acceptance per step", sprintf("overall acceptance_rate: %f", acceptance_rate));
	pause
endif

clf('reset')

# monoton wachsend

# Klasseneinteilung der y-Achse in Intervalle [y_u, y_o)
num_classes = 15;
class_width = 1/num_classes;
eps = 0.000001;
x_start = 0;
class_intervals = [0:num_classes];
cvec = [0:num_classes]/num_classes;
class_wkeit = [1:num_classes]*0;
for i = 1:num_classes
	y_u = (i-1) * class_width;
	y_o = y_u + class_width;

	% x_end = x_start;
	% while (abs(y_o - f(x_end)) >= eps)
		% yerr = y_o - f(x_end);
		% dx = max(eps/4, yerr/2);
		% dy = f(x_end+dx) - f(x_end);
		% x_end = x_end + yerr*dx/dy;
	% endwhile

	% x_end = max(0.0, min(x_end, 1.0));

	# overwrite with analytical value
	x_end = f_inv(y_o);
	class_intervals(i+1) = x_end;
	class_wkeit(i) = x_end - x_start;
	x_start = x_end;
endfor


sumup = @(x,i,n) sum(x([1:i]));

hold on;

# xvalues = svec/num_samples;
xvalues = (svec-1)/(length(svec)-1);

if (1 == show_function)
	plot(xvalues, f(xvalues));
	legend_str(legend_index) = "model"; legend_index++;
	% plot(class_intervals, cvec, "s");
	% legend_str(legend_index) = "Intervalle"; legend_index++;
endif

if (1 == show_reference)
	plot(xvalues, sort(reference), "x");
	% legend_str(legend_index) = "(1) samples auf f(x), gleichverteilt"; legend_index++;
	legend_str(legend_index) = "uniform samples"; legend_index++;
endif

if (1 == show_iterative)
	plot(xvalues, sort(iterative), "o");
	legend_str(legend_index) = "(2) samples auf f(x), MCMC"; legend_index++;
endif


if (1 == show_function_verteilungsfunktion)
	plot(xvalues, verteilungsfunktion(xvalues));
	legend_str(legend_index) = "Verteilungsfunktion F"; legend_index++;
endif

if (1 == show_reference_verteilungsfunktion)
	# Verteilungsfunktion rekonstruieren
	vfkt_ref_int = arrayfun(@(i) sumup(sort(reference), i, num_samples), [0:num_samples]);
	plot([0:num_samples]/num_samples, vfkt_ref_int/vfkt_ref_int(num_samples+1), "color", [0., 0., 0.]);
	% plot([0,1], [0,1], "color", [0, 0, 0]);
	legend_str(legend_index) = "F rekonstruiert von (1)"; legend_index++;
endif


% Fläche soll proportional zur W'keit sein:
% A = 1/num_classes*class_wkeit
% class_wkeit = h/num_classes
% class_wkeit*num_classes

if (1 == show_function_histogram)
	# reference histogram:
	% plot([0:num_classes-1]/(num_classes-1), class_wkeit);
	hist(f(xvalues), [0:num_classes-1]/(num_classes-1), 1, "facecolor", "none", "edgecolor", "g")
	legend_str(legend_index) = "Histogram H"; legend_index++;
endif

h_ref = hist(reference, [0:num_classes-1]/(num_classes-1), 1, "facecolor", "none", "edgecolor", "r");
if (1 == show_reference_histogram)
	hist(reference, [0:num_classes-1]/(num_classes-1), 1, "facecolor", "none", "edgecolor", "r")
	legend_str(legend_index) = "H rekonstruiert von (1)"; legend_index++;
endif

h_it = hist(iterative, [0:num_classes-1]/(num_classes-1), 1, "facecolor", "none", "edgecolor", "m");
if (1 == show_iterative_histogram)
	hist(iterative, [0:num_classes-1]/(num_classes-1), 1, "facecolor", "none", "edgecolor", "m")
	legend_str(legend_index) = "Histogram MCMC (2)"; legend_index++;
endif


# Integral der Histogramme (= Umkehrfunktion)
if (1 == show_inverse_function)
	plot(xvalues, f_inv(xvalues));
	legend_str(legend_index) = "CDF"; legend_index++;
endif

if (1 == show_inverse_function_numerically)
	class_wkeit_integral = arrayfun(@(i) sumup(class_wkeit, i, num_classes), [0:num_classes]);
	plot([0:num_classes]/(num_classes), class_wkeit_integral);
	legend_str(legend_index) = "f^-1 numerisch"; legend_index++;
endif

if (1 == show_inverse_function_by_integration_of_reference_histogram)
	hist_integral = arrayfun(@(i) sumup(h_ref, i, num_classes), [0:num_classes]);
	plot([0:num_classes]/(num_classes), hist_integral);
	legend_str(legend_index) = "f^-1 aus H_(1)"; legend_index++;
endif


# Dichtefunktionen:

# W'keit Proportional zur Fläche
norm_factor = f_inv_d(0.5);
% semilogy/plot
xv = [0:0.001:1];

if (1 == show_dichtefunktion)
	plot(xv, f_inv_d(xv)/norm_factor, "color", "b");
	legend_str(legend_index) = "Dichtefunktion"; legend_index++;
elseif (2 == show_dichtefunktion)
	semilogy(xv, f_inv_d(xv)/norm_factor, "color", "b");
	legend_str(legend_index) = "Dichtefunktion"; legend_index++;
endif

if (1 == show_dichtefunktion_numerically)
	plot([0:num_classes-1]/(num_classes-1), class_wkeit*num_classes/norm_factor, "color", "y");
	legend_str(legend_index) = "Dichtefunktion numerisch"; legend_index++;
elseif (2 == show_dichtefunktion_numerically)
	semilogy([0:num_classes-1]/(num_classes-1), class_wkeit*num_classes/norm_factor, "color", "y");
	legend_str(legend_index) = "Dichtefunktion numerisch"; legend_index++;
endif

if (0 != show_dichtefunktion_stdabw)
	xnc = [0:num_classes-1]/(num_classes-1);
	% p = arrayfun(@(x) 1/(num_classes), xnc);
	p = arrayfun(@(x) 1/(f_inv_2d(x)*num_classes), xnc);
	sa = sqrt(num_samples * p .* (1-p));
	sap = sqrt(abs(num_samples * p .* (1-p)));
	san = sqrt(-abs(num_samples * p .* (1-p)));
	% dfvalues = f_inv_d(xnc);
	dfvalues = class_wkeit*num_classes/norm_factor;
	plot(xnc, (dfvalues + sap)/norm_factor, "color", [1.0, 0.25, 1.0]);
	plot(xnc, (dfvalues - san)/norm_factor, "color", [0.25, 1.0, 1.0]);
	legend_str(legend_index) = "DF+stdabweichung"; legend_index++;
	legend_str(legend_index) = "DF-stdabweichung"; legend_index++;
endif


if (1 == show_dichtefunktion_from_reference_histogram)
	plot([0:num_classes-1]/(num_classes-1), h_ref*num_classes/norm_factor, "color", [1., 0.4, 0.0]);
	legend_str(legend_index) = "Dichtefunktion aus H_(1)"; legend_index++;
elseif (2 == show_dichtefunktion_from_reference_histogram)
	semilogy([0:num_classes-1]/(num_classes-1), h_ref*num_classes/norm_factor, "color", [1., 0.4, 0.0]);
	legend_str(legend_index) = "Dichtefunktion aus H_(1)"; legend_index++;
endif

if (1 == show_dichtefunktion_from_iterative_histogram)
	plot([0:num_classes-1]/(num_classes-1), h_it*num_classes/norm_factor, "color", [0., 0.4, 1.0]);
	legend_str(legend_index) = "Dichtefunktion von H_(2)"; legend_index++;
elseif (2 == show_dichtefunktion_from_reference_histogram)
	semilogy([0:num_classes-1]/(num_classes-1), h_it*num_classes/norm_factor, "color", [0., 0.4, 1.0]);
	legend_str(legend_index) = "Dichtefunktion von H_(2)"; legend_index++;
endif

nf = 1;
if (1 == show_dichtefunktion_rebalanced)
	rebalanced = exp([0:num_classes-1]/(num_classes-1)/T).*h_it*num_classes/norm_factor;
	mid_index = 1 + (num_classes-1)/2;
	nf = rebalanced(mid_index) / (class_wkeit(mid_index)*num_classes/norm_factor)
	plot([0:num_classes-1]/(num_classes-1), rebalanced/nf, "color", [0., 0.8, 0.0]);
	legend_str(legend_index) = "Dichtefunktion angepasst"; legend_index++;
elseif (2 == show_dichtefunktion_rebalanced)
	rebalanced = exp([0:num_classes-1]/(num_classes-1)/T).*h_it*num_classes/norm_factor;
	mid_index = 1 + (num_classes-1)/2;
	% nf = rebalanced(mid_index) / (h_ref(mid_index)*num_classes);
	nf = rebalanced(mid_index) / (class_wkeit(mid_index)*num_classes/norm_factor)
	semilogy([0:num_classes-1]/(num_classes-1), rebalanced/nf, "color", [0., 0.8, 0.0]);
	legend_str(legend_index) = "Dichtefunktion angepasst"; legend_index++;
endif

if (1 == show_inverse_function_by_integration_of_rebalanced_histogram)
	rebalanced = exp([0:num_classes-1]/(num_classes-1)/T).*h_it*num_classes/norm_factor;
	mid_index = 1 + (num_classes-1)/2;
	nf = rebalanced(mid_index) / (class_wkeit(mid_index)*num_classes/norm_factor)
	nf_hist = h_ref(mid_index)*num_classes/norm_factor
	nf = nf * nf_hist

	hist_integral = min(1.0, arrayfun(@(i) sumup(rebalanced/nf, i, num_classes), [0:num_classes]));
	% semilogy([0:num_classes]/(num_classes), hist_integral/nf_hist, "color", [0., 0.0, 0.0]);
	plot([0:num_classes]/(num_classes), hist_integral, "color", [0., 0.0, 0.0]);
	legend_str(legend_index) = "f^-1 aus angepasster DF"; legend_index++;
endif


xref = [0:num_samples-1]/(num_samples-1);
si = sort(reference);
umkehrfkt = [0:num_samples-1]/(num_samples-1);
index = 1;
for i = 1:num_samples
	while (si(index) < umkehrfkt(i))
		index = index + 1;
		if (index >= num_samples)
			index = num_samples;
			break;
		endif
	endwhile

	if umkehrfkt(i) - si(max(index - 1, 1)) < si(index) - umkehrfkt(i)
		index = max(index - 1, 1);
	endif
	
	umkehrfkt(i) = xref(index);
endfor

if (1 == show_pdf)
	pdf = @(x) sinh(s)./(s * sqrt(1 + (1-2*x).*(1-2*x)*sinh(s)*sinh(s) ));
	pdfv = pdf(xvalues);
	plot(xvalues, pdfv/max(pdfv));
	legend_str(legend_index) = "PDF"; legend_index++;
endif

if (1 == show_inverse_function_from_samples)
	plot(xref, umkehrfkt, "d");
	legend_str(legend_index) = "CDF from samples (x)"; legend_index++;
endif

if (1 == show_pdf_from_inverse_function_samples)
	fv = sort(umkehrfkt);
	diff = @(x,i) (x(i+1)-x(i));
	dfv = arrayfun(@(i) diff([fv(1), fv, fv(length(fv))], i), [1:length(fv)]);
	plot(xref, dfv/max(dfv), "r");
	legend_str(legend_index) = "PDF from (x)"; legend_index++;
endif

% plot(xref, min(1, umkehrfkt ./ (exp(-xref/T)*41)), "d");
% plot(xref, (exp(-xref/T*2)*200), "d");
% plot(xref, umkehrfkt./verteilungsfunktion(xref), "d");
% plot(xref, umkehrfkt(1)/verteilungsfunktion(xref(1)) *exp(-xref/T), "d");
% plot(xref, exp(-xref/T) .* umkehrfkt, "d");
% plot(xref, T*exp(xref/T) .* umkehrfkt, "d");
% plot(xref, T*xref.*(log(xref)-1) .* umkehrfkt, "d");

% umkehrfkt_it_int = arrayfun(@(i) sumup(sort(umkehrfkt), i, num_samples), [0:num_samples]);
% plot([0:num_samples]/num_samples, umkehrfkt_it_int/umkehrfkt_it_int(num_samples+1), "color", [1., 0., 1.]);

% diff_f = @(a, x) a(x) - a(x-1);
% diff = arrayfun(@(x) diff_f([0, sort(umkehrfkt), 1], x), [2:num_samples+2]) / num_samples;
% plot([0:num_samples]/num_samples, diff, "color", [.3, 1., 0.2]);
% semilogy([0:num_samples]/num_samples, exp([0:num_samples]/num_samples/T).*diff/(nf*1.1), "color", [.3, 1., 0.2]);

% diff_d = arrayfun(@(x) diff_f([0, sort(iterative), 1], x), [2:num_samples+2]) / num_samples
% semilogy([0:num_samples]/num_samples, 1./diff_d, "color", [.3, 0.2, 1.0]);
% semilogy([0:num_samples]/num_samples, exp([0:num_samples]/num_samples/T).*diff/nf, "color", [.3, 1., 0.2]);


# Verteilungsfunktion rekonstruieren
	% vfkt_it_int = arrayfun(@(i) sumup(sort(iterative), i, num_samples), [0:num_samples]);
% normalization = T*exp([0:num_samples]/num_samples/T);
% norm_func = @(x) T*x.*(log(x)-1);
% norm_func = @(x) T*log(x);
% normalization = norm_func([0:num_samples]/num_samples);
	% plot([0:num_samples]/num_samples, vfkt_it_int/vfkt_it_int(num_samples+1), "color", [1., 0., 0.]);
% plot(vfkt_it_int/vfkt_it_int(num_samples+1), [0:num_samples]/num_samples, "color", [0., 0., 1.]);
% plot(vfkt_it_int/vfkt_it_int(num_samples+1), normalization.*[0:num_samples]/num_samples, "color", [1., 1., 0.]);
% plot([0:num_samples]/num_samples, normalization.*vfkt_it_int/vfkt_it_int(num_samples+1), "color", [1., 0., 1.]);


% xv = [0:0.001:1];
% plot(xv, wkeitsdichtefkt_inv_d(xv*2)/num_classes);

% plot(xvalues, f_inv(xvalues));
% plot(xvalues, f(f_inv(xvalues)));
% plot(xvalues, f(f_inv(xvalues)));
% plot(xvalues, f_inv_d(xvalues));

# sage:
# x,y,s = var('x y s')

% legend(legend_str(1:legend_index-1), "location", "northeastoutside");
legend(legend_str(1:legend_index-1), "location", "northwest");

set(gcf, 'Position', [0 0 800 600]);

xlabel("definition range")

axis([0 1 0 1])

set(0, "defaulttextfontsize", 24)
set(0, "defaultaxesfontsize", 16)

fname = sprintf("%s_n%d_s%d.svg", base_filename, num_samples, s)
pause
print("-dsvg", fname)

% set(gcf, 'Position', get(0, 'Screensize'));
pause






