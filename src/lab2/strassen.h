#ifndef MTP_LAB1_STRASSEN_H
#define MTP_LAB1_STRASSEN_H

#include "../mt/TaskGraph.h"
#include "tasks.h"


namespace lab2 {

MatrixOp* matmulStrassen(mt::TaskGraph &graph, Lab2BaseTask* m1, Lab2BaseTask* m2, size_t limit);

}


#endif //MTP_LAB1_STRASSEN_H
