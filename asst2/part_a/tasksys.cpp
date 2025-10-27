#include "tasksys.h"


IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemSerial::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //

    // Reserve appropriate space for threads
    threads.reserve(num_threads);
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) noexcept {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

    // Create thread when running each task.
    for (int i = 0; i < num_total_tasks; i++) {
        std::thread t([=]() { runnable->runTask(i, num_total_tasks); });
        threads.push_back(std::move(t));
    }

    // Join all worker threads before returning.
    for (auto &t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    threads.clear();
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //

    threads.reserve(num_threads);
    this->num_threads = num_threads;
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    threads.clear();
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) noexcept {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

    // Store total tasks for use by runTask and as the initial remaining count.
    this->num_total_tasks = num_total_tasks;
    std::atomic<int> remaining(num_total_tasks);

    // Spawn worker threads that spin when no work is available.
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, &remaining, num_total_tasks]() {
            while (true) {
                // Exit condition: no remaining tasks.
                if (remaining.load(std::memory_order_acquire) <= 0) {
                    break;
                }

                IRunnable* task = nullptr;
                int task_id = -1;

                // Try to pop a task under lock.
                {
                    std::lock_guard<std::mutex> lock(task_queue_mutex);
                    if (!task_queue.empty()) {
                        task_id = task_queue.front().first;
                        task = task_queue.front().second;
                        task_queue.pop();
                    }
                }

                if (task) {
                    // Execute the task.
                    task->runTask(task_id, num_total_tasks);

                    // Decrement remaining tasks.
                    remaining.fetch_sub(1, std::memory_order_acq_rel);
                } else {
                    // No task available: yield to reduce CPU pressure while spinning.
                    std::this_thread::yield();
                }
            }
        });
    }

    for (int i = 0; i < num_total_tasks; i++) {
        std::lock_guard<std::mutex> lock(task_queue_mutex);
        task_queue.push({i, runnable});
    }

    // Wait for all worker threads to finish.
    for (auto &t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //

    threads.reserve(num_threads);
    this->num_threads = num_threads;
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    threads.clear();
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) noexcept {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

    // Total number of tasks (passed to runTask), and an atomic counter
    // to track the number of remaining tasks.
    const int total_tasks = num_total_tasks;
    std::atomic<int> remaining(total_tasks);

    // Condition variable and mutex used for the main thread to wait
    // until all tasks are completed (avoids busy-waiting).
    std::mutex completion_mutex;
    std::condition_variable completion_cv;

    // Create worker threads that sleep when there is no work available.
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, &remaining, &completion_cv, &completion_mutex, total_tasks]() {
            while (true) {
                IRunnable* task = nullptr;
                int task_id = -1;

                {   // Critical section for accessing the task queue.
                    std::unique_lock<std::mutex> lock(task_queue_mutex);

                    // Wait until there is a new task in the queue
                    // or until all tasks have been completed (remaining == 0).
                    task_queue_cv.wait(lock, [this, &remaining]() {
                        return !task_queue.empty() || remaining.load() <= 0;
                    });

                    // If the queue is empty and all tasks are done, exit the thread safely.
                    if (task_queue.empty()) {
                        if (remaining.load() <= 0) {
                            return; // Worker thread exits.
                        } else {
                            // Possible spurious wake-up or another thread took the task,
                            // so continue waiting.
                            continue;
                        }
                    }

                    // Pop one task from the queue.
                    task_id = task_queue.front().first;
                    task = task_queue.front().second;
                    task_queue.pop();
                } // Unlock task_queue_mutex

                if (task) {
                    // Execute the task with the total task count as parameter.
                    task->runTask(task_id, total_tasks);

                    // Atomically decrement the remaining task count.
                    int prev = remaining.fetch_sub(1);

                    // If this was the last task (prev == 1 → now 0),
                    // notify the main thread waiting on completion_cv.
                    if (prev == 1) {
                        std::lock_guard<std::mutex> lk(completion_mutex);
                        completion_cv.notify_one();
                    }
                }
            }
        });
    }

    for (int i = 0; i < num_total_tasks; i++) {
        std::lock_guard<std::mutex> lock(task_queue_mutex);
        task_queue.push({i, runnable});
    }

    // Wake up all worker threads to start processing tasks.
    task_queue_cv.notify_all();

    // Main thread waits until all tasks have been completed
    // (using completion_cv instead of busy-waiting).
    {
        std::unique_lock<std::mutex> lk(completion_mutex);
        completion_cv.wait(lk, [&remaining]() { return remaining.load() == 0; });
    }

    // After all tasks are done, wake up any remaining sleeping workers
    // so they can check remaining == 0 and exit.
    task_queue_cv.notify_all();

    // Join all worker threads before returning.
    for (auto &t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}
