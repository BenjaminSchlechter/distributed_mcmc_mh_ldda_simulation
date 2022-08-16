#ifndef LAZY_ROUND_ROBIN_SCHEDULER_HPP
#define LAZY_ROUND_ROBIN_SCHEDULER_HPP

#include <cstddef>
#include <vector>
#include <map>
#include <cassert>

/// round robin load balancing of up to workload_capacity computations inside a workgroup
template<std::size_t workload_capacity>
class lazy_round_robin_scheduler
{
	private:
		/// number of workers
		const int size;

		/// number of tasks on each worker
		std::vector<std::size_t> num_tasks_scheduled;

		/// lookup table for worker id based upon number of currently scheduled tasks
		std::multimap<std::size_t, int> ids_for_scheduled;

	public:

		/// check if any worker is without work
		bool is_process_without_work()
		{
			if (0 >= size) { return false; }

			auto first_it = ids_for_scheduled.begin();
			return (0 == first_it->first);
		}

		///check if any worker has less than workload_capacity jobs
		bool worker_available()
		{
			if (0 >= size) { return false; }

			auto first_it = ids_for_scheduled.begin();
			return (workload_capacity > first_it->first);
		}

		/// get a worker id (or -1 if none available) for an task
		int get_id_to_schedule_task_on()
		{
			if (0 >= size) { return -1; }

			auto first_it = ids_for_scheduled.begin();
			if (workload_capacity == first_it->first) {
				return -1;
			}

			int id = first_it->second;
			const std::size_t num_scheduled_old =
				num_tasks_scheduled[id]++;
			assert(first_it->first == num_scheduled_old);

			ids_for_scheduled.erase(first_it);
			ids_for_scheduled.emplace(num_scheduled_old + 1, id);

			// std::cout << "get_id_to_schedule_task_on: "
				// << id << " is now busy with "
				// << (num_scheduled_old+1) << " tasks" << std::endl;

			return id;
		}

		/// mark one task on a worker with the given id as finished
		void task_finished(int id)
		{
			if (0 >= size) { return; }

			assert(0 <= id);
			assert(id < size);

			// std::cout << "worker " << id
				// << " with " << num_tasks_scheduled[id]
				// << " tasks finished one" << std::endl;

			std::size_t num_scheduled_old = num_tasks_scheduled[id]--;
			assert(0 < num_scheduled_old);

			auto it = ids_for_scheduled.equal_range(num_scheduled_old);

			for (auto i = it.first; i != it.second; i++) {
				if (i->second == id) {
					ids_for_scheduled.erase(i);
					ids_for_scheduled.emplace(num_scheduled_old - 1, id);
					return;
				}
			}

			assert(false);
		}
		
		/// constructor
		lazy_round_robin_scheduler(int size) :
			size(size), num_tasks_scheduled(size), ids_for_scheduled()
		{
			static_assert(0 < workload_capacity);
			num_tasks_scheduled.resize(size);
			for (int id = 0; id < size; id++) {
				num_tasks_scheduled[id] = 0;
				ids_for_scheduled.emplace(0, id);
			}
		}
};

#endif
