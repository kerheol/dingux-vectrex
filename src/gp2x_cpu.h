
#if defined(GP2X_MODE)
#include "cpugp2x.h"
#elif defined(WIZ_MODE)
#include "cpuwiz.h"
#elif defined(CAANOO_MODE)
#include "cpucaanoo.h"
#else
#include "cpuno.h"
#endif
