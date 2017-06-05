#ifndef MTP_LAB1_TASKS_H
#define MTP_LAB1_TASKS_H

#include "../mt/Task.h"
#include <vector>
#include <mutex>
#include <string>
#include <fstream>
#include <cassert>


namespace lab1_v2 {

class MatrixProducer : public mt::Task {

    friend class FileReader;
    friend class MatrixSummator;
    friend class FileWriter;

public:

    MatrixProducer(const size_t nRows, const size_t nCols)
            : _nRows(nRows), _nCols(nCols), _dep_producers() {}


protected:
    std::vector<float> *_data;

    const size_t _nRows, _nCols;
    std::vector<MatrixProducer*> _dep_producers;


    virtual bool doStart(const std::vector<mt::Task*>& dependencies) override {
        _data = nullptr;
        _dep_producers.clear();
        for(auto t : dependencies) {
            auto mp = dynamic_cast<MatrixProducer*>(t);
            if (mp != nullptr) _dep_producers.push_back(mp);
        }
        return false;
    }

    virtual bool isWaiting() override {
        for (auto mp : _dep_producers) {
            if (!mp->isDone()) return true;
        }
        bool allHaveBuffer = true;
        for (auto mp : _dep_producers) {
            if (mp->_data == nullptr) {
                allHaveBuffer = false;
                break;
            }
        }
        assert(allHaveBuffer);
        return false;
    }

    virtual void doFinalize() override {
        if (_data != nullptr) delete _data;
        _dep_producers.clear();
    }

};


class FileReader : public MatrixProducer {

    const std::string _filename;

public:

    FileReader(const std::string &filename,
               const size_t nRows, const size_t nCols)
            : _filename(filename), MatrixProducer(nRows, nCols) {}

protected:

    virtual bool doWorkPortion() {
        if (_data == nullptr) {
            auto v = new std::vector<float>(_nRows*_nCols, 0);
            _data = v;
        }
        std::ifstream file{_filename};
        for(auto it = _data->begin(); it != _data->end(); it++) {
            file >> *it;
        }
        return true;
    }

};


class MatrixSummator : public MatrixProducer {

public:

    MatrixSummator(const size_t nRows, const size_t nCols)
            : MatrixProducer(nRows, nCols) {}

protected:

    virtual bool doWorkPortion() {

        for(auto mp : _dep_producers) {
            if (_data == nullptr) {
                _data = mp->_data;
                mp->_data = nullptr;
            } else {
                assert(_data->size() == mp->_data->size());
                for(size_t i = 0; i < _data->size(); i++) {
                    _data->at(i) += mp->_data->at(i);
                }
            }
        }
        return true;
    }

};

class FileWriter : public MatrixProducer {

    const std::string _filename;

public:

    FileWriter(const std::string &filename,
               const size_t nRows, const size_t nCols)
            : _filename(filename), MatrixProducer(nRows, nCols) {}

protected:

    virtual bool doWorkPortion() {

        assert(_dep_producers.size() == 1);
        std::ofstream file{_filename};
        auto data = _dep_producers[0]->_data;
        for(size_t row = 0; row < _nRows; row++) {
            for(size_t col = 0; col < _nCols; col++) {
                file << data->at(row*_nRows + col) << ' ';
            }
            file << '\n';
        }
        file.close();
        return true;
    }

};



}


#endif //MTP_LAB1_TASKS_H
