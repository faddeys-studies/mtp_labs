
#include "strassen.h"


lab2::MatrixOp*
defineSum(mt::TaskGraph &graph, lab2::Lab2BaseTask *m1, lab2::Lab2BaseTask *m2, float coeff=1, bool borrow=false) {
    auto sum = new lab2::Addition(m1->getNRows(), m2->getNCols(), coeff, borrow);
    graph.addTask(sum, {m1, m2});
    return sum;
}


lab2::MatrixOp*
lab2::matmulStrassen(mt::TaskGraph &graph, Lab2BaseTask *m1, Lab2BaseTask *m2, size_t limit) {
    size_t matSz = m1->getNCols();
    if (matSz <= limit) {
        auto mul = new Multiplication(m1->getNRows(), m2->getNCols());
        graph.addTask(mul, {m1, m2});
        return mul;
    }
    size_t subSz = matSz / 2;
    auto A11 = new Subscripting(subSz, subSz, 0,     0);
    auto A12 = new Subscripting(subSz, subSz, 0,     subSz);
    auto A21 = new Subscripting(subSz, subSz, subSz, 0);
    auto A22 = new Subscripting(subSz, subSz, subSz, subSz);
    graph.addTask(A11, {m1});
    graph.addTask(A12, {m1});
    graph.addTask(A21, {m1});
    graph.addTask(A22, {m1});

    auto B11 = new Subscripting(subSz, subSz, 0,     0);
    auto B12 = new Subscripting(subSz, subSz, 0,     subSz);
    auto B21 = new Subscripting(subSz, subSz, subSz, 0);
    auto B22 = new Subscripting(subSz, subSz, subSz, subSz);
    graph.addTask(B11, {m2});
    graph.addTask(B12, {m2});
    graph.addTask(B21, {m2});
    graph.addTask(B22, {m2});

    auto P1 = matmulStrassen(graph,
                             defineSum(graph, A11, A22),
                             defineSum(graph, B11, B22),
                             limit);
    auto P2 = matmulStrassen(graph,
                             defineSum(graph, A21, A22),
                             B11,
                             limit);
    auto P3 = matmulStrassen(graph,
                             A11,
                             defineSum(graph, B12, B22, -1),
                             limit);
    auto P4 = matmulStrassen(graph,
                             A22,
                             defineSum(graph, B21, B11, -1),
                             limit);
    auto P5 = matmulStrassen(graph,
                             defineSum(graph, A11, A12),
                             B22,
                             limit);
    auto P6 = matmulStrassen(graph,
                             defineSum(graph, A21, A11, -1),
                             defineSum(graph, B11, B12),
                             limit);
    auto P7 = matmulStrassen(graph,
                             defineSum(graph, A12, A22, -1),
                             defineSum(graph, B21, B22),
                             limit);

    auto C11 = defineSum(graph, defineSum(graph, P1, P4), defineSum(graph, P7, P5, -1));
    auto C12 = defineSum(graph, P3, P5);
    auto C21 = defineSum(graph, P2, P4);
    auto C22 = defineSum(graph, defineSum(graph, P1, P2, -1), defineSum(graph, P3, P6));

    auto C = new BlockMatrix(matSz, matSz);
    graph.addTask(C, {C11, C12, C21, C22});
    return C;
}
