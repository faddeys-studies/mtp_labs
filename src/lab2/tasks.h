#ifndef MTP_LAB1_TASKS_H
#define MTP_LAB1_TASKS_H

#include <string>
#include <fstream>
#include <sstream>
#include <cassert>

#include "../mt/Task.h"
#include "MatrixBuffer.h"


namespace lab2 {

class Lab2BaseTask : public mt::Task {
    friend class MatrixReader;
    friend class MatrixOp;
    friend class MatrixWriter;

public:

    Lab2BaseTask(size_t nRows, size_t nCols)
            : _result(nRows, nCols) {}

    bool hasFailed() const {
        std::unique_lock<std::mutex> _lk{_failMtx};
        return _failed;
    }

    const std::string& getFailCause() const {
        std::unique_lock<std::mutex> _lk{_failMtx};
        return _failCause;
    }

    size_t getNRows() { return _result.getNRows(); }
    size_t getNCols() { return _result.getNCols(); }

protected:
    MatrixBuffer _result;

    void fail(const std::string &cause) {
        std::unique_lock<std::mutex> _lk{_failMtx};
        _failed = true;
        _failCause = cause;
    }

    bool allocateBuffer() {
        if (!_result.allocate()) {
            std::stringstream ss;
            ss << "Cannot allocate buffer of size "
               << _result.getNRows() << 'x' << _result.getNCols()
               << " for task #" << getId();
            fail(ss.str());
            return false;
        }
        return true;
    }

    void doFinalize() {
        if (_result.isAllocated()) _result.free();
    }

private:
    bool _failed = false;
    mutable std::mutex _failMtx;
    std::string _failCause;

};


class MatrixReader : public Lab2BaseTask {

    const std::string _filename;
    const size_t _nominalNRows, _nominalNCols;

public:

    MatrixReader(const std::string &filename,
                 size_t nominalNRows, size_t nominalNCols,
                 size_t realNRows, size_t realNCols)
            : Lab2BaseTask(realNRows, realNCols)
            , _nominalNRows(nominalNRows)
            , _nominalNCols(nominalNCols)
            , _filename(filename) {}

    bool doWorkPortion() override {
        if (!allocateBuffer())
            return true;

        std::ifstream file{_filename};
        for (size_t r = 0; r < _nominalNRows; r++) {
            for (size_t c = 0; c < _nominalNCols; c++) {
                file >> _result.at(r, c);
            }
        }
        return true;
    }

    bool isWaiting() override { return false; };

};


class MatrixOp : public Lab2BaseTask {

    const size_t _nArgs;

public:

    MatrixOp(size_t nRows, size_t nCols, size_t nArgs)
            : Lab2BaseTask(nRows, nCols), _nArgs(nArgs) {}

    std::vector<Lab2BaseTask*> _dependencies;

protected:

    std::vector<MatrixBuffer*> _arguments;

    bool doStart(const std::vector<mt::Task*> &dependencies) override {
        for(auto dep : dependencies) {
            auto argDep = dynamic_cast<Lab2BaseTask*>(dep);
            if (argDep != nullptr) {
                _dependencies.push_back(argDep);
                _arguments.push_back(&argDep->_result);
            }
        }
        assert(_arguments.size() == _nArgs);
        return false;
    }

    bool isWaiting() override {
        for(auto dep : _dependencies) {
            if (!dep->isDone()) return true;
        }
        return false;
    }

    bool doWorkPortion() override {
        if (!checkFail())
            performOp();
        return true;
    }

    virtual void performOp() = 0;

    bool checkFail() {
        for (auto dep : _dependencies) {
            if (dep->hasFailed()) {
                fail(dep->getFailCause());
                return true;
            }
        }
        return false;
    }

};


class Subscripting : public MatrixOp {

    const size_t _rowOffs;
    const size_t _colOffs;

public:
    Subscripting(size_t nRows, size_t nCols, size_t rowOffs, size_t colOffs)
            : MatrixOp(nRows, nCols, 1)
            , _rowOffs(rowOffs)
            , _colOffs(colOffs)
    {}

protected:

    void performOp() override {
        if (!allocateBuffer()) return;
        _result.set(*_arguments[0],
                    0, 0, // where to start filling our result
                    _rowOffs, _colOffs, // where to take data from argument
                    _result.getNRows(), _result.getNCols() // how many to take
        );
    }

};


class Addition : public MatrixOp {

    const bool _borrowFromFirst;
    const float _coeff;

public:
    Addition(size_t nRows, size_t nCols, float coeff=1, bool borrow=false)
            : MatrixOp(nRows, nCols, 2), _coeff(coeff), _borrowFromFirst(borrow) {}

protected:

    void performOp() override {
        if (_borrowFromFirst) {
            _result.borrow(*_arguments[0]);
            _result.add(*_arguments[1], _coeff);
        } else {
            if (!allocateBuffer()) return;
            _result.sum(*_arguments[0], *_arguments[1], _coeff);
        }
    }

};


class Multiplication : public MatrixOp {
public:
    Multiplication(size_t nRows, size_t nCols) : MatrixOp(nRows, nCols, 2) {}

protected:

    void performOp() override {
        if (!allocateBuffer()) return;
        _result.mul(*_arguments[0], *_arguments[1]);
    }

};


class BlockMatrix : public MatrixOp {
public:
    BlockMatrix(size_t nRows, size_t nCols) : MatrixOp(nRows, nCols, 4) {}

protected:

    void performOp() override {
        if (!allocateBuffer()) return;
        _result.set(*_arguments[0], 0,                         0);
        _result.set(*_arguments[1], 0,                         _arguments[0]->getNCols());
        _result.set(*_arguments[2], _arguments[0]->getNRows(), 0);
        _result.set(*_arguments[3], _arguments[0]->getNRows(), _arguments[0]->getNCols());
    }

};


class MatrixWriter : public mt::Task {
    const std::string _filename;
    const size_t _nRows;
    const size_t _nCols;

    Lab2BaseTask* _source;

public:
    MatrixWriter(const std::string &filename,
                 const size_t nRows, const size_t nCols)
            : _filename(filename)
            , _nRows(nRows)
            , _nCols(nCols)
    {}

protected:

    bool doStart(const std::vector<mt::Task*>& deps) override {
        assert(deps.size() == 1);
        _source = dynamic_cast<Lab2BaseTask*>(deps[0]);
        assert(_source != nullptr);
        return false;
    }

    bool isWaiting() override {
        return !_source->isDone();
    }

    bool doWorkPortion() override {
        if (_source->hasFailed()) return true;
        std::ofstream file{_filename};
        auto &data = _source->_result;
        for(size_t row = 0; row < _nRows; row++) {
            for(size_t col = 0; col < _nCols; col++) {
                file << data.at(row, col) << ' ';
            }
            file << '\n';
        }
        file.close();
        return true;
    }

};


}


#endif //MTP_LAB1_TASKS_H
