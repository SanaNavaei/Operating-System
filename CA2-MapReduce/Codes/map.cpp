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

#define GENRE_CSV "genres.csv"

void trimLeft(std::string &text)
{
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), [](unsigned char ch)
                                          { return !std::isspace(ch); }));
}

void save_in_file(int file_num, std::vector<std::vector<std::string>> &count_genres, std::vector<std::string> &genres)
{
    for (int i = 0; i < count_genres.size(); i++)
    {
        std::ofstream file;
        file.open(count_genres[i][0]);
        file << count_genres[i][1];
    }

    for (int i = 0; i < genres.size(); ++i)
    {
        bool genre_exists = false;
        for (int j = 0; j < count_genres.size(); ++j)
        {
            if (genres[i] == count_genres[j][0])
            {
                genre_exists = true;
                break;
            }
        }
        if (!genre_exists)
        {
            std::ofstream file;
            file.open(genres[i]);
            file << "0";
        }
    }
}

void read_from_files(std::string files, std::vector<std::vector<std::string>> &count_genres)
{
    std::ifstream csv_files(files);
    std::string line;
    while (getline(csv_files, line))
    {
        std::stringstream text(line);
        std::string word;
        while (getline(text, word, ','))
        {
            bool non_repeated = true;
            trimLeft(word);
            for (int j = 0; j < count_genres.size(); j++)
            {
                if (word == count_genres[j][0])
                {
                    int genre_counter = stoi(count_genres[j][1]) + 1;
                    count_genres[j][1] = std::to_string(genre_counter);
                    non_repeated = false;
                    break;
                }
            }
            if (non_repeated)
            {
                std::vector<std::string> vec; // save the genre with 1 value
                vec.push_back(word);
                vec.push_back("1");
                count_genres.push_back(vec);
            }
        }
    }
}

std::vector<std::string> manage_genre_file(std::string library)
{
    library = "./" + library + "/" + GENRE_CSV;

    std::vector<std::string> genres;
    std::ifstream csv_file(library);
    std::string line;
    while (getline(csv_file, line))
    {
        std::stringstream text(line);
        std::string word;
        while (getline(text, word, ','))
        {
            trimLeft(word);
            genres.push_back(word);
        }
    }
    return genres;
}

std::string manage_address_files(std::string library, int file_num)
{
    std::string num = std::to_string(file_num);
    library = "./" + library + "/part" + num + ".csv";
    return library;
}

int main(int argc, char **argv)
{
    int file_num;
    std::string library;
    std::cin >> file_num >> library;
    std::string files = manage_address_files(library, file_num);

    std::vector<std::vector<std::string>> count_genres;
    std::vector<std::string> genres = manage_genre_file(library);
    read_from_files(files, count_genres);
    save_in_file(file_num, count_genres, genres);
    return 0;
}