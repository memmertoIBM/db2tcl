/* $Id$ */

#ifndef DB2TCLCMDS_H
#define DB2TCLCMDS_H

#include <tcl.h>
#include <sqlcli.h>

/* Registered Tcl functions */

typedef struct FieldBuffer { 
  /* description of the field */
  SQLSMALLINT dbtype;
  SQLCHAR    *cbuf;           /* ptr to name of select-list item */
  SQLSMALLINT cbufl;          /* length of select-list item name */
  SQLINTEGER  dsize;          /* max display size if field is a SQLCHAR */
  SQLUINTEGER prec;
  SQLSMALLINT scale;
  SQLSMALLINT nullok;

  /* Our storage space for the field data as it's fetched */
  SQLSMALLINT ftype;          /* external datatype we wish to get             */
  short       indp;           /* null/trunc indicator variable                */
  void       *buffer;         /* data buffer (poSQLINTEGERs to sv data)       */
  SQLINTEGER  bufferSize;     /* length of data buffer                        */
  SQLINTEGER  rlen;           /* length of returned data                      */
} FieldBuffer;

typedef struct Db2Connection
{
    char id[18];
    char database[18];
    char user[18];
    char password[18];

    SQLHANDLE hdbc;
    
    SQLCHAR sql_state[5];
    SQLINTEGER native_error;
    SQLCHAR error_msg[SQL_MAX_MESSAGE_LENGTH + 1];
    SQLSMALLINT size_error_msg;
    SQLRETURN rc;

} Db2Connection;

extern int Db2_connect(
		ClientData cData, 
		Tcl_Interp *interp, 
		int argc, 
		char *argv[]);

extern int Db2_disconnect(
		ClientData cData, 
		Tcl_Interp *interp, 
		int argc, 
		char *argv[]);

extern int Db2_exec(
		ClientData cData, 
		Tcl_Interp *interp, 
		int argc, 
		char *argv[]);

extern int Db2_select(
		ClientData cData, 
		Tcl_Interp *interp, 
		int argc, 
		char *argv[]);

extern int Db2_finish(
		ClientData cData, 
		Tcl_Interp *interp, 
		int argc, 
		char *argv[]);

extern int Db2_fetchrow(
		ClientData cData, 
		Tcl_Interp *interp, 
		int argc, 
		char *argv[]);

#endif	 /* DB2TCLCMDS_H */
