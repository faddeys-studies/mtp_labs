#ifndef MTP_LAB1_ARGUMENTS_H
#define MTP_LAB1_ARGUMENTS_H

#include <map>
#include <string>
#include <vector>

using std::string;
using std::map;
using std::vector;

namespace cli {

class Arguments {
public:

    Arguments(const map<string, bool> &flags,
              const map<string, string> &params,
              const map<string, vector<string>> &paramlists)
            : _flags(flags), _params(params), _paramlists(paramlists) {};

    void set(const string &key, bool flag) { _flags[key] = flag; }

    void set(const string &key, const string &param) { _params[key] = param; }

    void set(const string &key, const vector<string> &paramlist) { _paramlists[key] = paramlist; }

    bool flag(const string &key) const { return _flags.at(key); }

    const string &param(const string &key) const { return _params.at(key); }

    const vector<string> &paramlist(const string &key) const { return _paramlists.at(key); }

    bool hasFlag(const string &key) const { return _flags.count(key) > 0; }

    bool hasParam(const string &key) const { return _params.count(key) > 0; }

    bool hasParamList(const string &key) const { return _paramlists.count(key) > 0; }

private:
    map<string, bool> _flags;
    map<string, string> _params;
    map<string, vector<string>> _paramlists;

};
}

#endif //MTP_LAB1_ARGUMENTS_H
