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

// 统计给定路径下的文件数量
std::size_t countFilesInDirectory(const fs::path& path)
{
    std::size_t fileCount = 0;

    // 使用递归目录迭代器遍历所有文件和子目录
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (fs::is_regular_file(entry)) {
            ++fileCount; // 如果是文件，则增加计数
        }
    }

    return fileCount;
}

using RemoveResult = std::pair<bool, std::string>;
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
                // 递归删除目录
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
        const auto& [isOk, message] = th.get();
        if (isOk) {
            std::cout << message << std::endl;
        }
        else {
            printRed(message);
        }
    }
    threads.clear();
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        printRed("Usage: rayrm d:/test d:/test2 d:/test3.txt ...");
        return 1;
    }
    const auto maxThreadCount = std::thread::hardware_concurrency() * 2;
    // 开始计时
    const auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::future<RemoveResult>> threads;
    // 如果输入的只有一个路径
    if (argc == 1 && fs::is_directory(argv[1])) {
        //const auto directoryPath = fs::path(argv[1]);
        //const std::size_t fileCount = countFilesInDirectory(directoryPath);
        //std::cout << "The directory contains " << fileCount << " file(s)." << std::endl;

        // 如果是目录，为每个子路径创建线程
        //for (const auto& entry : fs::recursive_directory_iterator(argv[1])) {
        for (const auto& entry : fs::directory_iterator(argv[1])) {
            threads.emplace_back(std::async(std::launch::async | std::launch::deferred, removeData, entry.path()));
            if (threads.size() >= maxThreadCount) {
                syncWait(threads);
            }
        }
    }
    else {
        // 解析命令行参数并创建线程
        for (int i = 1; i < argc; ++i) {
            threads.emplace_back(std::async(std::launch::async | std::launch::deferred, removeData, fs::path(argv[i])));
        }
    }
    // 等待所有线程完成
    syncWait(threads);
    // 结束计时
    const auto end = std::chrono::high_resolution_clock::now();
    // 计算耗时
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // 打印耗时
    std::cout << "Time taken: " << duration.count() << " milliseconds\n";
    return 0;
}
