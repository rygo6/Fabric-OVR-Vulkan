#ifndef FABRIC_MXC_LOG_H
#define FABRIC_MXC_LOG_H

#define fbrLogDebug(m) (printf("%s - %s\n", __FUNCTION__, m))
#define fbrLogDebugInfo1(m, t0, i0) (printf("%s - %s - %s: "#t0"\n", __FUNCTION__, m, #i0, i0))
#define fbrLogDebugInfo2(m, t0, i0, t1, i1) (printf("%s - %s - %s: "#t0" - %s: "#t1"\n", __FUNCTION__, m, #i0, i0, #i1, i1))
#define fbrLogDebugInfo3(m, t0, i0, t1, i1,  t2, i2) (printf("%s - %s - %s: "#t0" - %s: "#t1" - %s: "#t2"\n", __FUNCTION__, m, #i0, i0, #i1, i2, #i2, i2))

#endif //FABRIC_MXC_LOG_H
