#include <iostream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <dirent.h>
#include <regex>
#include <fstream>
#include <sys/stat.h>
#include <sys/wait.h>

#define READ 0
#define WRITE 1
#define MAX_FILE_SIZE 100
#define SIZE 5000
#define MAP_FILE "./map.out"
#define REDUCE_FILE "./reduce.out"
#define OUTPUT_FILE "output.csv"
#define GENRE_CSV "genres.csv"

int extract_num(std::string text)
{
    std::string output = std::regex_replace(text, std::regex("[^0-9]*([0-9]+).*"), std::string("$1"));
    return stoi(output);
}

void trimLeft(std::string &text)
{
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), [](unsigned char ch)
                                          { return !std::isspace(ch); }));
}

void mapping(std::vector<std::string> &files, char *library)
{
    for (int i = 0; i < files.size(); i++)
    {
        sleep(1);
        int fd_pipe[2];
        if (pipe(fd_pipe) == -1)
        {
            write(1, "An error occured while opening the pipe\n", 40);
            exit(1);
        }

        pid_t pid_files = fork(); // fork for per file.csv
        if (pid_files < 0)        // error on fork
        {
            write(1, "An error occured on fork\n", 25);
            exit(1);
        }
        else if (pid_files == 0) // child
        {
            close(fd_pipe[WRITE]);
            close(0); // STDIN
            dup(fd_pipe[READ]);
            close(fd_pipe[READ]);

            char addr[] = MAP_FILE;
            char *args[] = {&addr[0], NULL};
            execv(args[0], args);
        }
        else // parent
        {
            char str[SIZE];
            close(fd_pipe[READ]);
            int file_num = extract_num(files[i]); // find the number of files using regex
            sprintf(str, "%d %s", file_num, library);
            write(fd_pipe[WRITE], str, strlen(str));
            close(fd_pipe[WRITE]);
        }
    }
}

void reduce(std::vector<std::string> &files, std::vector<std::string> &genre_file)
{
    for (int i = 0; i < genre_file.size(); i++)
    {
        unlink(genre_file[i].c_str());
        mkfifo(genre_file[i].c_str(), 0666);
        pid_t pid_files = fork(); // fork for per genre file
        if (pid_files < 0)        // error on fork
        {
            write(1, "An error occured on fork\n", 25);
            exit(1);
        }
        else if (pid_files == 0) // child
        {
            char addr[] = REDUCE_FILE;
            std::string temp = std::to_string(files.size());
            char *args[] = {&addr[0], genre_file[i].data(), temp.data(), NULL};
            execv(args[0], args);
        }
    }
}

void handling_process(std::vector<std::string> &files, char *library, std::vector<std::string> &genre_file, int pid, int output)
{
    reduce(files, genre_file);
    mapping(files, library);
    
    while((pid = wait(&output)) > 0) {} //wait for all chilren to exit
}

std::vector<std::string> manage_genre_file(char *lib)
{
    std::string library = lib;
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

std::vector<std::string> get_csv_files(const char *address)
{
    DIR *d = opendir(address);
    struct dirent *entry;
    std::vector<std::string> all_files;
    if (d)
    {
        while ((entry = readdir(d)) != NULL)
        {
            char *file_names = entry->d_name;
            int size = strlen(entry->d_name) - 1;
            if (file_names[0] == 'p' && file_names[1] == 'a' && file_names[2] == 'r' && file_names[3] == 't' && file_names[size - 3] == '.' && file_names[size - 2] == 'c' && file_names[size - 1] == 's' && file_names[size] == 'v')
                all_files.push_back(entry->d_name);
        }
        closedir(d);
    }
    else
    {
        write(1, "There is no such directory!\n", 28);
        exit(1);
    }
    return all_files;
}

const char *get_input(int argc, char **argv)
{
    if (argc != 2)
    {
        write(1, "Wrong. You should enter the directory of library.\n", 50);
        exit(1);
    }
    std::string address = "./";
    address += argv[1];
    const char *char_address = address.c_str();
    return char_address;
}

int main(int argc, char **argv)
{
    const char *char_address = get_input(argc, argv); // get the directory
    std::vector<std::string> files = get_csv_files(char_address);  // get all files in a given directory
    std::vector<std::string> genre_file = manage_genre_file(argv[1]); //get genres in genres.csv
    int pid, output;
    handling_process(files, argv[1], genre_file, pid, output); //map and reduce
    return 0;
}
