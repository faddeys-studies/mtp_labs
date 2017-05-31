#ifndef MTP_LAB1_PARSER_H
#define MTP_LAB1_PARSER_H

#include <string>
#include <vector>
#include "Arguments.h"

using std::string;

namespace cli {
class Parser {

public:
    Parser(const string &programName, const string &helpText, bool addHelpCmd = true);

    Parser &flag(const string &key, const string &alias = "", const string &helpText = "");

    Parser &param(const string &key, const string &alias = "", const string &spec = "", const string &helpText = "");

    Parser &positional(const string &key, const string &spec = "", const string &helpText = "");

    Arguments parse(int argc, char **argv) const;

    void fail(const string &message = "", bool withUsage = true) const;

    void fail(const string &argument, const string &message = "", bool withUsage = true) const;

    void printUsage() const;

private:

    class ArgSpec {
    public:

        const string key;
        const string alias;
        const string helpText;
        const bool required;
        const bool isList;
        const bool isFlag;

        ArgSpec(const string &key, const string &alias, bool isFlag,
                const string &spec, const string &helpText);

        typedef std::vector<string>::const_iterator string_iterator;

        int parse(Arguments &args, string_iterator argvStart, string_iterator argvEnd) const;

    };

    string programName;
    string helpText;
    bool useHelpCmd;
    std::vector<ArgSpec> positionalSpecs;
    std::vector<ArgSpec> switchedSpecs;

    void _checkUniqueName(const string &key, const string &alias);

};
}

#endif //MTP_LAB1_PARSER_H
