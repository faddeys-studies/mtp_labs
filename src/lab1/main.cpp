#include <iostream>
#include <thread>
#include <queue>
#include <chrono>

#include "../cli-args/Parser.h"
#include "tasks.h"
#include "../mt/TaskGraph.h"


unsigned getPositive(const cli::Arguments& args,
                     const cli::Parser &parser,
                     const std::string &key) {
    int result = 0;
    try {
        result = std::stoi(args.param(key));
    } catch (const std::invalid_argument&) {
        parser.fail(key, "Required integer", true);
    }
    if (result <= 0) {
        parser.fail(key, "Required positive integer", true);
    }
    return (unsigned)result;
}


int main(int argc, char **argv) {
    cli::Parser parser{"lab1", "Adds matrices from given files"};
    parser  .param("n-threads", "-n", "", "Number of threads")
            .param("rows", "-r", "", "Number of rows")
            .param("cols", "-c", "", "Number of columns")
            .param("out-name", "-o", "", "Output file name")
            .positional("in-names", "+");
    auto args = parser.parse(argc, argv);
    unsigned nWorkers = getPositive(args, parser, "n-threads");
    unsigned nRows = getPositive(args, parser, "rows");
    unsigned nCols = getPositive(args, parser, "cols");

    std::queue<mt::Task*> tasks;
    mt::TaskGraph graph;

    for(const auto& inName : args.paramlist("in-names")) {
        mt::Task *t = new lab1::RowReader(nRows, nCols, inName);
        tasks.push(t);
        graph.addTask(t, {});
    }

    while (tasks.size() > 1) {
        mt::Task *left = tasks.front();
        tasks.pop();
        mt::Task *right = tasks.front();
        tasks.pop();
        mt::Task *sum = new lab1::RowAdder(nRows, nCols);
        graph.addTask(sum, {left, right});
        tasks.push(sum);
    }
    mt::Task *totalSum = tasks.front();
    mt::Task *writer = new lab1::RowWriter(args.param("out-name"), nRows);
    graph.addTask(writer, {totalSum});

    using namespace std::chrono;
    milliseconds start = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    graph.runAll(nWorkers);
    milliseconds end = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    auto dur = end-start;
    std::cout << "time: " << dur.count()/1000.0 << "s" << std::endl;

    return 0;
}