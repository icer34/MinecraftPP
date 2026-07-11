#pragma once

#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool
{
  public:
    ThreadPool(size_t numThreads = std::thread::hardware_concurrency())
    {
        for (size_t i = 0; i < numThreads; i++)
        {
            m_threads.emplace_back(
                [this]
                {
                    while (true)
                    {
                        std::function<void()> task;

                        {
                            std::unique_lock<std::mutex> lock(m_queueMutex);

                            m_cv.wait(lock, [this] { return !m_tasks.empty() || m_stop; });

                            if (m_stop && m_tasks.empty())
                                return;
                            if (m_tasks.empty())
                                continue;

                            task = std::move(m_tasks.front());
                            m_tasks.pop();
                        }

                        try
                        {
                            task();
                        }
                        catch (const std::exception &e)
                        {
                            std::cerr << "ThreadPool threw: " << e.what() << '\n';
                        }
                        catch (...)
                        {
                            std::cerr << "ThreadPool threw unknown exception" << std::endl;
                        }
                    }
                });
        }
    }

    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_stop = true;
        }

        m_cv.notify_all();

        for (auto &thread : m_threads)
        {
            thread.join();
        }
    }

    void enqueue(std::function<void()> task)
    {
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_tasks.emplace(std::move(task));
        }

        m_cv.notify_one();
    }

  private:
    std::vector<std::thread> m_threads;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_queueMutex;
    std::condition_variable m_cv;
    bool m_stop = false;
};