#ifndef MTP_LAB1_MATRIXBUFFER_H
#define MTP_LAB1_MATRIXBUFFER_H

#include <vector>


namespace lab2 {

class MatrixBuffer {

    size_t _nRows, _nCols;
    std::vector<float> _data;

public:

    MatrixBuffer(size_t nRows, size_t nCols)
            : _nRows(nRows)
            , _nCols(nCols)
            , _data() {};

    size_t getNRows() const { return _nRows; }
    size_t getNCols() const { return _nCols; }
    size_t getTotalSize() const { return _nRows * _nCols; }

    float& at(size_t row, size_t col);
    const float& at(size_t row, size_t col) const;

    bool allocate();
    bool isAllocated() const;
    void free();

    void add(const MatrixBuffer&, float coeff = 1);
    void sum(const MatrixBuffer&, const MatrixBuffer&, float coeff = 1);
    void mul(const MatrixBuffer&, const MatrixBuffer&);

    void set(const MatrixBuffer&,
             size_t rowOffs = 0, size_t colOffs = 0,
             size_t srcRowOffs = 0, size_t srcColOffs = 0,
             size_t nRows = 0, size_t nCols = 0);
    void swap(MatrixBuffer&);
    void borrow(MatrixBuffer& m) {
        checkSize(*this, m);
        swap(m);
    }

//    bool resize(size_t nRows, size_t nCols);

private:
    static void checkSize(const MatrixBuffer& m1, const MatrixBuffer& m2);
    static void checkAllocated(const MatrixBuffer& m);

};

}
#endif //MTP_LAB1_MATRIXBUFFER_H
