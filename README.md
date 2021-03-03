# FTP-Daniel-Aisha
*Building an application protocol that runs on top of the sockets.*

## Description

This project implements a client-server FTP system in C.

### How to run

For the server, run <code>gcc -o FTPserver FTPserver.c && ./FTPserver</code><br>

For the client, do <code>gcc -o FTPclient FTPclient.c && ./FTPclient 127.0.0.1 9000</code>. 
This will open the client instance and try to connect to local host, which has
IP address 127.0.0.1, on port 9000, which is the server's default port.

## Checkpoint 1

At this stage of the project, we have implemented the USER, PASS and QUIT 
commands in the *FTPclient.c*. The client can connect to the server and do 
basic authentication, but none of the file transfer is yet implemented.

To test checkpoint one, run the server with: <br>
<code>gcc -o FTPserver FTPserver.c && ./FTPserver</code><br>
And then run the client with <br>
<code>gcc -o FTPclient FTPclient.c && ./FTPclient 127.0.0.1 9000</code><br>
After this, authenticate with one of the following user-password pairs:<br>
<ul>
    <li><code>USER Aisha PASS AHPass</code></li>
    <li><code>USER Daniel PASS DBPass</code></li>
</ul>



RSA implementation:
http://www.trytoprogram.com/c-examples/c-program-to-encrypt-and-decrypt-string/
