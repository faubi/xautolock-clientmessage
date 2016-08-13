/*****************************************************************************
 *
 * Authors: Michel Eyckmans (MCE) & Stefan De Troch (SDT)
 *
 * Content: This file is part of version 2.x of xautolock. It declares 
 *          the stuff used to implement the program's IPC features.
 *
 *          Please send bug reports etc. to mce@scarlet.be.
 *
 * --------------------------------------------------------------------------
 *
 * Copyright 1990, 1992-1999, 2001-2002, 2004, 2007 by  Stefan De Troch and
 * Michel Eyckmans.
 *
 * Versions 2.0 and above of xautolock are available under version 2 of the
 * GNU GPL. Earlier versions are available under other conditions. For more
 * information, see the License file.
 *
 *****************************************************************************/

#ifndef __message_h
#define __message_h

#include "config.h"
#include "options.h"
#include <sys/socket.h>
#include <sys/select.h>

extern void checkConnectionAndSendMessage (Display* d, Window w);
extern void lookForMessages (Display* d, double timeout);
extern void cleanupSemaphore (Display* d);


typedef struct
{
  response type;
  long data[4];
} fullResponse;

#endif /* __message_h */
