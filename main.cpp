#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>

namespace fs = std::filesystem;

// 用于在控制台打印红色文本的函数
void printRed(const std::string& message) {
    std::cout << "\033[31m" << message << "\033[0m" << std::endl;
}

// 删除给定路径的文件或文件夹
void deletePath(const fs::path& pathToDelete) {
    try {
        if (fs::exists(pathToDelete)) {
            // 递归删除文件或文件夹
            const auto removed_count = fs::remove_all(pathToDelete);
            std::cout << "Removed " << removed_count << " item(s) from: " << pathToDelete << std::endl;
        }
        else {
            printRed("Error: Path does not exist: " + pathToDelete.string());
        }
    }
    catch (fs::filesystem_error& e) {
        printRed("Error: " + std::string(e.what()));
    }
}

int main(int argc, char* argv[]) {

    // 解析命令行参数
    std::vector<std::string> paths(argc - 1);
    for (int i = 1; i < argc; ++i) {
        paths.push_back(argv[i]);
    }

    // 删除指定的路径
    for (const auto& pathString : paths) {
        deletePath(fs::path(pathString));
    }

    return 0;
}
