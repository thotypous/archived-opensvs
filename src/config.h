#ifndef _osvs_config_h
#define _osvs_config_h

#define CONFIG_CONNECT_HOST "localhost"
#define CONFIG_CONNECT_PORT 6667
#define CONFIG_CONNECT_RETRY_TIME 10 /* seconds */
#define CONFIG_CONNECT_PASSWORD "pass"

#define CONFIG_SVS_SERVER "services.openbrasil.org"
#define CONFIG_SVS_DESCRIPTION "openbrasil services"
#define CONFIG_SVS_HOST "openbrasil.org"

#define CONFIG_BUFFER_LINEMAX  1026
#define CONFIG_CHANNEL_NAMEMAX   50

/* hash tables sizes, please set only 2^n-1 values */
#define CONFIG_USERCACHE_SIZE 127
#define CONFIG_KILLPROT_SIZE  127
#define CONFIG_ANTIFLOOD_SIZE 255

#define CONFIG_ANTIFLOOD_LIMIT       3
#define CONFIG_ANTIFLOOD_PERSEC      5
#define CONFIG_ANTIFLOOD_IGNORETIME 60 /* seconds */

#define HIGHFIRST  /* little-endian machine */

#endif
