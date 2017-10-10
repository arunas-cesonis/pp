#ifndef PP_MACROS_H
#define PP_MACROS_H

#define PP_DOT() .
#define _PP_STR(X) # X
#define PP_STR(X) _PP_STR(X)
#define _PP_P(X) X
#define PP_P(X) _PP_P(X)


#define N_0
#define N_1
#define N_2
#define N_4
#define N_5
#define N_6
#define N_7
#define N_9 

#define _INC(X) CAT(INC_,X)
#define INC(X) _INC(X)

#define _CAT(A,B) A ## B
#define CAT(A,B) _CAT(A,B)

#define VALID_C(C) ((C >= 'a' && C <= 'z')\
|| (C >= 'A' && C <= 'Z' && (C += 'a' - 'A'))\
|| (C >= '0' && C <= '9')\
|| (C == '.' || C == '-' || C == '_'))

#endif
