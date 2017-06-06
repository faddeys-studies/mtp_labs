
#include <vector>
#include <chrono>
#include <iostream>
#include "../cli-args/Parser.h"
#include "../mt/TaskGraph.h"
#include "tasks.h"
#include "strassen.h"


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

size_t getPaddedSize(size_t origSize) {
    size_t result = 1;
    while (result < origSize) result <<= 1;
    return result;
}

using namespace lab2;


int main(int argc, char **argv) {
    cli::Parser parser{"lab2", "Multiplies matrices from given files"};
    parser  .param("n-threads", "-n", "", "Number of threads")
            .param("size", "-N", "", "Matrix dimensions")
            .param("strassen-limit", "-L", "", "Size limit for stopping Strassen's algorithm")
            .param("out-name", "-o", "", "Output file name")
            .positional("in-names", "+");
    auto args = parser.parse(argc, argv);
    unsigned nWorkers = getPositive(args, parser, "n-threads");
    size_t limit = getPositive(args, parser, "strassen-limit");
    size_t matSize = getPositive(args, parser, "size");
    size_t paddedSize = getPaddedSize(matSize);

    mt::TaskGraph graph;

    std::vector<Lab2BaseTask*> matricesWave{};
    for(const auto& name : args.paramlist("in-names")) {
        Lab2BaseTask* loader = new MatrixReader(
                name,
                matSize, matSize,
                paddedSize, paddedSize
        );
        matricesWave.push_back(loader);
        graph.addTask(loader, {});
    }

    while(matricesWave.size() > 1) {
        std::vector<Lab2BaseTask*> matricesNextWave{};
        for (size_t i = 1; i < matricesWave.size(); i+=2) {
            Lab2BaseTask* result = matmulStrassen(
                    graph, matricesWave[i-1], matricesWave[i], limit);
            matricesNextWave.push_back(result);
        }
        if (matricesWave.size() % 2 == 1)
            matricesNextWave.push_back(matricesWave[matricesWave.size()-1]);
        matricesWave.swap(matricesNextWave);
    }
    auto saver = new MatrixWriter(args.param("out-name"), matSize, matSize);
    graph.addTask(saver, {matricesWave[0]});

    using namespace std::chrono;
    milliseconds start = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    graph.runAll(nWorkers);
    milliseconds end = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    auto dur = end-start;
    std::cout << "time: " << dur.count()/1000.0 << "s" << std::endl;

}
