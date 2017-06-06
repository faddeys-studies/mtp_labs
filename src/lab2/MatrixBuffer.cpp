#include "MatrixBuffer.h"

float& lab2::MatrixBuffer::at(size_t row, size_t col) {
    checkAllocated(*this);
    return _data[row*_nCols + col];
}

const float &lab2::MatrixBuffer::at(size_t row, size_t col) const {
    checkAllocated(*this);
    return _data[row*_nCols + col];
}

bool lab2::MatrixBuffer::allocate() {
    if (isAllocated())
        return true;
    try{
        _data.resize(_nRows*_nCols, 0);
    } catch (const std::bad_alloc&) {
        return false;
    }
    return true;
}

bool lab2::MatrixBuffer::isAllocated() const {
    return !_data.empty();
}

void lab2::MatrixBuffer::free() {
    _data.clear();
    _data.shrink_to_fit();
}

void lab2::MatrixBuffer::add(const lab2::MatrixBuffer &m, float coeff) {
    checkSize(*this, m);
    checkAllocated(*this);
    checkAllocated(m);
    for(size_t i = 0; i < _data.size(); i++) {
        _data[i] += coeff * m._data[i];
    }
}

void lab2::MatrixBuffer::sum(const lab2::MatrixBuffer &m1, const lab2::MatrixBuffer &m2, float coeff) {
    checkAllocated(*this);
    set(m1);
    add(m2, coeff);
}

void lab2::MatrixBuffer::mul(const lab2::MatrixBuffer &m1, const lab2::MatrixBuffer &m2) {
    if (m1._nCols != m2._nRows) {
        throw std::runtime_error("Bad dimensions for matmul");
    }
    checkSize(*this, m1);
    checkSize(*this, m2);
    checkAllocated(m1);
    checkAllocated(m2);
    checkAllocated(*this);

    for (size_t r = 0; r < _nRows; r++) {
        for (size_t c = 0; c < _nCols; c++) {
            at(r, c) = 0;
            for (size_t i = 0; i < _nCols; i++) {
                at(r, c) += m1.at(r, i) * m2.at(i, c);
            }
        }
    }

}

void lab2::MatrixBuffer::set(const lab2::MatrixBuffer &m,
                             size_t rowOffs, size_t colOffs,
                             size_t srcRowOffs, size_t srcColOffs,
                             size_t nRows, size_t nCols) {
    if (nRows == 0) nRows = m._nRows;
    if (nCols == 0) nCols = m._nCols;
    if (_nRows < nRows+rowOffs || _nCols < nCols+colOffs) {
        throw std::runtime_error("Argument matrix does not fit into this matrix at given offset");
    }
    for (size_t r = 0; r < nRows; r++) {
        for (size_t c = 0; c < nCols; c++) {
            at(r+rowOffs, c+colOffs) = m.at(r+srcRowOffs, c+srcColOffs);
        }
    }
}

//bool lab2::MatrixBuffer::resize(size_t nRows, size_t nCols) {
//    if (nCols < _nCols) {
//        for(size_t row = 1; row < std::min(nRows, _nRows); row++) {
//            for(size_t col = 0; col < nCols; col++) {
//                _data[row*nCols + col] = at(row, col);
//            }
//        }
//    }
//    try {
//        _data.resize(nRows*nCols, 0);
//    } catch (const std::bad_alloc&) {
//        return false;
//    }
//    if (nCols > _nCols) {
//
//    }
//}

void lab2::MatrixBuffer::swap(lab2::MatrixBuffer &m) {
    _data.swap(m._data);
    std::swap(_nRows, m._nRows);
    std::swap(_nCols, m._nCols);
}

void lab2::MatrixBuffer::checkSize(const lab2::MatrixBuffer &m1, const lab2::MatrixBuffer &m2) {
    if (m1._nRows!=m2._nRows || m1._nCols!=m2._nCols) {
        throw std::runtime_error("Matrices have different size");
    }
}

void lab2::MatrixBuffer::checkAllocated(const lab2::MatrixBuffer &m) {
    if (!m.isAllocated()) {
        throw std::runtime_error("Buffer is not allocated");
    }
}
