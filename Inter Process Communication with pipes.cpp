#include <bits/stdc++.h>
#include <iostream>
#include <fstream>
#include <unistd.h>   // Required for fork(), getpid(), getppid()
#include <sys/wait.h> // Required for wait()
#include <dirent.h>
#include <cstring>
using namespace std;

void create_files(map<string, string> &received_map, string &path)
{
    for (auto &x : received_map)
    {
        string full_path = path + "/" + x.first; // build full_path
        ofstream file;
        file.open(full_path);

        if (!file.is_open())
        {
            cout << "\n Error in creating file!" << endl;

            // child exits
            exit(1);
        }
        file << x.second;

        // Close the file to free up resources.
        file.close();
    }
}

// Assumptions1 : We are assuming that the file content doesn't contain "|" or "\n", because if it had been there then it could have broke the parsing

// Assumptions2 : We are also assuming that there is no empty line in the file content

string map_to_string(map<string, string> &mp)
{
    // filename.txt|content

    string string_data = "";

    for (auto &x : mp)
    {
        string_data += x.first + "|" + x.second + "\n";
    }

    return string_data;
}

map<string, string> string_to_map(string &str)
{
    map<string, string> mp;

    int n = str.length();

    for (int i = 0; i < n; i++)
    {
        string s1, s2;

        while (i < n && str[i] != '|')
        {
            s1 += str[i];
            i++;
        }
        if (i < n)
            i++;
        while (i < n && str[i] != '\n')
        {
            s2 += str[i];
            i++;
        }

        mp[s1] = s2;
    }

    return mp;
}

map<string, string> read_directory(bool &err, string path, int child_num)
{
    // string path = "/home/anushree-saha/system assign/d1";
    map<string, string> mp;

    DIR *dir = opendir(path.c_str()); // to convert from string to const char *
    if (dir == NULL)
    {
        cout << "\n Cannot open";
        exit(1);
    }
    else
    {
        cout << "\n opened successfully child" << child_num << endl;
    }

    struct dirent *dp;

    while ((dp = readdir(dir)) != NULL)
    {
        string filename = dp->d_name; // get filename
        if (filename == "." || filename == "..")
        { // skip "." and ".."
            continue;
        }
        string full_path = path + "/" + filename; // build full_path

        // open file
        ifstream inputFile(full_path);

        string line;

        if (inputFile.is_open())
        {
            string content = "";
            // Read file line by line using getline
            while (getline(inputFile, line))
            {
                content += line; // store in map
            }

            mp[filename] = content;
            inputFile.close(); // Close the file
        }
        else
        {
            // cout << "Unable to open file for reading." << endl;

            err = true;
            break;
        }
    }

    closedir(dir);

    return mp;
}

int main()
{
    int ipc_pipes1[2];
    pipe(ipc_pipes1);

    int ipc_pipes2[2];
    pipe(ipc_pipes2);

    pid_t pid1, pid2;

    pid1 = fork();
    map<string, string> mp1, mp2;

    if (pid1 < 0)
    {
        perror("fork error for child 1");
        exit(1);
    }

    if (pid1 == 0)
    {
        // Code for the first child process
        printf("\n First child process (PID: %d, Parent PID: %d)\n", getpid(), getppid());
        // Do child 1 specific work here

        string path = "path of directory d1";

        // read directory
        bool err = false;
        mp1 = read_directory(err, path, 1);

        // Pipes cannot send: map<string,string> so,serialize the map into a string
        // serialize to string

        string data = "";
        if (!err)
            data += map_to_string(mp1);
        else
        {
            data += "ERROR\n" + map_to_string(mp1);
        }

        // write
        close(ipc_pipes2[1]);
        char buffer1[data.length() + 1];
        // convert string to char array
        strcpy(buffer1, data.c_str());

        int len = data.length();

        write(ipc_pipes1[1], buffer1, len);
        close(ipc_pipes1[1]); // not writing

        // read
        close(ipc_pipes1[0]);
        // assuming strings are short as per the given instruction
        char buffer2[4096];
        ssize_t rstr = read(ipc_pipes2[0], buffer2, sizeof(buffer2) - 1);

        if (rstr > 0)
        {
            buffer2[rstr] = '\0';

            string received_str(buffer2);

            // deserialize to map

            string str_err = "ERROR\n";
            if (received_str.compare(0, str_err.length(), str_err) == 0)
            {
                cout << "\n Error received from other process";
                received_str.erase(0, str_err.length());
            }
            map<string, string> received_map = string_to_map(received_str);

            // create files and write the contents
            create_files(received_map, path);
        }
        else if (rstr == 0)
        {
            // EOF: other process closed the pipe
            cout << "\n Pipe closed by the other process (EOF)\n";
        }
        else if (rstr == -1)
        {
            // Error occurred during read
            perror("read error");
            exit(1);
        }

        close(ipc_pipes2[0]); // not reading

        exit(0); // Child terminates after its work
    }
    else
    {
        // Code for the original parent process
        // Second fork: creates the second child process
        pid2 = fork();

        if (pid2 < 0)
        {
            perror("fork error for child 2");
            exit(1);
        }

        if (pid2 == 0)
        {
            // Code for the second child process
            printf("\n Second child process (PID: %d, Parent PID: %d)\n", getpid(), getppid());
            // Do child 2 specific work here

            string path = "path of directory d2";
            bool err = false;
            mp2 = read_directory(err, path, 2);

            // Pipes cannot send: map<string,string> so,serialize the map into a string
            string data = "";
            if (!err)
                data += map_to_string(mp2);
            else
            {
                data += "ERROR\n" + map_to_string(mp2);
            }

            // read
            close(ipc_pipes2[0]);
            char buffer1[4096];
            ssize_t rstr = read(ipc_pipes1[0], buffer1, sizeof(buffer1) - 1);

            if (rstr > 0)
            {
                buffer1[rstr] = '\0';

                string received_str(buffer1);

                // deserialize to map

                string str_err = "ERROR\n";
                if (received_str.compare(0, str_err.length(), str_err) == 0)
                {
                    cout << "\n Error received from other process";
                    received_str.erase(0, str_err.length());
                }
                map<string, string> received_map = string_to_map(received_str);

                // create files and write the contents
                create_files(received_map, path);
            }
            else if (rstr == 0)
            {
                // EOF: other process closed the pipe
                cout << "\n Pipe closed by the other process (EOF)\n";
            }
            else if (rstr == -1)
            {
                // Error occurred during read
                perror("read error");
                exit(1);
            }

            close(ipc_pipes1[0]);

            // write
            close(ipc_pipes1[1]);
            char buffer2[data.length() + 1];
            strcpy(buffer2, data.c_str());
            int len = data.length();
            write(ipc_pipes2[1], buffer2, len);
            close(ipc_pipes2[1]);

            exit(0); // Child terminates after its work
        }
        else
        {
            // Code for the original parent process continues

            close(ipc_pipes1[0]);
            close(ipc_pipes1[1]);
            close(ipc_pipes2[0]);
            close(ipc_pipes2[1]);
            // Wait for both children to finish
            printf("\n Parent waiting for children to finish...\n");
            waitpid(pid1, NULL, 0); // Wait for the first child
            waitpid(pid2, NULL, 0); // Wait for the second child
            printf("\n Both children finished. Parent process exiting.\n");
        }
    }

    return 0;
}
