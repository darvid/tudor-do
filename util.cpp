#include <algorithm>
#include <cstdio>
#include <istream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

void find_and_replace(std::string& str,
                      const std::string& substr,
                      const std::string& repl)
{
    std::string::size_type pos;
    for (pos = str.find(substr);
         pos != std::string::npos;
         pos = str.find(substr, pos))
    {
        str.replace(pos, substr.length(), repl);
        pos += repl.length();
    }
}

int find_last_space_pos(std::string str)
{
    std::string::size_type pos;
    for (pos = str.find_last_of(' ');
         pos != std::string::npos;)
    {
        if (str.substr(pos - 1, 1) == "\\")
        {
            --pos;
            str = str.substr(0, pos);
            pos = str.find_last_of(' ');
        }
        break;
    }
    return pos;
}

bool exists(const std::string& path)
{
    struct stat st;
    return (stat(path.c_str(), &st) == 0 ? true : false);
}

void warning(const std::string& msg)
{
    fprintf(stderr, "warning: %s\n", msg.c_str());
}

void fatal_error(const std::string& msg)
{
    fprintf(stderr, "%s\n", msg.c_str());
    exit(1);
}

std::string lower(const std::string& str)
{
    std::string newstr(str);
    std::transform(newstr.begin(), newstr.end(), newstr.begin(),
        ::tolower);
    return newstr;
}

std::vector<std::string> split(const std::string& str, const char delim)
{
    std::vector<std::string> elems;
    std::stringstream ss(str);
    std::string item;
    while (getline(ss, item, delim))
        elems.push_back(item);
    return elems;
}

