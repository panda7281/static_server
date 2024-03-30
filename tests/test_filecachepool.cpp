#include <catch2/catch_all.hpp>
#include "filecachepool.h"
#include "test_utils.h"

TEST_CASE("File Cache Pool", "[basic]")
{
    // create a 100KB pool
    FileCachePool pool(102400, 5);
    create_test_dir();
    
    std::vector<std::string> file_names = { "test1", "test2", "test3", "test4", "test5" };
    std::vector<off_t> file_size = { 1024, 2048, 512, 256, 768 };
    std::vector<std::string> file_paths(file_names.size());
    off_t curr = 0;
    for (int i = 0; i < file_names.size(); i++)
    {
        file_paths[i] = create_test_file(file_size[i], file_names[i]);
        pool.getFile(file_paths[i]);
        curr += file_size[i];
        REQUIRE(pool.getCurrentItemCount() == i+1);
        REQUIRE(pool.getCurrentSize() == curr);
    }

    remove_test_dir();
}

TEST_CASE("File Cache Pool", "[invalid file]")
{
    // create a 100KB pool
    FileCachePool pool(102400, 5);
    create_test_dir();
    // access a directory
    auto ptr = pool.getFile("test_dir");
    REQUIRE(!ptr);
    // access a invalid file
    ptr = pool.getFile("some_random_file");
    REQUIRE(!ptr);
    remove_test_dir();
}

TEST_CASE("File Cache Pool", "[evict by size]")
{
    // create a 3KB pool
    FileCachePool pool(3070, 5);
    create_test_dir();
    
    std::vector<std::string> file_names = { "test1", "test2", "test3", "test4", "test5" };
    std::vector<off_t> file_size = { 1024, 2048, 512, 256, 768 };
    std::vector<std::string> file_paths(file_names.size());

    std::vector<off_t> file_size_true = { 1024, 2048, 2560, 2816, 1536 };
    std::vector<int> file_cnt_true = { 1, 1, 2, 3, 3 };
    for (int i = 0; i < file_names.size(); i++)
    {
        file_paths[i] = create_test_file(file_size[i], file_names[i]);
        pool.getFile(file_paths[i]);
        REQUIRE(pool.getCurrentItemCount() == file_cnt_true[i]);
        REQUIRE(pool.getCurrentSize() == file_size_true[i]);
    }

    remove_test_dir();
}

TEST_CASE("File Cache Pool", "[evict by count]")
{
    // create a 100KB pool
    FileCachePool pool(102400, 3);
    create_test_dir();
    
    std::vector<std::string> file_names = { "test1", "test2", "test3", "test4", "test5" };
    std::vector<off_t> file_size = { 1024, 2048, 512, 256, 768 };
    std::vector<std::string> file_paths(file_names.size());

    std::vector<off_t> file_size_true = { 1024, 2048, 2560, 2816, 1536 };
    std::vector<int> file_cnt_true = { 1, 1, 2, 3, 3 };
    for (int i = 0; i < file_names.size(); i++)
    {
        file_paths[i] = create_test_file(file_size[i], file_names[i]);
        pool.getFile(file_paths[i]);
        REQUIRE(pool.getCurrentItemCount() <= 3);
    }

    remove_test_dir();
}

TEST_CASE("File Cache Pool", "[consistency]")
{
    // create a 100KB pool
    FileCachePool pool(102400, 10);
    create_test_dir();
    
    std::vector<std::string> file_names = { "test1", "test2", "test3", "test4", "test5" };
    std::vector<off_t> file_size = { 1024, 2048, 512, 256, 768 };
    std::vector<std::string> file_paths(file_names.size());

    for (int i = 0; i < file_names.size(); i++)
    {
        file_paths[i] = create_test_file(file_size[i], file_names[i]);
        auto file = pool.getFile(file_paths[i]);
        REQUIRE(file->getStat()->st_size == file_size[i]);
    }

    // mod files
    std::reverse(file_size.begin(), file_size.end());
    for (int i = 0; i < file_names.size(); i++)
    {
        create_test_file(file_size[i], file_names[i]);
        auto file = pool.getFile(file_paths[i]);
        REQUIRE(file->getStat()->st_size == file_size[i]);
    }

    remove_test_dir();
}