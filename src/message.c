/*****************************************************************************
*
* Authors: Michel Eyckmans (MCE) & Stefan De Troch (SDT)
*
* Content: This file is part of version 2.x of xautolock. It implements
*          the program's IPC features.
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

#include "message.h"
#include "state.h"
#include "options.h"
#include "miscutil.h"

static Atom semaphore;       /* semaphore property for locating 
                                an already running xautolock    */
static Atom messageRequest;  /* indicates a request to the
                                already running process         */
static Atom messageResponse; /* indicates a response from the
                                already running process         */

#define SEM_PID "_SEMAPHORE_WINDOW_"  
#define MESSAGE_REQUEST "_MESSAGE_REQUEST"
#define MESSAGE_RESPONSE "_MESSAGE_RESPONSE"

/*
*  Message handlers.
*/
static void
disableByMessage (Display* d, Window root, fullResponse* response)
{
  /*
  *  The order in which things are done is rather important here.
  */
  if (!secure)
  {
    setLockTrigger (lockTime);
    disableKillTrigger ();
    disabled = True;
    response->type = response_success;
  }
  else
  {
    response->type = response_failure;
  }
}

static void
enableByMessage (Display* d, Window root, fullResponse* response)
{
  if (!secure) 
  {
    resetTriggers ();
    disabled = False;
    response->type = response_success;
  }
  else
  {
    response->type = response_failure;
  }
}

static void
toggleByMessage (Display* d, Window root, fullResponse* response)
{
  if (!secure)
  {
    if ((disabled = !disabled)) /* = intended */
    {
      setLockTrigger (lockTime);
      disableKillTrigger ();
    }
    else
    {
      resetTriggers ();
    }
    response->type = response_success;
  }
  else
  {
    response->type = response_failure;
  }
}

static void
exitByMessage (Display* d, Window root, fullResponse* response)
{
  if (!secure)
  {
    error0 ("Exiting. Bye bye...\n");
    exit (0);
    response->type = response_success;
  }
  else
  {
    response->type = response_failure;
  }
}

static void
lockNowByMessage (Display* d, Window root, fullResponse* response)
{
  if (!secure && !disabled)
  {
    lockNow = True;
    response->type = response_success;
  }
  else
  {
    response->type = response_failure;
  }
}

static void
unlockNowByMessage (Display* d, Window root, fullResponse* response)
{
  if (!secure && !disabled)
  {
    unlockNow = True;
    response->type = response_success;
  }
  else
  {
    response->type = response_failure;
  }
}

static void
restartByMessage (Display* d, Window root, fullResponse* response)
{
  if (!secure)
  {
    XDeleteProperty (d, root, semaphore);
    XFlush (d);
    execv (argArray[0], argArray);
    response->type = response_success;
  }
  else
  {
    response->type = response_failure;
  }
}

static void
isDisabledMessage (Display* d, Window root, fullResponse* response)
{
  response->type = response_bool;
  response->data[0] = disabled;
}

/*
*  Event handler function passed to eventListen. Receives the display and
*  a pointer to the event being handles and return a Bool specifying whether
*  to continue to receive events.
*/
typedef Bool (*eventHandler) (Display*, XEvent*);

void
eventListen(Display* d, double timeout, eventHandler callback)
{
  int fd;                  /* file descriptor to wait on         */
  struct timeval timeLeft; /* amount of time until timeout       */
  struct timeval now;      /* current time on each loop          */
  struct timeval until;    /* time to return at if still waiting */
  XEvent event;            /* event received from server         */

/*
  *  Continually receive events from the X server and pass them to the callback
  *  until either the timeout is reached or the callback returns False.
  */
  
  if (timeout <= 0)
  {
    return;
  }
  fd = ConnectionNumber(d);
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(fd, &fds);
  
  timeLeft.tv_sec = (int) timeout;
  timeLeft.tv_usec = (int) ((timeout - timeLeft.tv_sec) * 1000000);
  
  gettimeofday(&now, NULL);
  until.tv_sec = now.tv_sec + timeLeft.tv_sec;
  until.tv_usec = now.tv_usec + timeLeft.tv_usec;
  if (until.tv_usec >= 1000000)
  {
    until.tv_usec -= 1000000;
    until.tv_sec += 1;
  }
  
  while (1) {
    if (XPending(d) || select(fd+1, &fds, NULL, NULL, &timeLeft)) {
      XNextEvent(d, &event);
      if(!callback(d, &event))
      {
        break;
      }
    }
    gettimeofday(&now, NULL);
    timeLeft.tv_usec = until.tv_sec - now.tv_sec;
    timeLeft.tv_usec = until.tv_usec - now.tv_usec;
    if (timeLeft.tv_usec < 0)
    {
      if (timeLeft.tv_sec <= 0)
      {
        break;
      }
      timeLeft.tv_usec += 1000000;
      timeLeft.tv_sec -= 1;
    }
  }
}

/*
*  Event handler used to receive messages while running. Invokes actions based
*  on request type and sends a response for each indicating success or failure
*/
Bool
handleRequest(Display* d, XEvent* event)
{  
  Window root;          /* as it says        */
  response response;    /* response type to send   */
  XEvent responseEvent = { 0 }; /* event sent to requester */
  
  printf("Received event\n");
  

  if (event->type == ClientMessage
    && event->xclient.message_type == messageRequest) {
    message request = event->xclient.data.l[0];
    root = RootWindowOfScreen (ScreenOfDisplay (d, 0));
    printf("Event is request %d\n", request);
    fullResponse* responseBody = (fullResponse*) &responseEvent.xclient.data;
    switch (request)
    {
      case msg_disable:
       disableByMessage (d, root, responseBody);
      break;

      case msg_enable:
       enableByMessage (d, root, responseBody);
      break;

      case msg_toggle:
       toggleByMessage (d, root, responseBody);
      break;

      case msg_lockNow:
       lockNowByMessage (d, root, responseBody);
      break;

      case msg_unlockNow:
       unlockNowByMessage (d, root, responseBody);
      break;

      case msg_restart:
       restartByMessage (d, root, responseBody);
      break;

      case msg_exit:
       exitByMessage (d, root, responseBody);
      break;
      
      case msg_isDisabled:
        isDisabledMessage (d, root, responseBody);
      break;

      default:
      /* unknown message, ignore silently */
       responseBody->type = response_none;
      break;
    }
    if (responseBody->type != response_none)
    {
      printf("Sending response, type = %d, data[0] = %d\n", responseBody->type, responseBody->data[0]);
      responseEvent.type = ClientMessage;
      responseEvent.xclient.display = d;
      responseEvent.xclient.message_type = messageResponse;
      responseEvent.xclient.format = 32;
      XSendEvent(d, event->xclient.window, False, 0, &responseEvent);
    }
  }
  return True;
}

/*
*  Event handler used to receive response to sent request. Upon receiving a
*  response it exits with the response status
*/
Bool
handleResponse(Display* d, XEvent* event)
{
  
  if (event->type == ClientMessage
    && event->xclient.message_type == messageResponse) {
    fullResponse* response = (fullResponse*) &event->xclient.data;
    printf("Received response, type = %d, data[0] = %d\n", response->type, response->data[0]);
    switch(response->type)
    {
      case response_success:
        exit(EXIT_SUCCESS);
      break;
      
      case response_failure:
        error0("The operation was denied by the target instance. "
          "It may be in secure mode.\n");
        exit(EXIT_FAILURE);
      break;
      
      case response_bool:
        if (response->data[0])
        {
          printf("true\n");
        }
        else
        {
          printf("false\n");
        }
        exit(EXIT_SUCCESS);
      break;
      
      default:
        exit(EXIT_FAILURE);
    }
  }
  else
  {
    return True;
  }
}

void
lookForMessages(Display* d, double timeout){
    eventListen(d, timeout, handleRequest);
}
    
/*
*  Function for creating the communication atoms.
*/
void
getAtoms (Display* d)
{
  char* sem; /* semaphore property name */
  char* req; /* request property name   */
  char* rsp; /* response property name  */
  char* ptr; /* iterator                */

  sem = newArray (char, strlen (progName) + strlen (SEM_PID) + strlen(id) + 1);
  (void) sprintf (sem, "%s%s%s", progName, SEM_PID, id);
  for (ptr = sem; *ptr; ++ptr) *ptr = (char) toupper (*ptr);
  semaphore = XInternAtom (d, sem, False);
  free (sem);

  req = newArray (char, strlen (progName) + strlen (MESSAGE_REQUEST) + 1);
  (void) sprintf (req, "%s%s", progName, MESSAGE_REQUEST);
  for (ptr = req; *ptr; ++ptr) *ptr = (char) toupper (*ptr);
  messageRequest = XInternAtom (d, req, False);
  free (req);

  rsp = newArray (char, strlen (progName) + strlen (MESSAGE_RESPONSE) + 1);
  (void) sprintf (rsp, "%s%s", progName, MESSAGE_RESPONSE);
  for (ptr = rsp; *ptr; ++ptr) *ptr = (char) toupper (*ptr);
  messageResponse = XInternAtom (d, rsp, False);
  free (rsp);
}

/*
*  Function for finding out whether another xautolock is already 
*  running and for sending it a message if that's what the user
*  wanted.
*/
void
checkConnectionAndSendMessage (Display* d, Window w)
{
  pid_t         pid;      /* as it says               */
  Window        root;     /* as it says               */
  Atom          type;     /* actual property type     */
  int           format;   /* dummy                    */
  unsigned long nofItems; /* dummy                    */
  unsigned long after;    /* dummy                    */
  Window*       contents; /* semaphore property value */
  XEvent        request;  /* event containing message */
  XClassHint    hint;     /* used to verify window    */

  getAtoms (d);

  root = RootWindowOfScreen (ScreenOfDisplay (d, 0));

  (void) XGetWindowProperty (d, root, semaphore, 0L, 2L, False,
                            AnyPropertyType, &type, &format,
          &nofItems, &after,
                            (unsigned char**) &contents);
  printf("Entered cCAMS. Message = %d\n", messageToSend);

  if (type == XA_INTEGER)
  {
    int status = XGetClassHint(d, (Window) *contents, &hint);
//     printf("[%d]\n", strlen(hint.res_class));
    if (status == BadWindow || strcmp(hint.res_class, APPLIC_CLASS) != 0)
    {
      if (messageToSend)
      {
        error2 ("No %s with window ID %d, or the process "
                "is owned by another user.\n", progName, (Window) *contents);
        exit (EXIT_FAILURE);
      }
    }
    else if (messageToSend)
    {
      request.type = ClientMessage;
      request.xclient.display = d;
      request.xclient.window = w;
      request.xclient.message_type = messageRequest;
      request.xclient.format = 32;
      request.xclient.data.l[0] = messageToSend;
      XSendEvent(d, (Window) *contents, False, 0, &request);
      eventListen(d, 1, handleResponse);
      exit (EXIT_SUCCESS);
    }
    else
    {
        error2 ("%s is already running. You may need to use a different ID.\n"
                , progName, (Window) *contents);
        exit (EXIT_FAILURE);
    }
  }
  else if (messageToSend)
  {
    error1 ("Could not locate a running %s.\n", progName);
    exit (EXIT_FAILURE);
  }

  (void) XChangeProperty (d, root, semaphore, XA_INTEGER, 8, 
                          PropModeReplace, (unsigned char*) &w,
        (int) sizeof (w));

  (void) XFree ((char*) contents);
}

