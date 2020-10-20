// spam_filter-vs_prj.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <fstream>        // std::ifstream
#include <string>         // std::string
#include <sstream>        // std::istringstream
#include <iostream>       // std::cout
//#include <vector>         // std::vector
#include <unordered_map>  // std::unordered_map
#include <algorithm>      // std::find_if
#include <cctype>         // std::isalpha
#include <filesystem>     // std::filesystem
namespace fs = std::filesystem;
#include "utf8.h"

// fwd ref
std::string GetSubjectLine(char const* filename);
void GetWords(std::string& line, std::unordered_map<std::string, unsigned>& words);
void CalcProbForWord(
  std::unordered_map<std::string, float>& probMap,
  std::unordered_map<std::string, unsigned>& spamWords,
  unsigned ct
);

int main()
{
  // res
  std::unordered_map<std::string, unsigned> spamWords;
  std::unordered_map<std::string, unsigned> hamWords;
  unsigned spamCt = 0;
  unsigned hamCt = 0;

  /*
  (a) Use the training data to make a list of words (made out of letters only) from the Subject
      line. Now you have a list of words to test against.
  */

  // get ls of spam words
  std::string spamPath = "data/spam/train";
  for (const auto& entry : fs::directory_iterator(spamPath))
  {
    fs::path entryPath = entry.path();
    std::string path_string{entryPath.u8string()};

    // get subject line w/ subject
    std::string line = GetSubjectLine(path_string.c_str());

    // get words from subject line
    GetWords(line, spamWords);

    // up spam ct
    ++spamCt;
  }

  // get ls of ham words (ez)
  std::string ezHamPath = "data/easy_ham/train";
  for (const auto& entry : fs::directory_iterator(ezHamPath))
  {
    fs::path entryPath = entry.path();
    std::string path_string{entryPath.u8string()};

    // get subject line w/ subject
    std::string line = GetSubjectLine(path_string.c_str());

    // get words from subject line
    GetWords(line, hamWords);

    // up spam ct
    ++hamCt;
  }

  // get ls of ham words (hard)
  std::string hardHamPath = "data/hard_ham/train";
  for (const auto& entry : fs::directory_iterator(hardHamPath))
  {
    fs::path entryPath = entry.path();
    std::string path_string{entryPath.u8string()};

    // get subject line w/ subject
    std::string line = GetSubjectLine(path_string.c_str());

    // get words from subject line
    GetWords(line, hamWords);

    // up spam ct
    ++hamCt;
  }

  ////debug
  //for (auto& word : hamWords)
  //{
  //  std::cout << word.first << ": " << word.second << std::endl;
  //}
  //std::cout << std::endl;

  /*
  (b) For each wk, compute the probabilities P(wk | spam) and P(wk | ham): Use ес = 1, ет = 2
      for smoothing
  */

  // res
  std::unordered_map<std::string, float> spamProbMap;
  std::unordered_map<std::string, float> hamProbMap;

  // itr each word ls map and calc P(wk | spam/ham)
  CalcProbForWord(spamProbMap, spamWords, spamCt);
  CalcProbForWord(hamProbMap, hamWords, hamCt);

  ////debug
  //for (auto& i : spamProbMap)
  //{
  //  std::cout << i.first << ": " << i.second << std::endl;
  //}
  //std::cout << std::endl;

  /*
  (c) Output the list of 5 words with highest probabilities P(spam | wk)
  (d) Output the list of 5 words with highest probabilities P(ham | wk)
  */

  // res
  std::unordered_map<std::string, float> top5SpamProb;
  std::unordered_map<std::string, float> top5HamProb;



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

// https://stackoverflow.com/questions/7616867/how-to-test-a-string-for-letters-only/7616973
bool isalpha(uint32_t c) {
    return (c >= 0x0041 && c <= 0x005A)
        || (c >= 0x0061 && c <= 0x007A);
}

void GetWords(std::string& line, std::unordered_map<std::string, unsigned>& words)
{
  // skip if no line
  if (line.empty()) return;

  // substr w/o "Subject: "
  line = line.substr(9);
  // iss
  std::istringstream iss(line);
  // words from line
  std::string word;

  // https://stackoverflow.com/questions/7616867/how-to-test-a-string-for-letters-only/7616973
  //struct non_alpha {
  //  bool operator()(char c) {
  //    return !std::isalpha(c);
  //  }
  //};

  while (iss >> word)
  {
    // https://stackoverflow.com/questions/7616867/how-to-test-a-string-for-letters-only/7616973
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
        // stop if not a letter
        if (not isalpha(static_cast<uint32_t>(c))) break;

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

    ++words[lowerWord];
  }
}

#define ALPHA 1
#define BETA 2
void CalcProbForWord(std::unordered_map<std::string, float>& probMap, std::unordered_map<std::string, unsigned>& spamWords, unsigned ct)
{
  for (auto& i : spamWords)
  {
    std::string word = i.first;
    float spamProb = static_cast<float>(ALPHA + spamWords[word]) / static_cast<float>(BETA + ct);
    probMap[word] = spamProb;
  }
}

