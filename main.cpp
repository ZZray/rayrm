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
                const auto removed_count = fs::remove_all(dataToRemove);
                message = std::format("Removed {} item(s) from directory: {}", removed_count, dataToRemove.string());
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
    // 如果输入的只有一个路径
    if (argc == 2 && fs::is_directory(argv[1])) {
        printResult(removeData(argv[1]));
    }
    else {
        std::vector<std::future<RemoveResult>> threads;
        // 解析命令行参数并创建线程
        for (int i = 1; i < argc; ++i) {
            threads.emplace_back(std::async(std::launch::async | std::launch::deferred, removeData, fs::path(argv[i])));
            if (threads.size() > std::thread::hardware_concurrency()) {
                syncWait(threads);
            }
        }
        // 等待所有线程完成
        syncWait(threads);
    }
    // 结束计时
    const auto end = std::chrono::high_resolution_clock::now();
    // 计算耗时
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    printYellow(std::format("Time taken {}ms", duration.count()));
    return 0;
}
