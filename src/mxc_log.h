#ifndef MOXAIC_MXC_LOG_H
#define MOXAIC_MXC_LOG_H

#define mxcLogDebug(m) (printf("%s - %s\n", __FUNCTION__, m))
#define mxcLogDebugInfo1(m, t0, i0) (printf("%s - %s - %s: "#t0"\n", __FUNCTION__, m, #i0, i0))
#define mxcLogDebugInfo2(m, t0, i0, t1, i1) (printf("%s - %s - %s: "#t0" - %s: "#t1"\n", __FUNCTION__, m, #i0, i0, #i1, i1))
#define mxcLogDebugInfo3(m, t0, i0, t1, i1,  t2, i2) (printf("%s - %s - %s: "#t0" - %s: "#t1" - %s: "#t2"\n", __FUNCTION__, m, #i0, i0, #i1, i2, #i2, i2))

#endif //MOXAIC_MXC_LOG_H
