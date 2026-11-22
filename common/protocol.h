#ifndef PROTOCOL_H
#define PROTOCOL_H

#define SCAN_PORT          9000      /* cong UDP thiet bi lang nghe lenh quet */

#define CMD_SCAN           "SCAN"    /* controller -> thiet bi (UDP) */
#define CMD_DEVICE         "DEVICE"  /* thiet bi -> controller: DEVICE|type|id|tcp_port */
#define CMD_ERR            "ERR"     /* ERR|code */

#define ERR_BUSY           "BUSY"
#define ERR_UNKNOWN_CMD    "UNKNOWN_CMD"
#define ERR_LINE_TOO_LONG  "LINE_TOO_LONG"

#endif
