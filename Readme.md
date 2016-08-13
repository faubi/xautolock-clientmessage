xautolock-clientmessage
-

This is a modified version of [xautolock](http://www.freecode.com/projects/xautolock) with a few improvements made to it:

* It adds an `-id` option allowing multiple instances to be run simultaneously with different ids
* An `-isdisabled` option has also been added, allowing the enabled/disabled status of a running instance to be queried. Additionally, options which send messages (e.g. `-toggle` and `-locknow`) will now have exit status 0 if successful and 1 if unsuccessful.
* It now catches SIGINT and SIGTERM signals and terminates gracefully.

In order to accomplish the second change above, the messaging system was rewritten to use X11 ClientMessage events (thus the name).

Note that this has only been run/tested on a recent version of GNU/Linux, so although it might be, I can't guarentee that it is still compatible with other platforms.

(This readme file only pertains to the modifications; for information on the application itself see the [original readme](/Readme).)
