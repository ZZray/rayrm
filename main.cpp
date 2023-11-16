/**
 * @author: rayzhang
 * std::filesystem::remove_all 不是线程安全的，如果要删除的路径之间存在依赖关系，这可能会导致问题
 */
#include <filesystem>
#include <vector>
#include <string>
#include <future>
#include <format>
#include <iostream>
#include <queue>

namespace fs = std::filesystem;

// 用于在控制台打印红色文本的函数
void printRed(const std::string& message)
{
    printf("\033[31m%s\033[0m\n", message.c_str());
}

// 用于在控制台打印绿色文本的函数
void printGreen(const std::string& message)
{
    printf("\033[32m%s\033[0m\n", message.c_str());
}

// 用于在控制台打印黄色文本的函数
void printYellow(const std::string& message)
{
    printf("\033[33m%s\033[0m\n", message.c_str());
}

using RemoveResult = std::pair<bool, std::string>;

void printResult(const RemoveResult& res)
{
    const auto& [isOk, message] = res;
    if (isOk) {
        printGreen(message);
    }
    else {
        printRed(message);
    }

}
class ThreadPool {
public:
    ThreadPool() {
        for (int i = 0; i < static_cast<int>(std::thread::hardware_concurrency()) * 5; ++i) {
            _threads.emplace_back([this] {
                while (!_stop) {
                    std::unique_lock lock(_mtx);
                    _condVar.wait(lock, [this] { return !_tasks.empty() || _stop; });

                    if (_stop) {
                        return;
                    }
                    auto task = std::move(_tasks.front());
                    _tasks.pop();
                    lock.unlock();
                    task();
                }
            });
        }
    }
    ~ThreadPool()
    {
        stop();
    }
    template<typename F, typename... Args>
    void run(F&& f, Args&&... args) {
        auto task = [Func = std::forward<F>(f)] { return Func(); };

        {
            std::lock_guard lock(_mtx);
            _tasks.emplace(task);
        }

        _condVar.notify_one();
    }
    void wait()
    {
        for (auto& t : _threads) {
            if (t.joinable()) {
                t.join();
            }
        }
    }
    void stop()
    {
        if (_stop) {
            return;
        }
        _stop = true;
        _condVar.notify_all();
        wait();
        _threads.clear();
    }

private:
    std::queue<std::function<void()>> _tasks;
    std::atomic_bool _stop{ false };
    std::mutex _mtx;
    std::condition_variable _condVar;
    std::vector<std::thread> _threads;
};
// 删除给定路径的文件或文件夹
RemoveResult removeData(const fs::path& dataToRemove)
{
    std::string message;
    bool isOk = true;
    try {
        if (fs::exists(dataToRemove)) {
            if (fs::is_regular_file(dataToRemove) || fs::is_symlink(dataToRemove)) {
                // 删除文件
                isOk = fs::remove(dataToRemove);
                if (isOk) {
                    message = std::format("Removed file: {}", dataToRemove.string());
                }
            }
            else if (fs::is_directory(dataToRemove)) {
                // todo: 优化性能
                ThreadPool pool;
                //const auto removedCount = fs::remove_all(dataToRemove);
                for (const auto& p : fs::recursive_directory_iterator(dataToRemove)) {
                    pool.run([p] {
                        const fs::path& filePath = p.path();
                        if (fs::is_regular_file(filePath) || fs::is_symlink(filePath)) {
                            // 删除文件
                            try {
                                fs::remove(filePath);
                            }
                            catch (...) {
                                std::cerr << "Failed to remove " << filePath << '\n';
                            }
                        }
                    });

                }
                pool.stop();
                // 删除主目录
                try {
                    std::filesystem::remove_all(dataToRemove);
                    message = std::format("Removed directory: {}", dataToRemove.string());
                }
                catch (...) {
                    message = std::format("Error: Failed to remove {}", dataToRemove.string());
                }
            }
        }
        else {
            isOk = false;
            message = std::format("Error: Path does not exist: {}", dataToRemove.string());
        }
    }
    catch (fs::filesystem_error& e) {
        message = std::format("Error: {}", e.what());
        isOk = false;
    }
    return { isOk, message };
}

void syncWait(std::vector<std::future<RemoveResult>>& threads)
{
    for (auto& th : threads) {
        printResult(th.get());
    }
    threads.clear();
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printYellow("Usage:\n rayrm d:/test d:/test2 d:/test3.txt ...");
        return 1;
    }
    // 开始计时
    const auto start = std::chrono::high_resolution_clock::now();
    for (int i = 1; i < argc; ++i) {
        printResult(removeData(argv[i]));
    }
    // 结束计时
    const auto end = std::chrono::high_resolution_clock::now();
    // 计算耗时
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    printYellow(std::format("Time taken {}ms", duration.count()));
    return 0;
}
