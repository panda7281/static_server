#pragma once

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
    system("mkdir -p test_dir");
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
    system("rm -rf test_dir");
}