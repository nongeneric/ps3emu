#include "ps3tool.h"

#include <fstream>
#include <map>
#include <string>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

using namespace boost::filesystem;

void HandleSplitLog(SplitLogCommand const& command) {
    create_directories(command.output);
    std::fstream f(command.log);
    if (!f.is_open()) {
        std::cout << "can't read file " << command.log << "\n";
        return;
    }

    std::map<std::string, std::ofstream> files;
    std::string line;
    boost::regex rx(" \\[(.*?)\\] ");
    while (std::getline(f, line)) {
        boost::smatch m;
        std::string name = "general";
        if (regex_search(line, m, rx)) {
            name = m.str(1);
        }
        auto it = files.find(name);
        if (it == end(files)) {
            std::ofstream os((path(command.output) / name).string());
            files[name] = std::move(os);
        }
        auto& os = files[name];
        os << line << "\n";
    }
}
