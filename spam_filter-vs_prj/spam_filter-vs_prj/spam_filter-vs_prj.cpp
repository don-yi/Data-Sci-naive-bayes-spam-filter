// spam_filter-vs_prj.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <fstream>    // std::ifstream
#include <string>     // std::string
#include <sstream>    // std::istringstream
#include <iostream>   // std::cout
#include <vector>     // std::vector
#include <algorithm>  // std::find_if
#include <cctype>     // std::isalpha
#include <filesystem> // std::filesystem
namespace fs = std::filesystem;
#include "utf8.h"

// fwd ref
std::string GetSubjectLine(char const* filename);
void GetWords(std::string& line, std::vector<std::string>& words);

int main()
{
  // res
  std::vector<std::string> words;

  std::string path = "data/spam/train";
  for (const auto& entry : fs::directory_iterator(path))
  {
    fs::path entryPath = entry.path();
    std::string path_string{entryPath.u8string()};

    // get subject line w/ subject
    std::string line = GetSubjectLine(path_string.c_str());

    // get words from subject line
    GetWords(line, words);
  }

  for (auto& word : words)
  {
    std::cout << word << " ";
  }
  std::cout << std::endl;


  return 0;
}

std::string GetSubjectLine(char const* filename)
{
  std::ifstream ifs(filename, std::ifstream::in);

  std::string line;
  while (std::getline(ifs, line))
  {
    std::istringstream iss(line);

    // skip non-subject lines, stop at subject line
    if (line.compare(0, 9, "Subject: ")) continue;
    else break;
  }

  return line;
}

bool isalpha(uint32_t c) {
    return (c >= 0x0041 && c <= 0x005A)
        || (c >= 0x0061 && c <= 0x007A);
}

void GetWords(std::string& line, std::vector<std::string>& words)
{
  if (line.empty()) return;

  // substr w/o "Subject: "
  line = line.substr(9);
  // iss
  std::istringstream iss(line);
  // words from line
  std::string word;

  //struct non_alpha {
  //  bool operator()(char c) {
  //    return !std::isalpha(c);
  //  }
  //};

  while (iss >> word)
  {
    //bool contains_non_alpha
    //  = std::find_if(word.begin(), word.end(), non_alpha()) != word.end();
    bool contains_non_alpha
      = std::any_of(utf8::unchecked::iterator(word.begin()),
        utf8::unchecked::iterator(word.end()),
        [](uint32_t c) { return !isalpha(c); });


    // if not only letters
    if (contains_non_alpha)
    {
      // get only letters
      std::string letterWord;
      for (auto& c : word)
      {
        if (isalpha(static_cast<uint32_t>(c))) break;

        letterWord += c;
      }

      word = letterWord;
    }

    // skip empty word
    if (word.empty()) continue;

    // get lower-case word
    std::string lowerWord;
    for (auto& c : word)
    {
      lowerWord += std::tolower(c);
    }

    //std::cout << lowerWord << std::endl;
    words.push_back(lowerWord);
  }
}

