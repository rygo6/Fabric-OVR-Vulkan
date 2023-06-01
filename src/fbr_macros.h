#ifndef FABRIC_MACROS_H
#define FABRIC_MACROS_H

#define CONCAT(A,B)         A ## B
#define EXPAND_CONCAT(A,B)  CONCAT(A, B)

#define COUNT(array) (sizeof(array)/sizeof(array[0]))

#endif //FABRIC_MACROS_H
