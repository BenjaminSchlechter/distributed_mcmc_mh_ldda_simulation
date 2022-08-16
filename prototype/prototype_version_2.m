
pkg load statistics

% Zufallsvariable: X : \Omega \rightarrow \mathbb{R}
% => zur Modellierung von Ereignisse
% \{X < t\} := \{\omegaup \in \Omega : X(\omegaup) < t\}
% wählt alle Ereignisse mit einer bestimmten Eigenschaft (<t) aus

% \Omega = [0, 1]
% X(\omegaup) = f(\omegaup)

% Indikatorfunktion \mathbb{1}\{A\} = \mathbb{1}_A(\omegaup) = 1 falls \omegaup \in A, 0 sonst


set(0, "defaulttextfontsize", 24)
set(0, "defaultaxesfontsize", 16)

legend_str = cell(20,1);
legend_index = 1;

base_filename = "prototype_udistr_hist"
cfg_display_histograms = 1
cfg_display_cdf = 0
% base_filename = "prototype_udistr_cdf"
% cfg_display_histograms = 0
% cfg_display_cdf = 1

# steepness
s = 10;

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

# normalisiert
f_inv_dn = @(y) (-2 * s * f_inv_d(y)) / (cosh(s) - 3 * sinh(s) - 1);
% \frac{-2\cdot s\cdot f_inv_d}{\cosh(s)-3\cdot\sinh(s)-1}

# number of samples
num_samples = 441;
svec = [1:num_samples];
reference = svec;
iterative = svec;

# number of variables (= length of bitvector)
l = 15;
to_num = @(rv) sum(rv .* 2.^([0:length(rv)-1]))/(2^l-1);


for i = 1:num_samples
	rv = unidrnd(2,1,l)-1;
	reference(i) = f(to_num(rv));
endfor


num_trials = 10;
T = 0.01;

% legend_location = "northeastoutside";
legend_location = "northwest";
% if (T < 0)
	% legend_location = "southeast";
% endif

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

# monoton wachsend

# Klasseneinteilung der y-Achse in Intervalle [y_u, y_o)
num_classes = 21;
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


hold on;

# xvalues = svec/num_samples;
xvalues = (svec-1)/(length(svec)-1);

plot(xvalues, f(xvalues));
legend_str(legend_index) = "model function"; legend_index++;

plot(xvalues, sort(reference), "x");
legend_str(legend_index) = "uniform sampled"; legend_index++;
% plot(xvalues, sort(iterative), "o");

plot(class_intervals, cvec, "s");
legend_str(legend_index) = "histogram intervals"; legend_index++;

% plot(xvalues, verteilungsfunktion(xvalues));

% Fläche soll proportional zur W'keit sein:
% A = 1/num_classes*class_wkeit
% class_wkeit = h/num_classes
% class_wkeit*num_classes

# reference histogram:
% plot([0:num_classes-1]/(num_classes-1), class_wkeit);
% legend_str(legend_index) = "PDF"; legend_index++;


% plot(xvalues, f_inv_dn(xvalues));
% legend_str(legend_index) = "PDF"; legend_index++;

if (1 == cfg_display_histograms)
	hist(f(xvalues), [0:num_classes-1]/(num_classes-1), 1, "facecolor", "none", "edgecolor", "g")
	legend_str(legend_index) = "reference histogram"; legend_index++;

	# histogram:
	hist(reference, [0:num_classes-1]/(num_classes-1), 1, "facecolor", "none", "edgecolor", "r")
	legend_str(legend_index) = "histogram from samples"; legend_index++;
	% hist(iterative, [0:num_classes-1]/(num_classes-1), 1)

	% fv = sort(reference);
	% diff = @(x,i) 2/(x(i+2)-x(i));
	% dfv = arrayfun(@(i) diff([fv(1), fv, fv(num_samples)], i), [1:num_samples])
	% plot(xvalues, dfv, "r");

	legend_str(legend_index) = "histogram by derivation"; legend_index++;
endif

# ------------

if (1 == cfg_display_cdf)
	plot(xvalues, f_inv(xvalues));
	legend_str(legend_index) = "CDF"; legend_index++;

	# Integral der Histogramme (= Umkehrfunktion)
	sumup = @(x,i,n) sum(x([1:i]));
	class_wkeit_integral = arrayfun(@(i) sumup(class_wkeit, i, num_classes), [0:num_classes]);
	plot([0:num_classes]/(num_classes), class_wkeit_integral);
	legend_str(legend_index) = "CDF reference"; legend_index++;

	plot(sort(reference), xvalues, "r");
	legend_str(legend_index) = "CDF from samples"; legend_index++;

	h = hist(reference, [0:num_classes-1]/(num_classes-1), 1, "facecolor", "none", "edgecolor", "r");
	hist_integral = arrayfun(@(i) sumup(h, i, num_classes), [0:num_classes]);
	plot([0:num_classes]/(num_classes), hist_integral);
	legend_str(legend_index) = "CDF approximation"; legend_index++;
endif
# ------------
legend(legend_str(1:legend_index-1), "location", legend_location);

xlabel("definition range")

set(gcf, 'Position', [0 0 800 600]);
% set(gcf, 'Position', get(0, 'Screensize'));

axis([0 1 0 1])

fname = sprintf("%s_n%d_s%d_t%d.svg", base_filename, num_samples, s, T)
pause

print("-dsvg", fname)

% FigH = figure('Position', get(0, 'Screensize'));
% fig = figure('Position', get(0, 'Screensize'));
% saveas(fig, fname, 'svg');

pause

# ------------


# W'keit Proportional zur Fläche
norm_factor = f_inv_d(0.5);
% semilogy/plot
xv = [0:0.001:1];
% semilogy(xv, f_inv_d(xv)/norm_factor, "color", "b");

% semilogy([0:num_classes-1]/(num_classes-1), class_wkeit*num_classes/norm_factor, "color", "y");

% semilogy([0:num_classes-1]/(num_classes-1), h*num_classes/norm_factor, "color", [1., 0.4, 0.0]);


# Verteilungsfunktion rekonstruieren
vfkt_ref_int = arrayfun(@(i) sumup(sort(reference), i, num_samples), [0:num_samples]);
plot([0:num_samples]/num_samples, vfkt_ref_int/vfkt_ref_int(num_samples+1), "color", [0., 0., 0.]);
% plot([0,1], [0,1], "color", [0, 0, 0]);


% xv = [0:0.001:1];
% plot(xv, wkeitsdichtefkt_inv_d(xv*2)/num_classes);

% plot(xvalues, f_inv(xvalues));
% plot(xvalues, f(f_inv(xvalues)));
% plot(xvalues, f(f_inv(xvalues)));
% plot(xvalues, f_inv_d(xvalues));

# sage:
# x,y,s = var('x y s')

% set(gcf, 'Position', get(0, 'Screensize'));
pause




