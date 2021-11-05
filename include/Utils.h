#pragma once

#include <ctime>
#include <chrono>
#include <thread>
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable

#include <iostream>
#include <iomanip>

class RegularTicker
{
private:
    std::chrono::microseconds period;
    std::chrono::system_clock::time_point previous;
public:
    RegularTicker(long long period_us) : period(period_us) {}

    void SetPeriod(long long period_us)
    {
        period = std::chrono::microseconds(period_us);
    }

    void Start()
    {
        previous = std::chrono::system_clock::now();
    }

    void Sleep()
    {
        std::this_thread::sleep_until( previous + period );
        previous = std::chrono::system_clock::now();
/*
        std::chrono::system_clock::time_point diff = previous + period;
        std::time_t epoch_time = std::chrono::system_clock::to_time_t(diff);
        std::cout << "Sleep until: " << std::ctime(&epoch_time) << std::endl;
*/
    }
};

class ProductionManager
{
private:
    std::mutex mtx;
    std::condition_variable cv;
    std::unique_lock<std::mutex> production_lock;
    bool ready = true;

public:
    ProductionManager()
    {
        production_lock = std::unique_lock<std::mutex>(mtx);
        production_lock.unlock();
    }

    ~ProductionManager()  // NOTE: Make sure he is destructed before thread holders (destruction is in reverse of declaration).
    {
        ready = true;  // Ensure client stops waiting.
        cv.notify_all();  // Ensure no thread hangs when destructing.
    }

    bool ShouldProduce()
    {
        if (ready || production_lock.owns_lock())  // Drop until the product was finished (if async).
            return false;

        production_lock.lock();
        return true;
    }

    void NotifyDone()
    {
        ready = true;
        production_lock.unlock();
        cv.notify_one();
    }

    void Request()
    {
        std::unique_lock<std::mutex> lck(mtx);
        ready = false;
        cv.wait(lck, [this]{ return ready; });
    }
};
