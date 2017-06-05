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

//#define LAB1_DEBUG

#ifdef LAB1_DEBUG
#include "../common.h"
#define lab1_debug(args) debug(args)
#else
#define lab1_debug(args)
#endif


class RowBuffer {
    const size_t _size;
    int _version;
    mutable bool _allocateDone = false;
    mutable std::vector<float> _data;
    mutable bool _wasRead;
    mutable std::mutex _mtx;

public:
    typedef std::vector<float>::const_iterator read_ptr;
    typedef std::vector<float>::iterator write_ptr;

    RowBuffer(size_t size)
            : _size(size)
//            , _data()
            , _data(size, 0)
            , _allocateDone(true)
            , _version(0)
            , _wasRead(true)
            , _mtx()
    {}

    read_ptr reader() const {
        ensureAllocated();
        return _data.cbegin();
    }
    read_ptr end() const {
        ensureAllocated();
        return  _data.cend();
    }
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
        // NOTE: perform this BEFORE locking, since these ops are also synchronized
        ensureAllocated();
        other->ensureAllocated();

        std::unique_lock<std::mutex> _lock{_mtx};
        std::unique_lock<std::mutex> _othersLock{other->_mtx};

        if (_data.size() != other->_data.size())
            throw std::runtime_error("Cannot swap buffers of different size");
        _data.swap(other->_data);
        _version++;
        _wasRead = false;
    }
    write_ptr writer() {
        ensureAllocated();
        return _data.begin();
    }
    write_ptr end() {
        ensureAllocated();
        return  _data.end();
    }

private:

    void ensureAllocated() const {
//        std::unique_lock<std::mutex> _lock{_mtx};
//        if (!_allocateDone) {
//            _data.resize(_size, 0);
//            _allocateDone = true;
//        }
    }

};


class RowProducer : public mt::Task {

public:
    RowProducer(size_t nRows, size_t nCols) : _nRows(nRows), _nCols(nCols) {}

    const RowBuffer* getOutBuffer() const { return _outBuffer; }

protected:
    RowBuffer *_outBuffer;
    size_t _nRowsProduced;
    const size_t _nRows, _nCols;

    virtual bool doStart(const std::vector<mt::Task*>& dependencies) override {
        prepareInternalBuffers(dependencies);
        _outBuffer = new RowBuffer(_nCols);
        _nRowsProduced = 0;
        return false;
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
        lab1_debug("start read #" << _nRowsProduced+1 << " by " << getId());
        for(auto it = _readBuffer->writer(); it != _readBuffer->end(); it++) {
            *_inFile >> *it;
        }
        lab1_debug("done  read #" << _nRowsProduced+1 << " by " << getId());
        return _readBuffer;
    }
};


class RowAdder : public RowProducer {
    RowBuffer *_sumBuffer;
    const RowProducer *_summand1Prod, *_summand2Prod;

public:
    RowAdder(size_t nRows, size_t nCols)
            : RowProducer(nRows, nCols) {}

protected:

    virtual bool isWaiting() override {
        if (_summand1Prod->getOutBuffer() == nullptr ||
                _summand2Prod->getOutBuffer() == nullptr) {
            return true;
        }
        return RowProducer::isWaiting();
    }

    virtual void prepareInternalBuffers(const std::vector<mt::Task *> &dependencies) override {
        assert(dependencies.size() == 2);
        _summand1Prod = dynamic_cast<RowProducer*>(dependencies[0]);
        _summand2Prod = dynamic_cast<RowProducer*>(dependencies[1]);
        assert(_summand1Prod != nullptr);
        assert(_summand2Prod != nullptr);

        _sumBuffer = new RowBuffer(_nCols);
    }
    virtual void destroyInternalBuffers() override {
        delete _sumBuffer;
        _summand1Prod = nullptr;
        _summand2Prod = nullptr;
    }
    virtual bool hasNextBuffer() override {
        return !_summand1Prod->getOutBuffer()->wasRead() && !_summand2Prod->getOutBuffer()->wasRead();
    }
    virtual RowBuffer* getNextBuffer() override {
        lab1_debug("start sum  #" << _nRowsProduced+1 << " by " << getId()
                   << " from " << _summand1Prod->getId() << " and " << _summand2Prod->getId());
        auto sum = _sumBuffer->writer();
        auto summand1 = _summand1Prod->getOutBuffer()->reader(),
             summand2 = _summand2Prod->getOutBuffer()->reader();
        while(sum != _sumBuffer->end()) {
            *sum = *summand1 + *summand2;
            ++sum; ++summand1; ++summand2;
        }
        _summand1Prod->getOutBuffer()->readDone();
        _summand2Prod->getOutBuffer()->readDone();
        lab1_debug("done  sum  #" << _nRowsProduced+1 << " by " << getId()
                   << " from " << _summand1Prod->getId() << " and " << _summand2Prod->getId());
        return _sumBuffer;
    }

};


class RowWriter : public mt::Task {
    const std::string& filename;
    const size_t _nRows;
    const bool _progress;

    const RowProducer *_sourceProducer;
    std::ofstream *_outFile;
    size_t _wroteRows;

public:
    RowWriter(const std::string& filename, const size_t nRows, bool progress=false)
            : filename(filename), _nRows(nRows), _progress(progress) {}

protected:

    virtual bool doStart(const std::vector<mt::Task*> &dependencies) override {
        assert(dependencies.size() == 1);
        _sourceProducer = dynamic_cast<RowProducer*>(dependencies[0]);
        assert(_sourceProducer != nullptr);
        _outFile = new std::ofstream(filename);
        _wroteRows = 0;
        return false;
    }

    virtual bool isWaiting() override {
        if (_sourceProducer->getOutBuffer() == nullptr) return true;
        return _sourceProducer->getOutBuffer()->wasRead();
    }

    virtual bool doWorkPortion() override {
        auto src = _sourceProducer->getOutBuffer();
        for(auto it = src->reader(); it != src->end(); it++) {
            *_outFile << *it << ' ';
        }
        *_outFile << std::endl;
        src->readDone();
        _wroteRows++;
        if (_progress) {
            if (_wroteRows > 1) std::cout << '\r';
            std::cout << _wroteRows << '/' << _nRows;
            if (_wroteRows >= _nRows) std::cout << std::endl;
            std::cout.flush();
        }
        lab1_debug("wrote row #" << _wroteRows);
        return _wroteRows >= _nRows;
    }

    virtual void doFinalize() override {
        _sourceProducer = nullptr;
        _outFile->close();
        delete _outFile;
    }

};


}


#endif //MTP_LAB1_TASKS_H
