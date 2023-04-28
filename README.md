# POP3-Server

## Learning Goal
- To learn how to program with TCP sockets in C;
- To learn how to read and implement a well-specified protocol;
- To develop your programming and debugging skills as they relate to the use of sockets in C;
- To implement the server side of a protocol;
- To develop general networking experimental skills;
- To develop a further understanding of what TCP does, and how to manage TCP connections from the server perspective.

## Overview
In this assignment you will use the Unix Socket API to construct a POP3 server, used for receiving emails. The executable for this server will be called mypopd. Your program is to take a single argument, the TCP port the server is to listen on for client connections.

To start your assignment, download the file pa_pop3.zip. This file contains the base files required to implement your code, as well as helper functions and a Makefile.

Some code is already provided that checks for the command-line arguments, listens for a connection, and accepts a new connection. Servers written in C that need to handle multiple clients simultaneously use the fork system call to handle each accepted client in a separate process, which means your system will naturally be able to handle multiple clients simultaneously without interfering with each other. However, debugging server processes that call fork can be challenging (as the debugger normally sees the parent process and all the interesting activity happens in the child). To support this, the provided code disables the call to fork and executes all code in a single process unless you define the preprocessor symbol DOFORK. We strongly suggest that you define the symbol DOFORK only when your server is working perfectly without it.

Additionally, some provided functions also handle specific functionality needed for this assignment, including the ability to read individual lines from a socket and to handle usernames, passwords and email files.

## POP3 Server
The POP3 protocol, described in RFC 1939, is used by mail clients to retrieve email messages from the mailbox of a specific email user. You will implement the server side of this protocol. More specifically, you are required to support, at the very least, the following commands:

- USER and PASS (plain authentication)
- STAT (message count information)
- LIST (message listing) - with and without arguments
- RETR (message content retrieval)
- DELE (delete message)
- RSET (undelete messages)
- NOOP (no operation)
- QUIT (session termination)

The functionality of the commands above is listed in the RFC. Since this is a short RFC you are encouraged to read the entire document, but the most important sections are 1-6 and 9-11. You must also pay special attention to possible error messages, and make sure your code sends the correct error messages for different kinds of invalid input, such as invalid command ordering, missing or extraneous parameters, or invalid recipients. You are not required to implement other commands. You are also not required to implement encryption.

You only need to implement plain user authentication, with no password encryption or digest. User and password checks can be performed using provided functions, based on the information in file users.txt described above.

A user's messages will be retrieved from a subdirectory of mail.store, with the subdirectory name corresponding to the user name. This is the same structure as the one used to store messages in the SMTP server; this allows you to use your SMTP server to add messages to a user's mailbox, and POP3 to retrieve these messages. Your program is to read those messages and return them to the user as requested. Most of the functionality to read the list of files, obtain their size and store their reference in memory is already implemented in functions provided in file mailuser.c.

Note that, although typical clients retrieve messages in a straightforward order, you must allow the user to provide commands in an order that is different than a regular client, as long as allowed by the RFC. In particular, note that the DELE command, which deletes a message, does not actually delete it right away, but only marks it as deleted. Your code must be able to, for example, list all remaining messages after a deletion by ignoring the deleted message while still listing other messages with the same numeric order as they had before the deletion. The email file will, then, be deleted once the session terminates.

Note that the autograder expects the LIST command with no arguments to produce a reply that matches the examples in the RFC and includes the number of messages as the first thing after the +OK response. The response line should look like:

+OK n messages (m octets)

where n is the number of messages in the mailbox. n should be a non-negative number.

Note that the autograder expects the RSET command to produce a reply that includes the number of messages that have been restored as the first thing after the +OK response. The response line should look like:

+OK n messages restored

where n is the number of messages that have been restored (or un-deleted) as a result of the reset. n should be a non-negative number.

## Implementation Constrains and Suggestions
The provided code already provides functionality for several features you are expected to perform. You are strongly encouraged to read the comments for the provided functions before starting the implementation, as part of what you are expected to implement can be vastly simplified by using the provided code. In particular, note functions like:

**send_formatted**
sends a string to the socket and allows you to use printf-like format directives to include extra parameters in your call
**nb_read_line**
reads from the socket and buffers the received data, while returning a single line at a time (similar to fgets)
**dlog**
print a log message to the standard error stream, using printf-like formatting directives. You can turn on and off logging by assigning the variable be_verbose
**split**
split a line into parts that are separated by white space
You may assume that the functionality of these functions will be available and unmodified in the grading environment.

The provided code includes in mypopd.c the skeleton of a structure that you may find helpful to direct your efforts. You do not need to keep this skeleton and should feel free to change it as much as necessary. It is provided in the hopes that more direction will be valuable. Do remember that all of your changes must be confined to the mypopd.c file since that is the only file that you will submit for grading.

Don't try to implement this assignment all at once. Instead incrementally build and simultaneously test the solution. A suggested strategy is to:

Start by reading the RFC for the POP3 protocol, and listing a "normal" scenario, where a client has one message which is listed and retrieved. Make note of the proper sequence of commands and replies to be used for each of these scenarios.
Have your program send the initial welcome message and immediately return.
Get your server to start reading and parsing commands and arguments. You may find library routines like strcasecmp(), strncasecmp(), and strchr() useful. At this point just respond with a message indicating that the command was not recognized.
Implement simple commands like QUIT and NOOP.
Implement a straightforward sequence of commands as listed in your first item. Start simple, then move on to more complex tasks.
Finally, identify potentially incorrect sequences of commands or arguments, and handle them accordingly. Examples may include specifying a password without a username, obtaining the list of messages without logging in first, etc.

## Test
For testing purposes you can use netcat (with the -C option). In particular, netcat can be very useful to test simple cases, unusual sequences of commands (e.g., a "LIST" command before a "USER" command), or incorrect behaviour. Make sure you test the case where multiple clients are connected simultaneously to your server. Also, test your solution in mixed case (e.g., QUIT vs quit).

If you are testing your server in a department computer, with a client in your own computer, you may need to connect to UBC's VPN, since a firewall may block incoming connections to the department computers that come from outside a UBC network.

The starter code includes a script test.sh that will run some simple tests. The input for these tests are in files named in.p.[0-9], the starting mail.store directory is in in.p.[0-9].mail.store, the expected output is in files named exp.p.[0-9], and the expected contents of the mail.store are in the directory named exp.p.[0-9].mail.store. This script should run on macOS and Linux and under the Linux subsystem for Windows (WSL).
