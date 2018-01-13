#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <functional>
#include <tuple>
#include <sstream>

using namespace std;

// trim from start
static inline string &ltrim(string &s) {
  auto startpos = s.find_first_not_of(" \t");
  if (string::npos != startpos)
  {
    s = s.substr(startpos);
  }
  return s;
}

/*
inline unsigned int stoui(const string& s)
{
  unsigned long lresult = stoul(s, 0, 16);

  //Kiki
  unsigned int result = (unsigned int) lresult;

  return result;
}
*/

inline unsigned int stoui(const string& s)
{
  //Kiki use sscanf () because  Android Studio does not support stoul ()
  //unsigned long result = stoul(s, 0, 16);
  unsigned int result = 0;
  sscanf(s.c_str(), "%x", &result);

  return result;
}


class ParseTxt {

public:
  static vector<uint32_t> parseRTable(string fileName)
  {
    vector<uint32_t> v;

    ifstream infile(fileName);
    if (!infile.is_open())
    {
      return v;
    }
    
    string line;
    while (getline(infile, line))
    {
      line = ltrim(line);
      if (line == "")
        continue;

      v.push_back(stoui(line));
    }

    return v;
  }

public:
  static vector<tuple<string, uint16_t, vector<uint8_t>>> parseInit(string fileName)
  {
    vector<tuple<string, uint16_t, vector<uint8_t>>> v;

    ifstream infile(fileName);
    if (!infile.is_open())
    {
      return v;
    }

    string line;
    while (getline(infile, line))
    {
      line = ltrim(line);
      if (line == "")
        continue;

      auto pos = line.find_first_of("#");
      if (pos != std::string::npos)
        line = line.substr(0, pos);

      if (line == "")
        continue;
        
      //Kiki
      if ( line == "\r" || line == "\n" )
          continue;

      v.push_back(parseInitLine(line));
    }

    return v;
  }

private:
  static tuple<string, uint16_t, vector<uint8_t>> parseInitLine(string &line)
  {
    istringstream iss(line);

    vector<string> toks;
    do
    {
      string sub;
      iss >> sub;

      if (sub != "")
        toks.push_back(sub);
    } while (iss);

    auto cmd = toks[0];
    auto addr = static_cast<uint16_t>(stoui(toks[1]));

    vector<uint8_t> v;
    for (auto i = 2; i < toks.size(); ++i)
      v.push_back(static_cast<uint8_t>(stoui(toks[i])));
    
    return make_tuple(cmd, addr, v);
    
  }
};
