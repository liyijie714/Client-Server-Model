* This is a course project. I didn’t list the steps to run the clint and server as it is supposed to run on some server not locally.

Test procedure:
(I) The Ping-Pong client and server:
     Concurrency: two laptops were used to connect the runing server on clear at the same time without any problem.	
     Big data: In the client.c program, data are generated as the form of circulation from letter 'a' to   
     letter 'z'. After receiving the echo data from server, the client.c program will compare the received message with the generated sending  message. If two message are different, mismatching error will be printed out.

test cases: 
	1.	Run server on clear
	2.	Two computers with Unix system and iOS system connect to the server.
	3.	Send small data (e.g. 10 bytes) concurrently with any times (1<=count<=10,000); works fine.
	4.	Send big data (e.g. 60000 bytes) concurrently with any times (1<=count<=10,1000);
	works fine.

Special cases:
		Due to the unstability of the internet, the received data may not match the original-sent data. In this case, client will abort the connection with server without effecting other clients

(II) The Web Server:
	Under “www” mode, the server can serve static content to client. Our server only handles “txt/html” file and serve all other file type as “txt/plain”. 
	The error_handler can generate error message for error 400, 404, 500 and 501 with error number, a short message, and a long message.

400: Bad Request. If the uri containes ../, this error will be printed out.
404: Not Found. This error happens when the requested file cannot be found.
500: Internal Server Error. This happens when server could not send datat to client.
501: Not Implemented. Since the server.c program only handles GET request, all other methods will be dealt as 501 error.

test cases:
	1.	Two laptops were used to connecting the server through browser simultaneously.
	2.	If “/” is the last character of URI, home.html will be appended to the uri automatically and the server will serve the home.html as the default home page.
	3.	Simple .html file and .txt file were opend successfully through browser.
	4.	The “../” 400 error was tested using telnet command line because the browser will ignore it automatically. 501 error was also tested using telnet command line. 404 error was tested through both browser and telnet command line. We have not find 500 erro during our tests which shows the stability of our server. 
