#ifndef TUDOR_DO_UTIL_H
#define TUDOR_DO_UTIL_H
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>

void find_and_replace(std::string& str,
                      const std::string& substr,
                      const std::string& repl);
int find_last_space_pos(std::string str);
std::string lower(const std::string& str);
bool exists(const std::string& path);
void warning(const std::string& msg);
void fatal_error(const std::string& msg);
std::vector<std::string> split(const std::string& str, char delim);

#endif /* TUDOR_DO_UTIL_H */

