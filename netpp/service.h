#pragma once
#include <asio/io_context.hpp>
#include <asio/thread.hpp>
#include <chrono>
#include <memory>
#include <thread>
namespace net
{
using namespace asio;

inline std::shared_ptr<io_context>& context()
{
	static auto context = std::make_shared<io_context>();
	return context;
}

inline std::vector<std::thread>& workers()
{
    static std::vector<std::thread> threads = []()
    {
        std::vector<std::thread> result;
        for(size_t i = 0; i < std::thread::hardware_concurrency(); ++i)
        {
            std::thread th([]()
            {
                auto io = context();
                while(io)
                {
                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(100ms);
                    context()->run();
                }
            });
            result.emplace_back(std::move(th));
        }

        return result;
    }();
    return threads;
}

inline void init_services()
{
	context();
	workers();
}

inline void deinit_services()
{
	context()->stop();

    auto& threads = workers();

    for(auto& worker : threads)
    {
        if(worker.joinable())
        {
            worker.join();
        }
    }
	threads.clear();

	context().reset();
}

} // namespace net
