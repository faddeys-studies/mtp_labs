#ifndef MTP_LAB1_TASKS_H
#define MTP_LAB1_TASKS_H

#include "../mt/Task.h"
#include <vector>
#include <mutex>
#include <string>
#include <fstream>
#include <cassert>
#include <iostream>


namespace lab1 {



class RowBuffer {
    std::vector<float> _data;
    int _version;
    mutable bool _wasRead;
    mutable std::mutex _mtx;

public:
    typedef std::vector<float>::const_iterator read_ptr;
    typedef std::vector<float>::iterator write_ptr;

    RowBuffer(size_t size)
            : _data(size, 0)
            , _version(0)
            , _wasRead(true)
            , _mtx()
    {}

    read_ptr reader() const { return _data.cbegin(); }
    read_ptr end() const { return  _data.cend(); }
    void readDone() const {
        std::unique_lock<std::mutex> _lock{_mtx};
        _wasRead = true;
    }
    bool wasRead() const {
        std::unique_lock<std::mutex> _lock{_mtx};
        return _wasRead;
    }
    int version() const { return _version; }

    void swap(RowBuffer *other) {
        std::unique_lock<std::mutex> _lock{_mtx};
        std::unique_lock<std::mutex> _othersLock{other->_mtx};
        if (_data.size() != other->_data.size())
            throw std::runtime_error("Cannot swap buffers of different size");
        _data.swap(other->_data);
        _version++;
        _wasRead = false;
    }
    write_ptr writer() { return _data.begin(); }
    write_ptr end() { return  _data.end(); }

};


class RowProducer : public mt::Task {

public:
    RowProducer(size_t nRows, size_t nCols) : _nRows(nRows), _nCols(nCols) {}

    const RowBuffer* getOutBuffer() const { return _outBuffer; }

protected:
    RowBuffer *_outBuffer;
    size_t _nRowsProduced;
    const size_t _nRows, _nCols;

    virtual void doStart(const std::vector<mt::Task*>& dependencies) override {
        prepareInternalBuffers(dependencies);
        _outBuffer = new RowBuffer(_nCols);
        _nRowsProduced = 0;
    }

    virtual bool isWaiting() override {
        return !hasNextBuffer() || !_outBuffer->wasRead();
    }

    virtual bool doWorkPortion() override {
        auto nextBuf = getNextBuffer();
        if (nextBuf == nullptr|| !_outBuffer->wasRead())
            return false; // this should never happen
        _outBuffer->swap(nextBuf);
        _nRowsProduced++;
        return _nRowsProduced >= _nRows;
    }

    virtual void doFinalize() override {
        delete _outBuffer;
        destroyInternalBuffers();
    }

    virtual void prepareInternalBuffers(const std::vector<mt::Task *> &dependencies) = 0;
    virtual void destroyInternalBuffers() = 0;
    virtual RowBuffer* getNextBuffer() = 0;
    virtual bool hasNextBuffer() = 0;

};


class RowReader : public RowProducer {

    const std::string& filename;
    RowBuffer *_readBuffer;
    std::ifstream *_inFile;

public:
    RowReader(size_t nRows, size_t nCols, const std::string& filename)
            : RowProducer(nRows, nCols), filename(filename) {}

protected:

    virtual void prepareInternalBuffers(const std::vector<mt::Task *> &dependencies) override {
        assert(dependencies.size() == 0);
        _readBuffer = new RowBuffer(_nCols);
        _inFile = new std::ifstream(filename);
    }
    virtual void destroyInternalBuffers() override {
        delete _readBuffer;
        _inFile->close();
        delete _inFile;
    }
    virtual bool hasNextBuffer() override {
        return true;
    }
    virtual RowBuffer* getNextBuffer() override {
        for(auto it = _readBuffer->writer(); it != _readBuffer->end(); it++) {
            *_inFile >> *it;
        }
        return _readBuffer;
    }
};


class RowAdder : public RowProducer {
    RowBuffer *_sumBuffer;
    const RowBuffer *_summand1Buffer, *_summand2Buffer;

public:
    RowAdder(size_t nRows, size_t nCols)
            : RowProducer(nRows, nCols) {}

protected:
    virtual void prepareInternalBuffers(const std::vector<mt::Task *> &dependencies) override {
        assert(dependencies.size() == 2);
        RowProducer *prod1 = dynamic_cast<RowProducer*>(dependencies[0]);
        RowProducer *prod2 = dynamic_cast<RowProducer*>(dependencies[1]);
        assert(prod1 != nullptr);
        assert(prod2 != nullptr);

        _sumBuffer = new RowBuffer(_nCols);
        _summand1Buffer = prod1->getOutBuffer();
        _summand2Buffer = prod2->getOutBuffer();
    }
    virtual void destroyInternalBuffers() override {
        delete _sumBuffer;
        _summand1Buffer = nullptr;
        _summand2Buffer = nullptr;
    }
    virtual bool hasNextBuffer() override {
        return !_summand1Buffer->wasRead() && !_summand2Buffer->wasRead();
    }
    virtual RowBuffer* getNextBuffer() override {
        auto sum = _sumBuffer->writer();
        auto summand1 = _summand1Buffer->reader(),
             summand2 = _summand2Buffer->reader();
        while(sum != _sumBuffer->end()) {
            *sum = *summand1 + *summand2;
            ++sum; ++summand1; ++summand2;
        }
        _summand1Buffer->readDone();
        _summand2Buffer->readDone();
        return _sumBuffer;
    }

};


class RowWriter : public mt::Task {
    const std::string& filename;
    const size_t _nRows;
    const bool _progress;

    const RowBuffer *_readFromBuffer;
    std::ofstream *_outFile;
    size_t _wroteRows;

public:
    RowWriter(const std::string& filename, const size_t nRows, bool progress=false)
            : filename(filename), _nRows(nRows), _progress(progress) {}

protected:

    virtual void doStart(const std::vector<mt::Task*> &dependencies) override {
        assert(dependencies.size() == 1);
        RowProducer *producer = dynamic_cast<RowProducer*>(dependencies[0]);
        assert(producer != nullptr);
        _readFromBuffer = producer->getOutBuffer();
        _outFile = new std::ofstream(filename);
        _wroteRows = 0;
    }

    virtual bool isWaiting() override {
        return _readFromBuffer->wasRead();
    }

    virtual bool doWorkPortion() override {
        for(auto it = _readFromBuffer->reader(); it != _readFromBuffer->end(); it++) {
            *_outFile << *it << ' ';
        }
        *_outFile << std::endl;
        _readFromBuffer->readDone();
        _wroteRows++;
        if (_progress) {
            if (_wroteRows > 1) std::cout << '\r';
            std::cout << _wroteRows << '/' << _nRows;
            if (_wroteRows >= _nRows) std::cout << std::endl;
            std::cout.flush();
        }
        return _wroteRows >= _nRows;
    }

    virtual void doFinalize() override {
        _readFromBuffer = nullptr;
        _outFile->close();
        delete _outFile;
    }

};


}


#endif //MTP_LAB1_TASKS_H
