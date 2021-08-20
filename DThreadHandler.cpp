#include "DThreadHandler.h"
void get_thread_info(int size, int threads, int thread_index, int &start_pos, int &task_on_thread)
{
    int _tasks_on_thread = size / threads;
    int _gain_threads = size % threads;

    if(thread_index < _gain_threads)
    {
        start_pos = (_tasks_on_thread + 1) * thread_index;
        task_on_thread = _tasks_on_thread + 1;
    }
    else
    {
        start_pos = (_tasks_on_thread + 1) * _gain_threads + _tasks_on_thread * (thread_index - _gain_threads);
        task_on_thread = _tasks_on_thread;
    }
}

int get_thread_id()
{
    auto std_id = std::this_thread::get_id();
    return *reinterpret_cast<int*>(&std_id);
}
