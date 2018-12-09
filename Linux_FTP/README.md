FTP Client-Server Implementation
===========
Simple implementation of a file transfer program. It includes custom client and server programs that provide the ability to authenticate users, list remote files, download remote files, and upload local files to remote locations.

### Directory layout:
	Linux_FTP/
		client/
			ftclient.c
			ftclient.h
			makefile
		common/
			common.c
			common.h
		server/
			ftserve.c
			ftserve.h
			makefile

###Usage
To compile and link ftserve:
```
	$ cd server/
	$ make
```

To compile and link ftclient:
```
	$ cd client/
	$ make
```

To run ftserve:
```
	$ server/ ./ftpserve
```

To run ftclient:
```
	$ client/ ./ftpserve servr_IP

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
