FTP Client-Server Implementation
===========
Simple implementation of a file transfer program. It includes custom client and server programs that provide the ability to authenticate users, list remote files, download remote files, and upload local files to remote locations.

### Directory layout:
	Linux_FTP/
		client/
			ftpclient.cpp
			ftpclient.h
			makefile
		common/
			common.cpp
			common.h
		serve/
			ftpserve.cpp
			ftpserve.h
			makefile

###Usage
To compile and link ftpserve:
```
	$ cd serve/
	$ make
```

To compile and link ftpclient:
```
	$ cd client/
	$ make
```

To run ftpserve:
```
	$ serve/ ./ftpserve
```

To run ftpclient:
```
	$ client/ ./ftpclient servr_IP

	Commands:
		list
		get <filename>
		put <filename>
		quit
```

Available commands:
```
list            - retrieve list of files in the current remote directory
get <filename>  - get the specified file
put <filename>  - Upload the specified file
quit            - end the ftp session
```

Logging In:
```
	Name: anonymous
	Password: [empty]
```
