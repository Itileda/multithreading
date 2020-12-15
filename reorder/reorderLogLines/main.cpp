#include <string>
#include <string_view>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <utility>

std::set<std::string> logLevel{"OFF",  "FATAL", "ERROR", "WARN",
                               "INFO", "DEBUG", "TRACE", "ALL"};

auto readFileToString = [](std::string& file_name) {
  std::string out, line;
  std::ifstream infile(file_name, std::ios::in);
  while (getline(infile, line)) {
    out += (line + "\n");
  }
  infile.close();
  return out;
};

// string timestamp to pair of {position, length)
using LogIndex = std::map<std::string, std::vector<std::pair<size_t, size_t>>>;

auto parseLine = [](std::string_view& line, LogIndex& indexes,
                    std::string& last_token, size_t start, size_t end) {
  std::string init_str(line);
  std::stringstream iss(init_str);
  std::string token;
  iss >> token;

  if (logLevel.count(token) != 0) {
    for (int i = 0; i < 4; ++i) {
      iss >> token;
    }

    token.erase(token.begin() + 12, token.end());
    auto it = std::remove_if(token.begin(), token.end(), [](char c) {
      if (c == ':' || c == ',') {
        return true;
      }
      return false;
    });
    token.erase(it, token.end());

    if (indexes.count(token) == 0) {
      indexes[token] = {std::make_pair(start, end - start)};
    } else {
      indexes[token].push_back(std::make_pair(start, end - start));
    }
    last_token = token;
  } else {
    indexes[last_token].push_back(std::make_pair(start, end - start));
  }
};

auto parse = [](LogIndex& indexes, std::string& log) {
  std::string_view log_view = std::string_view{log};
  std::string_view line;
  size_t end_line_pos = 0;
  size_t start_line_pos = 0;

  std::string last_token = "0";

  while (end_line_pos != log_view.npos) {
    end_line_pos = log_view.find('\n', start_line_pos);
    line = log_view.substr(start_line_pos, end_line_pos - start_line_pos);
    if (end_line_pos != log_view.npos) {
      parseLine(line, indexes, last_token, start_line_pos, end_line_pos);
    }
    start_line_pos = end_line_pos + 1;
  }

  std::cout << "log_view from parse size: " << log_view.size() << std::endl;
  std::cout << "log_view: " << &log_view << std::endl;

  return log_view;
};

auto write =
    [](std::string file_name, LogIndex& indexes, std::string_view& log_view) {
      std::cout << "write() enter\n";
      std::cout << "log_view: " << log_view.size() << std::endl;
      std::cout << "log_view: " << &log_view << std::endl;
      std::ofstream of;
      of.open(file_name);
      std::ostringstream log;
      log << " \n";
      for (auto i = indexes.begin(); i != indexes.end(); ++i) {
        for (auto index = (i->second).begin(); index != (i->second).end();
             ++index) {
          log << log_view.substr(index->first, index->second) << "\n";
        }
      }

      of << log.str();
      of.close();
    };

int main(int argc, char const* argv[]) {
  /* code */

  // std::string file_name = "3686.smartdevicelink.log";
  if (argc < 2) {
    std::cout << "provide log file name\n";
  }
  std::string file_name = std::string(argv[1]);
  LogIndex data;
  std::string log = readFileToString(file_name);
  std::string_view log_view = parse(data, log);
  std::string out_file = "out.log";
  write(out_file, data, log_view);
  return 0;
}
