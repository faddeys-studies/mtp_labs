#include <iostream>

#include "Parser.h"


int main(int argc, char **argv) {
    cli::Parser parser{"cli_arg_demo", "A help text for my program"};
    parser.flag("flag1", "-f1", "Flag #1")
            .flag("flag2", "-f2", "Flag #2")
            .param("a-param")
            .param("list", "-l", "*")
            .positional("names", "+");
    auto args = parser.parse(argc, argv);
    std::cout << "flag1: " << args.flag("flag1") << std::endl;
    std::cout << "flag2: " << args.flag("flag2") << std::endl;
    std::cout << "a-param: " << args.param("a-param") << std::endl;

    std::cout << "list: ";
    std::vector<string> paramList = args.paramlist("list");
    for(int i = 0; i < paramList.size(); i++) {
        if (i > 0) std::cout << ", ";
        std::cout << paramList[i];
    }
    std::cout << std::endl;

    std::cout << "names: ";
    paramList = args.paramlist("names");
    for(int i = 0; i < paramList.size(); i++) {
        if (i > 0) std::cout << ", ";
        std::cout << paramList[i];
    }
    std::cout << std::endl;
    return 0;
}