#pragma once

#include <thread>
#include <future>

#include <iostream>
#include <cerrno>  // Get error number
#include <cstring>  // Ger error string

namespace comms
{

typedef void (*threadfunc)();

class ThreadedDuplex
{
private:
    bool input_active = false;
    std::mutex input_mutex;
    std::thread input_thread;

    bool output_active = false;
    std::mutex output_mutex;
    std::thread output_thread;

public:

    void JoinInputThread()
    {
        if (input_thread.joinable())
        {
            input_thread.join();
            input_active = false;
        }
    }

    void JoinOutputThread()
    {
        if (output_thread.joinable())
        {
            output_thread.join();
            output_active = false;
        }
    }

    void JoinThreads()
    {
        JoinInputThread();
        JoinOutputThread();
    }

    ~ThreadedDuplex()
    {
        JoinThreads();
    }

    bool IsInputRunning()
    {
        return input_active && !input_thread.joinable();
    }

    bool IsOutputRunning()
    {
        return output_active && !output_thread.joinable();
    }

    void StartInputThread(threadfunc input_func)
    {
        if (!input_mutex.try_lock()) return;
        if (IsInputRunning())
        {
            input_mutex.unlock();
            return;
        }
        input_active = true;
        input_mutex.unlock();
        input_thread = std::thread([this, input_func] {  // NOTE: If passed as reference, second method in start (output) gets called in both these start methods.
            try
            {
                input_func();
            }
            catch (...)
            {
                std::cerr << "[THREADING] Input thread exception: " << std::strerror(errno) << std::endl;
            }
            input_active = false;
        });
    }

    void StartOutputThread(threadfunc output_func)
    {
        if (!output_mutex.try_lock()) return;
        if (IsOutputRunning())
        {
            output_mutex.unlock();
            return;
        }
        output_active = true;
        output_mutex.unlock();
        output_thread = std::thread([this, output_func] {
            try
            {
                output_func();
            }
            catch (...)
            {
                std::cerr << "[THREADING] Output thread exception: " << std::strerror(errno) << std::endl;
            }
            output_active = false;
        });
    }

    void Start(threadfunc input_func, threadfunc output_func)
    {
        StartInputThread(input_func);
        StartOutputThread(output_func);
    }

    template <class Function, class Parent>
    void StartInputThread(Function input_func, Parent * context)
    {
        if (!input_mutex.try_lock()) return;
        if (IsInputRunning())
        {
            input_mutex.unlock();
            return;
        }
        input_active = true;
        input_mutex.unlock();
        input_thread = std::thread([this, input_func, context] {
            try
            {
                (context->*input_func)();
            }
            catch (...)
            {
                std::cerr << "[THREADING] Input thread exception: " << std::strerror(errno) << std::endl;
            }
            input_active = false;
        });
    }

    template <class Function, class Parent>
    void StartOutputThread(Function output_func, Parent * context)
    {
        if (!output_mutex.try_lock()) return;
        if (IsOutputRunning())
        {
            output_mutex.unlock();
            return;
        }
        output_active = true;
        output_mutex.unlock();
        output_thread = std::thread([this, output_func, context] {
            try
            {
                (context->*output_func)();
            }
            catch (...)
            {
                std::cerr << "[THREADING] Output thread exception: " << std::strerror(errno) << std::endl;
            }
            output_active = false;
        });
    }

    template <class Function, class Parent>
    void Start(Function input_func, Function output_func, Parent * context)
    {
        StartInputThread(input_func, context);
        StartOutputThread(output_func, context);
    }
};

}
