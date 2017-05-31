#include "Parser.h"
#include <cstdlib>
#include <iostream>

namespace cli {


Parser::Parser(const string &programName, const string &helpText, bool addHelpCmd)
        : programName(programName), helpText(helpText), positionalSpecs(), switchedSpecs(), useHelpCmd(addHelpCmd) {}

Parser &Parser::flag(const string &key, const string &alias, const string &helpText) {
    _checkUniqueName(key, alias);
    switchedSpecs.push_back({key, alias, true, "?", helpText});
    return *this;
}

Parser &Parser::param(const string &key, const string &alias, const string &spec, const string &helpText) {
    _checkUniqueName(key, alias);
    switchedSpecs.push_back({key, alias, false, spec, helpText});
    return *this;
}

Parser &Parser::positional(const string &key, const string &spec, const string &helpText) {
    ArgSpec argspec{key, "", false, spec, helpText};
    if (argspec.isList) {
        for (auto &argspec_ : positionalSpecs) {
            if (argspec_.isList)
                throw std::runtime_error(
                        "Unable to resolve many variadic arguments");
            if (!argspec_.required)
                throw std::runtime_error(
                        "Both variadic and optional arguments are not allowed");
        }
    } else if (argspec.required) {
        for (auto &argspec_ : positionalSpecs) {
            if (!argspec_.required || argspec_.isList)
                throw std::runtime_error(
                        "Required args after optionals are not allowed");
        }
    }
    positionalSpecs.push_back(argspec);
    return *this;
}

Arguments Parser::parse(int argc, char **argv_) const {
    std::vector<string> argv;
    for (int i = 1; i < argc; i++) {
        argv.push_back(string(argv_[i]));
    }
    if (argv.size() == 1 && (argv[0] == "-h" || argv[0] == "--help")) {
        printUsage();
        exit(0);
    }

    Arguments args({}, {}, {});
    for (const auto &spec : switchedSpecs) {
        bool found = true;
        while (found) {
            found = false;
            for (auto it = argv.cbegin(); it != argv.cend(); it++) {
                int nConsumed = spec.parse(args, it, argv.end());
                if (nConsumed > 0) {
                    found = true;
                    argv.erase(it, it + nConsumed);
                    break;
                }
            }
            if (!found) {
                if (spec.isList) {
                    if (!args.hasParamList(spec.key))
                        args.set(spec.key, std::vector<string>());
                } else {
                    if (spec.required) fail(spec.key, "required", true);
                    else args.set(spec.key, false);
                }
            }
            break;
        }
    }
    int variadicIdx = -1;
    for (int i = 0; i < positionalSpecs.size(); i++)
        if (positionalSpecs[i].isList) {
            variadicIdx = i;
            break;
        }
    if (variadicIdx >= 0) {
        int nVariadic = argv.size() - positionalSpecs.size() + 1;
        if (nVariadic < 0 || (nVariadic < 1 && positionalSpecs[variadicIdx].required)) {
            fail(positionalSpecs[variadicIdx].key, "required", true);
        }
        for (int i = 0; i < variadicIdx; i++)
            args.set(positionalSpecs[i].key, argv[i]);
        args.set(positionalSpecs[variadicIdx].key,
                 std::vector<string>(argv.begin() + variadicIdx,
                                     argv.begin() + variadicIdx + nVariadic));
        for (int i = variadicIdx + 1; i < positionalSpecs.size(); i++)
            args.set(positionalSpecs[i].key, argv[i + nVariadic]);
    } else {
        if (argv.size() > positionalSpecs.size()) {
            fail("Extra arguments", true);
        }
        int i = 0;
        for (; i < argv.size(); i++) {
            args.set(positionalSpecs[i].key, argv[i]);
        }
        if (i < positionalSpecs.size() && positionalSpecs[i].required) {
            fail(positionalSpecs[i].key, "required", true);
        }
    }

    return args;
}

void Parser::fail(const string &message, bool withUsage) const {
    if (message.length() > 0) std::cout << message << std::endl;
    if (withUsage) printUsage();
    exit(1);
}

void Parser::printUsage() const {
    using std::cout;
    using std::endl;
    if (helpText.length() > 0) cout << helpText << endl;
    cout << "Syntax: " << programName;
    for (const auto &spec : switchedSpecs) {
        cout << ' ';
        if (!spec.required) cout << "[";
        const string &switch_ = (spec.alias.length() > 0) ? spec.alias : ("--" + spec.key);
        cout << switch_;
        if (!spec.isFlag) cout << " <" << spec.key << ">";
        if (spec.isList) cout << "...";
        if (!spec.required) cout << "]";
    }
    for (const auto &spec : positionalSpecs) {
        cout << ' ';
        if (!spec.required) cout << "[";
        cout << "<" << spec.key << ">";
        if (spec.isList) cout << "...";
        if (!spec.required) cout << "]";
    }
    cout << endl;

    for (const auto &spec : positionalSpecs) {
        cout << "\t" << spec.key << "\t" << spec.helpText << endl;
    }
    for (const auto &spec : switchedSpecs) {
        cout << "\t--" << spec.key << "\t" << spec.helpText << endl;
    }
}

void Parser::fail(const string &argument, const string &message, bool withUsage) const {
    using std::cout;
    using std::endl;
    cout << "Error in argument \"" << argument << '\"';
    if (message.length() > 0) {
        cout << ": " << message;
    }
    cout << endl;
    fail("", withUsage);
}

void Parser::_checkUniqueName(const string &key, const string &alias) {
    for (const auto &argspec : switchedSpecs) {
        if (key == argspec.key || key == argspec.alias)
            throw std::runtime_error("Key " + key + " already defined");
        if (alias.length() > 0 && (alias == argspec.key || alias == argspec.alias))
            throw std::runtime_error("Alias " + alias + " already defined");
    }
}

Parser::ArgSpec::ArgSpec(const string &key, const string &alias, bool isFlag,
                         const string &spec, const string &helpText)
        : key(key), alias(alias), isFlag(isFlag), isList(spec == "*" || spec == "+"),
          required(spec == "" || spec == "+"), helpText(helpText) {
    if (spec != "" && spec != "*" && spec != "?" && spec != "+")
        throw std::runtime_error("Invalid spec");
}

int Parser::ArgSpec::parse(Arguments &args, string_iterator argvStart, string_iterator argvEnd) const {
    if (argvStart == argvEnd) return 0;
    if (!isFlag && argvStart + 1 == argvEnd) return 0;
    if (*argvStart != "--" + key && *argvStart != alias) return 0;
    if (isFlag) {
        args.set(key, true);
        return 1;
    } else {
        const string &value = *(argvStart + 1);
        if (isList) {
            if (args.hasParamList(key)) {
                const_cast<std::vector<string> &>(args.paramlist(key)).push_back(value);
            } else {
                args.set(key, std::vector<string>{value});
            }
        } else {
            args.set(key, value);
        }
        return 2;
    }
}

}