// spam_filter-vs_prj.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <fstream>        // std::ifstream
#include <string>         // std::string
#include <sstream>        // std::istringstream
#include <iostream>       // std::cout
#include <unordered_map>  // std::unordered_map
#include <algorithm>      // std::find_if
#include <cctype>         // std::isalpha
#include <filesystem>     // std::filesystem
#include <limits>         // std::numeric_limits
namespace fs = std::filesystem;
#include "utf8.h"

// fwd ref
std::string GetSubjectLine(char const* filename);
void GetWords(
  std::string& line, std::unordered_map<std::string, unsigned>& words
);
void CalcProbForWord(
  std::unordered_map<std::string, float>& probMap,
  std::unordered_map<std::string, unsigned>& spamWords,
  unsigned ct
);
void GetTop5Probs(
  std::unordered_map<std::string, float>& probMap,
  std::unordered_map<std::string, float>& top5Prob
);
void InitIsWordsInSubject(
  std::unordered_map<std::string, bool>& isWordsInSubject,
  std::unordered_map<std::string, float>& top5SpamProb,
  std::unordered_map<std::string, float>& top5HamProb
);
float& GetSpamProbGivenWords(
  std::unordered_map<std::string, unsigned>& words,
  std::unordered_map<std::string, float>& spamProbMap,
  std::unordered_map<std::string, float>& hamProbMap
);
float CalcPWords(
  std::unordered_map<std::string, float>& probMap,
  std::unordered_map<std::string, unsigned>& words
);
void TestSpamFilter(
  char const* testPath,
  unsigned& spamCt,
  unsigned& hamCt,
  std::unordered_map<std::string, float>& spamProbMap,
  std::unordered_map<std::string, float>& hamProbMap,
  unsigned& predSpamCt,
  unsigned& predHamCt
);
void GetWordsLs(
  std::string path,
  std::unordered_map<std::string, unsigned>& words,
  unsigned& ct
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
  GetWordsLs(spamPath, spamWords, spamCt);
  // get ls of ham words (ez)
  std::string ezHamPath = "data/easy_ham/train";
  GetWordsLs(ezHamPath, hamWords, hamCt);
  // get ls of ham words (hard)
  std::string hardHamPath = "data/hard_ham/train";
  GetWordsLs(hardHamPath, hamWords, hamCt);

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

  std::cout << "TOP 5 SPAM WORDS :" << std::endl;
  GetTop5Probs(spamProbMap, top5SpamProb);
  std::cout << "TOP 5 HAM WORDS :" << std::endl;
  GetTop5Probs(hamProbMap, top5HamProb);

  ////
  // Test the spam filter using the saved testing data.

  // res
  unsigned trueSpam = 0;
  unsigned falseSpam = 0;
  unsigned trueHam = 0;
  unsigned falseHam = 0;

  //std::cout << std::endl;

  TestSpamFilter(
    "data/spam/test",
    spamCt,
    hamCt,
    spamProbMap,
    hamProbMap,
    trueSpam,
    falseSpam
  );
  TestSpamFilter(
    "data/easy_ham/test",
    spamCt,
    hamCt,
    spamProbMap,
    hamProbMap,
    falseHam,
    trueHam
  );
  TestSpamFilter(
    "data/hard_ham/test",
    spamCt,
    hamCt,
    spamProbMap,
    hamProbMap,
    falseHam,
    trueHam
  );

  // total emails
  unsigned numEmail = trueSpam + falseSpam + trueHam + falseHam;
  unsigned numPredSpam = trueSpam + falseHam;
  unsigned numRealSpam = trueSpam + falseSpam;

  // output res rates
  std::cout << std::endl;
  std::cout << "ACCURACY RATE: "
    << static_cast<float>((trueSpam + trueHam)) / static_cast<float>(numEmail)
    << std::endl;
  std::cout << "PRECISION RATE: "
    << static_cast<float>(trueSpam) / static_cast<float>(numPredSpam)
    << std::endl;
  std::cout << "RECALL RATE: "
    << static_cast<float>(trueSpam) / static_cast<float>(numRealSpam)
    << std::endl;

  std::cout << std::endl;

  return 0;
}

void GetWordsLs(
  std::string path,
  std::unordered_map<std::string, unsigned>& words,
  unsigned& ct
) {
  for (const auto& entry : fs::directory_iterator(path))
  {
    fs::path entryPath = entry.path();
    std::string path_string{entryPath.u8string()};

    // get subject line w/ subject
    std::string line = GetSubjectLine(path_string.c_str());

    // get words from subject line
    GetWords(line, words);

    // up spam ct
    ++ct;
  }
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

void GetWords(
  std::string& line, std::unordered_map<std::string, unsigned>& words
) {
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
      = std::any_of(
        utf8::unchecked::iterator(word.begin()),
        utf8::unchecked::iterator(word.end()),
        [](uint32_t c) { return !isalpha(c); }
    );

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

    // get lower-case word
    std::string lowerWord;
    for (auto& c : word)
    {
      lowerWord += std::tolower(c);
    }

    // skip empty, meaningless, unlikely words
    if (
      lowerWord.empty()
      or lowerWord == "your"
      or lowerWord == "for"
      or lowerWord == "the"
      or lowerWord == "you"
      or lowerWord == "a"
      or lowerWord == "re"
      or lowerWord == "for"
      or lowerWord == "to"
      or lowerWord == "on"
      or lowerWord == "of"
      or lowerWord == "in"
      or lowerWord == "and"
      or lowerWord == "with"
      or lowerWord == "is"
      or lowerWord == "from"
      or lowerWord == "ouch"
      or lowerWord == "bliss"
      or lowerWord == "spamassassin"
      or lowerWord == "perl"
      ) continue;

    // up words map
    ++words[lowerWord];
  }
}

#define ALPHA 1
#define BETA 2
void CalcProbForWord(
  std::unordered_map<std::string, float>& probMap, 
  std::unordered_map<std::string, unsigned>& words, 
  unsigned ct
) {
  for (auto& i : words)
  {
    std::string word = i.first;
    unsigned wordCt = i.second;
    float spamProb 
      = static_cast<float>(ALPHA + wordCt) / static_cast<float>(BETA + ct);
    probMap[word] = spamProb;
  }
}

void GetTop5Probs(
  std::unordered_map<std::string, float>& probMap, 
  std::unordered_map<std::string, float>& top5Prob
) {
  std::unordered_map<std::string, float> mapType;
  using pair_type = decltype(mapType)::value_type;

  for (size_t i = 0; i < 5; i++) {
    auto pr = std::max_element(
      std::begin(probMap),
      std::end(probMap),
      [](const pair_type& p1, const pair_type& p2) {
        return p1.second < p2.second;
      }
    );

    std::cout << pr->first << ": " << pr->second << std::endl;

    top5Prob[pr->first] = pr->second;
    probMap.erase(pr->first);
  }

  // put top 5 back to the map
  for (auto& i : top5Prob) probMap[i.first] = i.second;

  std::cout << std::endl;
}

// (e) Set up the function
float& GetSpamProbGivenWords(
  std::unordered_map<std::string, unsigned>& words,
  float& pSpam,
  float& pHam,
  std::unordered_map<std::string, float>& spamProbMap,
  std::unordered_map<std::string, float>& hamProbMap
)
{
  float pWordsSpam = CalcPWords(spamProbMap, words);
  float pWordsHam = CalcPWords(hamProbMap, words);

  float res = (pWordsSpam * pSpam) / (pWordsSpam * pSpam + pWordsHam * pHam);
  return res;
}

void InitIsWordsInSubject(
  std::unordered_map<std::string, bool>& isWordsInSubject,
  std::unordered_map<std::string, float>& top5SpamProb,
  std::unordered_map<std::string, float>& top5HamProb
)
{
  for (auto& pr : top5SpamProb)
  {
    isWordsInSubject[pr.first] = false;
  }
  for (auto& pr : top5HamProb)
  {
    isWordsInSubject[pr.first] = false;
  }
}

//void IncludeEachOtherProb(
//  std::unordered_map<std::string, float>& top5Prob1,
//  std::unordered_map<std::string, float>& top5Prob2,
//  unsigned& ct2
//)
//{
//  // include each other in prob map
//  for (auto& i : top5Prob1) {
//    std::string word = i.first;
//
//    // skip if exist
//    if (top5Prob2.find(word) != top5Prob2.end()) {
//      continue;
//    }
//
//    top5Prob2[word] = static_cast<float>(ALPHA) / static_cast<float>(BETA + ct2);
//  }
//}

float CalcPWords(
  std::unordered_map<std::string, float>& probMap,
  std::unordered_map<std::string, unsigned>& words
) {
  float res = 1.0f;

  for (auto& i : probMap) {
    std::string word = i.first;
    float prob = i.second;

    // handle prob given word
    if (words.find(word) != words.end())
      res *= prob;
    else
      res *= 1.0f - prob;
  }

  return res;
}

void TestSpamFilter(
  char const* testPath,
  unsigned& spamCt,
  unsigned& hamCt,
  std::unordered_map<std::string, float>& spamProbMap,
  std::unordered_map<std::string, float>& hamProbMap,
  unsigned& predSpamCt,
  unsigned& predHamCt
) {
  for (const auto& entry : fs::directory_iterator(testPath)) {
    fs::path entryPath = entry.path();
    std::string path_string{entryPath.u8string()};

    // get subject line w/ subject
    std::string line = GetSubjectLine(path_string.c_str());

    // get words from subject line
    std::unordered_map<std::string, unsigned> words;
    GetWords(line, words);

    // pre-computes
    unsigned totalCt = spamCt + hamCt;
    float pSpam = static_cast<float>(spamCt) / static_cast<float>(totalCt);
    float pHam = static_cast<float>(hamCt) / static_cast<float>(totalCt);

    float spamP
      = GetSpamProbGivenWords(words, pSpam, pHam, spamProbMap, hamProbMap);
    //std::cout << spamP << std::endl;

    if      (spamP >= .3f) ++predSpamCt;
    else if (spamP <  .3f) ++predHamCt;
  }
}
