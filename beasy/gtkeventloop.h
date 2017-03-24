#ifndef _PIDGINEVENTLOOP_H_
#define _PIDGINEVENTLOOP_H_

#include "eventloop.h"

/**
 * Returns the GTK+ event loop UI operations structure.
 *
 * @return The GTK+ event loop UI operations structure.
 */
OulEventLoopUiOps*  beasy_eventloop_get_ui_ops(void);

#endif /* _PIDGINEVENTLOOP_H_ */
