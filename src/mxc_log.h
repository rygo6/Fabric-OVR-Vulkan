#ifndef MOXAIC_MXC_LOG_H
#define MOXAIC_MXC_LOG_H

#define mxcLogDebug(m) (printf("%s - %s\n", __FUNCTION__, m))
#define mxcLogDebugInfo(m, t, i) (printf("%s - %s - %s: "#t"\n", __FUNCTION__, m, #i, i))
#define mxcLogDebugInfo2(m, t, i, t2, i2) (printf("%s - %s - %s: "#t" - %s: "#t2"\n", __FUNCTION__, m, #i, i, #i2, i2))
#define mxcLogDebugInfo3(m, t0, i0, t1, i1,  t2, i2) (printf("%s - %s - %s: "#t0" - %s: "#t1" - %s: "#t2"\n", __FUNCTION__, m, #i0, i0, #i1, i2, #i2, i2))

#endif //MOXAIC_MXC_LOG_H
