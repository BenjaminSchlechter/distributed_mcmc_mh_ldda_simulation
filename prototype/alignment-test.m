
pkg load statistics

s = 15

% x_overlap = 0.1
% x_split = 0.2

% num_samples = 100;

x_overlap = 0.05
x_split = 0.075

num_samples = 200;

base_filename = "alignment"

set(0, "defaulttextfontsize", 24)
set(0, "defaultaxesfontsize", 16)


x1_end   = x_split + x_overlap/2
x2_start = x_split - x_overlap/2

nsamples = [ceil(num_samples*x1_end), ceil(num_samples*(1-x2_start))]
assert(nsamples(1)+nsamples(2) >= num_samples)

overlapping_range = [num_samples-nsamples(2)+1:nsamples(1)];
num_overlapping_samples = length(overlapping_range)

overlapping_indices_x1 = overlapping_range;
overlapping_indices_x2 = [1:length(overlapping_range)];
assert(length(overlapping_indices_x1) == length(overlapping_indices_x2))

x = [0:num_samples-1]/(num_samples-1);

x1 = x(1:nsamples(1));
x2 = x(num_samples-nsamples(2)+1:end);

assert(nsamples(1) == length(x1));
assert(nsamples(2) == length(x2));

f = @(x) 1/2 + sinh(s - 2*s*x)/(cosh(s) - 3*sinh(s) - 1);


error_strength = 0.35 % 3.5
avg_runs = 325 % 300
falloff = 3

% error_strength = 0.3
% avg_runs = 400
% falloff = 5

% error_strength = 0.25
error_offset = exp(error_strength*error_strength/2) - 1

% error_1 = mean(lognrnd (0, error_strength, avg_runs, nsamples(1))) - error_offset;
% error_2 = mean(lognrnd (0, error_strength, avg_runs, nsamples(2))) - error_offset;

% falloff = 10
% falloff = 2
errf1 = @(x) 1 - (exp(x    *falloff)-1)/(exp(falloff)-1);
errf2 = @(x) 1 - (exp((1-x)*falloff)-1)/(exp(falloff)-1);

error_1 = [1:nsamples(1)];
error_2 = [1:nsamples(2)];

for i = 1:nsamples(1)
	errx = (i - 1)/nsamples(1); % off by one to prevent NaN
	% avg_runs_ = min(round(avg_runs*errf1(errx)), 10);
	% error_1(i) = mean(lognrnd (0, error_strength, avg_runs_, 1)) - error_offset;
	error_strength_ = max(0.1, error_strength*(1 - errf1(errx)));
	error_offset_ = exp(error_strength_*error_strength_/2) - 1;
	error_1(i) = mean(lognrnd (0, error_strength_, avg_runs, 1)) - error_offset_;
endfor

for i = 1:nsamples(2)
	errx = (i)/nsamples(2); % off by one to prevent NaN
	% avg_runs_ = min(round(avg_runs*errf2(errx)), 10);
	% error_2(i) = mean(lognrnd (0, error_strength, avg_runs_, 1)) - error_offset;
	error_strength_ = max(0.1, error_strength*(1 - errf2(errx)));
	error_offset_ = exp(error_strength_*error_strength_/2) - 1;
	error_2(i) = mean(lognrnd (0, error_strength_, avg_runs, 1)) - error_offset_;
endfor


y1 = f(x1);
y2 = f(x2);

% y1_scale = 1.124
y1_scale = 1.300

debug_err1_avg = mean(error_1)
debug_err1_min = min(error_1)
debug_err1_max = max(error_1)

debug_err2_avg = mean(error_2)
debug_err2_min = min(error_2)
debug_err2_max = max(error_2)

y1_oe = y1 .* error_1;
y1_we = y1_oe * y1_scale;
y2_we = y2 .* error_2;

hold on;

plot(x, f(x))

plot(x1, y1_oe, "x") % part 1
plot(x1, y1_we, "s") % part 1 scaled
plot(x2, y2_we, "o") % part 2


max_err = 0;
min_err = Inf;
best_ov_len = 0;
best_ov_start = 0;
best_avg_sf = 0;

worst_ov_len = 0;
worst_ov_start = 0;
worst_avg_sf = 0;

num_all_avg_sf = 0;
all_avg_sf = [];

min_overlapping_for_adjustment = 5;
for ov_len = min_overlapping_for_adjustment:num_overlapping_samples
	for ov_rstart = 0:num_overlapping_samples-ov_len

		ovrange_1 = overlapping_indices_x1(1+ov_rstart:ov_len+ov_rstart);
		ovrange_2 = overlapping_indices_x2(1+ov_rstart:ov_len+ov_rstart);
		assert(length(ovrange_1) == length(ovrange_2));
		assert(length(ovrange_1) == ov_len);

		ovy_1_ref = y1_we(overlapping_indices_x1);
		ovy_2_ref = y2_we(overlapping_indices_x2);

		ovy_1 = y1_we(ovrange_1); % overlapping values from each
		ovy_2 = y2_we(ovrange_2);

		avg_sf = mean(ovy_1 / ovy_2);
		assert(isfinite(avg_sf));
		
		num_all_avg_sf = num_all_avg_sf + 1;
		all_avg_sf = [avg_sf, all_avg_sf];
		

		% abs_err = sum(abs(ovy_1/avg_sf - ovy_2));
		% avg_err = abs_err/length(ovrange_1);
		% avg_err = abs_err/(length(ovrange_1)+1);
		% avg_err = abs_err/(2*length(ovrange_1));
		
		% abs_err = sum(abs(ovy_1/avg_sf - ovy_2).^2);
		% avg_err = abs_err/length(ovrange_1);

		abs_err = sum(abs(ovy_1_ref/avg_sf - ovy_2_ref));
		avg_err = abs_err/length(ovy_1_ref);

		if (avg_err < min_err)
			best_ov_len = ov_len;
			best_ov_start = ov_rstart;
			best_avg_sf = avg_sf;
			min_err = avg_err;
		endif

		if (avg_err > max_err)
			worst_ov_len = ov_len;
			worst_ov_start = ov_rstart;
			worst_avg_sf = avg_sf;
			max_err = avg_err;
		endif
	endfor
endfor

printf("\n")
printf("target scale:\n")
y1_scale = y1_scale

printf("\n")
printf("best fitting:\n")
min_err = min_err
best_ov_len = best_ov_len
best_ov_start = best_ov_start
best_avg_sf = best_avg_sf

printf("\n")
printf("worst approx:\n")
max_err = max_err
worst_ov_len = worst_ov_len
worst_ov_start = worst_ov_start
worst_avg_sf = worst_avg_sf

printf("\n")
printf("statistical approx:\n")
maximum_sf = max(all_avg_sf)
median_sf = median(all_avg_sf)
average_sf = mean(all_avg_sf)
minimum_sf = min(all_avg_sf)

plot(x1, y1_we/best_avg_sf, "d")


% y1 = y2 * sf
% ovrange_1 = overlapping_indices_x1;
% ovrange_2 = overlapping_indices_x2;
% assert(length(ovrange_1) == length(ovrange_2));
% ovy_1 = y1_we(ovrange_1);
% ovy_2 = y2_we(ovrange_2);

% avg_sf = mean(ovy_1 / ovy_2)
% abs_err = sum(abs(ovy_1/avg_sf - ovy_2));
% avg_err = abs_err/length(ovrange_1)

% plot(x1, y1_we/avg_sf, "d")


% plot(x1(overlapping_indices_x1), f(x1(overlapping_indices_x1)), "s")
% plot(x2(overlapping_indices_x2), f(x2(overlapping_indices_x2)), "d")

set(gcf, 'Position', [0 0 800 600]);

legend("model", "samples part 1", "samples part 1 scaled", "samples part 2", "fitted", "location", "southeast");

xlabel("definition range")

axis([0 1 0 1])

fname = sprintf("%s.svg", base_filename)
pause
print("-dsvg", fname)

pause


% counter example - metric isnt optimal
% target scale:
% y1_scale = 1.3000

% best fitting:
% min_err = 2.2511e-03
% best_ov_len = 5
% best_ov_start = 4
% best_avg_sf = 1.3094

% worst approx:
% max_err = 5.8079e-03
% worst_ov_len = 8
% worst_ov_start = 2
% worst_avg_sf = 1.3012
