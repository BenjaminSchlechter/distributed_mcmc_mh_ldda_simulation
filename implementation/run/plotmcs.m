%j.h.lorenz@gmail.com
% 29.11. 14 Uhr
% https://de.wikipedia.org/wiki/Kerndichtesch%C3%A4tzer
pkg load statistics
% pkg load econometrics

function x = gen_normalized_xvec(l)
  x = [0:l-1]/l;
endfunction

function values = hist_to_df(h, num_classes)
	summe = sum(h);
	h = h/summe;
	avgheight = h(1:end-1)+(h(2:end)-h(1:end-1))/2
	area=sum(avgheight)/num_classes
	values = h/area
endfunction


legend_str = cell(20,1);
legend_index = 1;

default_distr_values = sort(csvread("r2/default.csv"));

limit = max(default_distr_values);

num_classes = round(sqrt(length(default_distr_values)))
cvec=[0:num_classes-1]/(num_classes-1);



hold on;
semilogy(gen_normalized_xvec(length(default_distr_values)), default_distr_values/limit);
legend_str(legend_index) = "Normalverteiltes Sampling"; legend_index++;

hdv=hist(default_distr_values/limit, cvec, 1, "facecolor", "none", "edgecolor", "r")
plotyy ([], [], cvec, hdv, @semilogy, @plot);
legend_str(legend_index) = "Histogramm NV"; legend_index++;

% plotyy ([], [], cvec, hist_to_df(hdv, num_classes), @semilogy, @plot);
% legend_str(legend_index) = "Dichtefunktion MCMC"; legend_index++;

% dens = kernel_density(cvec, default_distr_values/limit, 1.0)
% plotyy ([], [], cvec, hdv, @semilogy, @plot);
% legend_str(legend_index) = "Dichtefunktion NV (kde)"; legend_index++;

adjust_values = sort(csvread("r2/adjust.csv"));

hold on;
semilogy(gen_normalized_xvec(length(adjust_values)), adjust_values/limit);
legend_str(legend_index) = "MCMC Sampling 1"; legend_index++;

hav=hist(adjust_values/limit, cvec, 1, "facecolor", "none", "edgecolor", "r");
plotyy ([], [], cvec, hav, @semilogy, @plot);
legend_str(legend_index) = "Histogramm MCMC 1"; legend_index++;

% plotyy ([], [], cvec, hist_to_df(hav, num_classes), @semilogy, @plot);
% legend_str(legend_index) = "Dichtefunktion MCMC"; legend_index++;

adjust_values = sort(csvread("r2/adjust-t300.csv"));

hold on;
semilogy(gen_normalized_xvec(length(adjust_values)), adjust_values/limit);
legend_str(legend_index) = "MCMC Sampling 2"; legend_index++;

hav=hist(adjust_values/limit, cvec, 1, "facecolor", "none", "edgecolor", "r");
plotyy ([], [], cvec, hav, @semilogy, @plot);
legend_str(legend_index) = "Histogramm MCMC 2"; legend_index++;

% plotyy ([], [], cvec, hist_to_df(hav, num_classes), @semilogy, @plot);
% legend_str(legend_index) = "Dichtefunktion MCMC"; legend_index++;




grid on
legend(legend_str(1:legend_index-1), "location", "northeastoutside");
pause
