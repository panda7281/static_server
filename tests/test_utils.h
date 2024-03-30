#pragma once

#include <fstream>
#include <string>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

/**
 * @brief Create a test dir
 * 
 */
void create_test_dir()
{
    if (system("mkdir -p test_dir") != 0)
    {
        std::runtime_error("fail to make test dir");
    }
}

/**
 * @brief Create a test file inside test dir
 * 
 * @param size file size in bytes
 * @param name file name
 * @return std::string file path
 */
std::string create_test_file(off_t size, std::string name)
{
    std::string name_with_dir = "test_dir/" + name;
    // remove file if exist
    remove(name_with_dir.c_str());

    int fd = open(name_with_dir.c_str(), O_CREAT | O_WRONLY, 0644);
    if (fd == -1) {
        std::runtime_error("fail to create file");
    }

    if (ftruncate(fd, size) == -1) {
        std::runtime_error("fail to truncate file");
        close(fd);
    }
    close(fd);

    return name_with_dir;
}

/**
 * @brief remove all test files and the folder
 * 
 */
void remove_test_dir()
{
    if (system("rm -rf test_dir") != 0)
    {
        std::runtime_error("fail to remove test dir");
    }
}

/**
 * @brief check if the content of two files are identical
 * 
 * @param file1 
 * @param file2 
 * @return true 
 * @return false 
 */
bool compareFiles(const std::string& file1, const std::string& file2) {
    std::ifstream f1(file1, std::ios::binary | std::ios::ate);
    std::ifstream f2(file2, std::ios::binary | std::ios::ate);

    if (f1.fail() || f2.fail()) {
        return false;
    }

    if (f1.tellg() != f2.tellg()) {
        return false; // File sizes are different
    }

    f1.seekg(0);
    f2.seekg(0);

    std::vector<char> buf1(std::istreambuf_iterator<char>(f1), {});
    std::vector<char> buf2(std::istreambuf_iterator<char>(f2), {});

    return buf1 == buf2;
}