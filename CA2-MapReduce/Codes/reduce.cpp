#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define SIZE 5000

int read_files(char *genre_name, int files_count, std::vector<std::vector<std::string>> &files_data)
{
    int count = 0;
    for (int i = 0; i < files_count; i++)
    {
        std::ifstream file;
        file.open(genre_name);
        int num;
        file >> num;
        count += num;
    }
    return count;
}

int main(int argc, char **argv)
{
    std::vector<std::vector<std::string>> files_data;
    int result = read_files(argv[1], atoi(argv[2]), files_data);
    std::cout << argv[1] << ": " << result << '\n';
    return 0;
}