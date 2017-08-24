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
* Currently, only HTTP GET and PUT requests are supported.
* For GET requests, use a CLIENT_0_1 function block.
* For PUT requests, use a CLIENT_1_0 or CLIENT_1 function block
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
* The following IEC 61499 data types are currently NOT supported for PUT requests: DATE, TIME_OF_DAY, arrays, structs
  

By default, a GET request is sent as: <br />
"GET /path HTTP/1.1 <br />
Host: ip:port" <br />
<br />
a PUT request is sent as: <br />
"PUT /path HTTP/1.1 <br />
Host: ip:port <br />
Content-type: text/html <br />
Content-length: *length_of_data_string* <br />
<br />
<br />
data_string"

