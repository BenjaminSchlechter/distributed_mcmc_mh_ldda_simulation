
#include <chrono>
#include <string>
#include <iostream>
#include <cstdio>
#include <cstddef>
#include <sys/select.h>
#include <stdio.h>

#include "probsat-execution.hpp"

#include "../../../config.hpp"
#include "../../../util/util.hpp"


uint8_t execute_probsat_(
	uint64_t &num_flips,
	const std::string &filename,
	const probsat_seed_t seed,
	const uint64_t max_flips,
	const std::chrono::seconds &max_exec_time,
	std::string *debug_probsat_output)
{
	uint8_t reason = 0;

	const std::string cmd =
		probsat_cmd +
		((0 < max_flips) ?
			" --maxflips " + std::to_string(max_flips) : "")
		+ " " + filename + " " + std::to_string(seed) + " 2>&1";

	num_flips = std::numeric_limits<uint64_t>::max();

	FILE *pipe_fh = nullptr;
	pipe_fh = popen(cmd.c_str(), "r");
	if (!pipe_fh) {
		perror("popen failed");
		return reason;
	}

	try {
		auto start = std::chrono::high_resolution_clock::now();

		int piped = fileno(pipe_fh);
		if (-1 == piped) {
			perror("fileno(pipe_fh)");
			throw std::runtime_error("fileno failed");
		}

		fcntl(piped, F_SETFL, O_NONBLOCK);
		if (-1 == piped) {
			perror("fcntl(piped, F_SETFL, O_NONBLOCK);");
			throw std::runtime_error("fcntl failed");
		}

		const std::string marker = "c numFlips";
		size_t bsize = 4096;
		assert(bsize <= std::numeric_limits<ssize_t>::max());
		char buffer[bsize];
		std::string bstr = "";

		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(piped, &readfds);

		timespec timeout;
		timespec *timeout_ptr =
			(std::chrono::seconds::zero() == max_exec_time) ?
				nullptr : &timeout;
		assert(std::chrono::ceil<std::chrono::seconds>(
			max_exec_time - std::chrono::seconds::zero()).count()
			<= std::numeric_limits<decltype(timeout.tv_sec)>::max());

		bool running = fd_is_valid(piped);
		while (running)
		{
			auto now = std::chrono::high_resolution_clock::now();
			auto current_execution_time = now - start;

			if (nullptr != timeout_ptr) {
				timeout.tv_sec = (decltype(timeout.tv_sec)) std::max(0l,
					std::chrono::ceil<std::chrono::seconds>(
						max_exec_time - current_execution_time).count());
				timeout.tv_nsec = 1;
			}

			int rval = pselect(
				piped+1, &readfds, nullptr, nullptr, timeout_ptr, nullptr);
			switch (rval) {
				case 0: // timeout
					now = std::chrono::high_resolution_clock::now();
					current_execution_time =
						std::chrono::duration_cast<std::chrono::seconds>
						(now - start);
					if (max_exec_time <= current_execution_time) {
						std::cerr << "timeout after "
							<< current_execution_time.count()
							<< " seconds" << std::endl;
						running = false;
						reason = 1;
					}
					break;
					
				case 1: // data available
					while (running)
					{
						ssize_t num_chars_read =
							read(piped, &buffer[0], bsize);
						buffer[num_chars_read] = 0; // null terminate

						if (0 > num_chars_read)
						{
							if ((errno != EAGAIN)
								and (errno != EWOULDBLOCK))
							{
								perror("read");
								throw std::runtime_error("read failed");
								running = false;
							} else {
								break;
							}
						} else {
							if (nullptr != debug_probsat_output) {
								*debug_probsat_output +=
									std::string(&buffer[0]) + "|BLOCK|";
							}
						}

						if (0 < num_chars_read)
						{
							std::size_t offset = bstr.length();
							std::string bfs = std::string(&buffer[0]);
							bstr += bfs;
							assert(bfs.length()
								== (std::size_t) num_chars_read);

							while (true) {
								size_t pos = bstr.find("\n", offset);
								if (pos == std::string::npos) {
									break;
								}

								std::string line = bstr.substr(0, pos);
								bstr =
									bstr.substr(pos+1, std::string::npos);
								offset = 0;

								// std::cout << "c probsat: "
									// << line << std::endl;

								if (line.starts_with(marker)) {
									size_t pos =
										line.find(": ", marker.length());
									assert(pos != std::string::npos);

									std::string num_flips_str =
										line.substr(pos + 2);
									assert(!num_flips_str.empty());

									num_flips = from_string<std::size_t>
										(num_flips_str);
									running = false;
									reason = 255;
									break;
								}
							}
						}
					}
					
					if (running) {
						running = fd_is_valid(piped);
					}
					break;
					
				default:
					perror("pselect");
					throw std::runtime_error("pselect failed");
					running = false;
					break;
			};
		}
	} catch(std::exception &ex) {
		std::cerr << "exception executing probsat:\n";
		if (!cmd.empty()) { std::cerr << cmd << "\n"; }
		std::cerr << ex.what() << std::endl;
	}
	
	if (pipe_fh) {
		if (nullptr != debug_probsat_output) {
			*debug_probsat_output += "|CLOSE|";
		}

		auto rval = pclose(pipe_fh);
		if (-1 == rval) {
			perror("pclose(pipe_fh)");
		}
		pipe_fh = nullptr;
	}
	
	return reason;
}


std::string execute_cmd(std::string cmd)
{
	FILE *pipe_fh = nullptr;
	pipe_fh = popen(cmd.c_str(), "r");
	if (!pipe_fh) {
		perror("popen failed");
		return "popen failed";
	}

	std::string content;
	try {
		char c;
		while ((c = fgetc(pipe_fh)) != EOF) {
			content += c;
		}
	} catch(std::exception &ex) {
		std::cerr << "exception executing probsat:\n";
		if (!cmd.empty()) { std::cerr << cmd << "\n"; }
		std::cerr << ex.what() << std::endl;
		return content + ex.what();
	}

	if (pipe_fh) {
		auto rval = pclose(pipe_fh);
		if (-1 == rval) {
			perror("pclose(pipe_fh)");
		}
		pipe_fh = nullptr;
	}
	
	return content;
}

std::pair<uint64_t, probsat_return_cause::reason> execute_probsat
	(std::string filename, probsat_seed_t seed)
{
	using reason_t = probsat_return_cause::reason;
	try {
		uint64_t num_flips = std::numeric_limits<uint64_t>::max();

		uint8_t reason = execute_probsat_(
			num_flips,
			filename,
			seed,
			probsat_max_flips,
			probsat_max_exec_time);

		if (1 == reason) {
			return std::make_pair<uint64_t, reason_t>(
				std::move(num_flips), probsat_return_cause::TIMEOUT);
		} else if (255 == reason) {
			if ((0 < num_flips) && (num_flips == probsat_max_flips)) {
				return std::make_pair<uint64_t, reason_t>(
					std::move(num_flips), probsat_return_cause::MAX_FLIPS);
			} else {
				return std::make_pair<uint64_t, reason_t>(
					std::move(num_flips), probsat_return_cause::SUCCESS);
			}
		}
	} catch(std::exception &ex) {
		std::cerr << "exception executing probsat:\n";
		std::cerr << ex.what() << std::endl;
	}

	return std::make_pair<uint64_t, reason_t>(
		std::numeric_limits<uint64_t>::max(), probsat_return_cause::ERROR);
}



