# FORTE HTTP Com Layer
Simple HTTP Com Layer for 4diac-RTE (FORTE)
​
# Requires
* 4diac-RTE (https://www.eclipse.org/4diac/)

# Installation
* Add the folder containing the source files to the FORTE modules directory (src/modules/)
* In CMake GUI, enable FORTE_COM_HTTP
​
# Documentation
* Currently, only HTTP GET requests are supported. PUT requests are in the works.
* For GET requests, use a CLIENT_0_1 function block.
* For PUT requests, use a CLIENT_1_0 or CLIENT_1 function block (not yet supported!)
* Other function blocks are currently not supported and may result in unexpected behaviour.

# Parameters
http[ip:port/path]
* ip: The IP address
* port: The port number
* path: The path in the URL

example: http[144.12.131.2:80/rest/battery/voltage]

# Notes
* The project is still in its early development stages. The interface is subject to change.
* A CHttpParser class is used to parse the HTTP requests and responses in the communication layer.
  It was designed with the goal of requesting measurements and setting values via a REST server.
  Thus, it is assumed that a number is returned for the GET request and a number is sent for the PUT request.
  To adjust the parsing of requests and responses, change the CHttpParser class accordingly.
* In the long run, a factory for the CHttpParser may be implemented.