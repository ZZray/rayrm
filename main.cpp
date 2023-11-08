/**
 * @author: rayzhang
 * printf 在输出时通常是原子操作。不过，在多线程环境中，仍然可能需要使用互斥锁来保护共享数据或避免其他类型的竞争条件。
 * std::filesystem::remove_all 不是线程安全的，如果要删除的路径之间存在依赖关系，这可能会导致问题
 */
#include <filesystem>
#include <vector>
#include <string>
#include <thread>

namespace fs = std::filesystem;

// 用于在控制台打印红色文本的函数
void printRed(const std::string& message) {
    printf("\033[31m%s\033[0m\n", message.c_str());
}

// 删除给定路径的文件或文件夹
void deletePath(const fs::path& pathToDelete) {
    try {
        if (fs::exists(pathToDelete)) {
            if (fs::is_regular_file(pathToDelete)) {
                // 删除文件
                fs::remove(pathToDelete);
                printf("Removed file: %ls\n", pathToDelete.c_str());
            } else if (fs::is_directory(pathToDelete)) {
                // 递归删除目录
                const auto removed_count = fs::remove_all(pathToDelete);
                printf("Removed %zu item(s) from directory: %ls\n", removed_count, pathToDelete.c_str());
            }
        } else {
            printRed("Error: Path does not exist: " + pathToDelete.string());
        }
    } catch (fs::filesystem_error& e) {
        printRed("Error: " + std::string(e.what()));
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::thread> threads;

    // 解析命令行参数并创建线程
    for (int i = 1; i < argc; ++i) {
        threads.emplace_back(deletePath, fs::path(argv[i]));
    }

    // 等待所有线程完成
    for (auto& th : threads) {
        th.join();
    }

    return 0;
}
