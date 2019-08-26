#include "cliClass.h"
//#include "mgnClass.h"

extern CMDTABLE cmnTable[];
//extern MGN mgn;                
extern char *channel;

#define BINIT( A, ARG )      BUF *A = (BUF *)ARG[0]
#define RESPONSE( A, ... ) if( bp )                     \
                            bp->add(A, ##__VA_ARGS__ ); \
                           else                         \
                            PF(A, ##__VA_ARGS__);
