#ifndef MOXAIC_MXC_LOG_H
#define MOXAIC_MXC_LOG_H

#define mxcLogDebug(m) (printf("%s - %s\n", __FUNCTION__, m))
#define mxcLogDebugInfo(m, t, i) (printf("%s - %s - %s: "#t"\n", __FUNCTION__, m, #i, i))
#define mxcLogDebugInfo2(m, t, i, t2, i2) (printf("%s - %s - %s: "#t" - %s: "#t2"\n", __FUNCTION__, m, #i, i, #i2, i2))

#endif //MOXAIC_MXC_LOG_H
