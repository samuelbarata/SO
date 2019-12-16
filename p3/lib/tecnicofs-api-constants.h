/* tecnicofs-api-constants.h */
#ifndef TECNICOFS_API_CONSTANTS_H
#define TECNICOFS_API_CONSTANTS_H

typedef enum permission { NONE, WRITE, READ, RW } permission;
						//	0	  1		2	 3

/* Client already has an open session with a TecnicoFS server */
#define TECNICOFS_ERROR_OPEN_SESSION -1
/* Doesn't exist an open session */
#define TECNICOFS_ERROR_NO_OPEN_SESSION -2
/* Communication failed */
#define TECNICOFS_ERROR_CONNECTION_ERROR -3
/* Already exists a file with the given name */
#define TECNICOFS_ERROR_FILE_ALREADY_EXISTS -4
/* No file found with the given name */
#define TECNICOFS_ERROR_FILE_NOT_FOUND -5
/* Client doesn't have permissions for the operation */
#define TECNICOFS_ERROR_PERMISSION_DENIED -6
/* Number of open files that can be open has been reached */
#define TECNICOFS_ERROR_MAXED_OPEN_FILES -7
/* File is not open */
#define TECNICOFS_ERROR_FILE_NOT_OPEN -8
/* File is open */
#define TECNICOFS_ERROR_FILE_IS_OPEN -9
/* File is open in the a mode that allows the operation */
#define TECNICOFS_ERROR_INVALID_MODE -10
/* Generic error */
#define TECNICOFS_ERROR_OTHER -11
/* Permission is not a valid permission */
#define TECNICOFS_ERROR_INVALID_PERMISSION	-11
/* The given file descriptor is not valid */
#define TECNICOFS_INVALID_FD				-11
/* The FS has no space for more files */
#define TECNICOFS_ERROR_MAXED_FILES			-11
/* The Open file was renamed and no longer exists */
#define TECNICOFS_ERROR_FILE_RENAMED		-5
/* The given command is not valid */
#define TECNICOFS_ERROR_INVALID_COMMAND		-11


#define TECNICOFS_DEFAULT_NO_ERROR			-69

#endif /* TECNICOFS_API_CONSTANTS_H */
