/* $Id$ */

#include "db2tclcmds.h"

#include <string.h>

static SQLHANDLE henv = SQL_NULL_HANDLE;
static int num_connect = 0;

int Db2DelConnection (ClientData cData, Tcl_Interp * interp)
{
    Db2Connection *conn;

    conn = (Db2Connection *) cData;

    /* Disconnect from database */
    conn->rc = SQLDisconnect (conn->hdbc);

    if (conn->rc != SQL_SUCCESS)
    {
	SQLError (henv, conn->hdbc, SQL_NULL_HANDLE,
		  (SQLCHAR *) & conn->sql_state,
		  &conn->native_error,
		  (SQLCHAR *) & conn->error_msg,
		  sizeof (conn->error_msg), &conn->size_error_msg);
	Tcl_AppendResult (interp, conn->error_msg, 0);
	return TCL_ERROR;
    }

    /* Free unused handles */
    SQLFreeHandle (SQL_HANDLE_DBC, conn->hdbc);

    num_connect--;

    if (num_connect == 0)
    {
	SQLFreeHandle (SQL_HANDLE_ENV, henv);
	henv = SQL_NULL_HANDLE;
    }

    Tcl_EventuallyFree ((ClientData) conn, TCL_DYNAMIC);

    return 0;
}

int Db2InputProc (ClientData cData, char *buf, int bufSize, int *errorCodePtr)
{
    Db2Connection *conn;

    conn = (Db2Connection *) cData;

    return 0;
}

int Db2OutputProc (ClientData cData, char *buf, int bufSize,
		   int *errorCodePtr)
{
    Db2Connection *conn;

    conn = (Db2Connection *) cData;

    return 0;
}

Tcl_ChannelType Db2_ConnType = {
    "db2sql",			/* channel type */
    NULL,
    Db2DelConnection,		/* closeproc */
    Db2InputProc,		/* inputproc */
    Db2OutputProc,		/* outputproc */
};

/*
   TCL syntax:

   db2_connect dbname ?username? ?password?

   Return database handle
*/

int Db2_connect (ClientData cData, Tcl_Interp * interp, int argc,
		 char *argv[])
{
    int i;
    Tcl_Channel conn_channel;
    Db2Connection *conn;

    if (argc > 4 || argc < 2)
    {
	Tcl_AppendResult (interp, "Wrong number of arguments", 0);
	return TCL_ERROR;
    }

    conn = (Db2Connection *) malloc (sizeof (Db2Connection));
    memset (conn, '\0', (sizeof (Db2Connection)));

    strncpy (conn->database, argv[1], SQL_MAX_DSN_LENGTH);

    if (argc > 2  && argv[2])
    {
	strncpy (conn->user, argv[2], MAX_UID_LENGTH);
    }

    if (argc > 3 && argv[3])
    {
	strncpy (conn->password, argv[3], MAX_PWD_LENGTH);
    }

    if (henv == SQL_NULL_HANDLE)
    {
	conn->rc = SQLAllocHandle (SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	if (conn->rc != SQL_SUCCESS)
	{
	    SQLError (henv, SQL_NULL_HANDLE, SQL_NULL_HANDLE,
		      (SQLCHAR *) & conn->sql_state,
		      &conn->native_error,
		      (SQLCHAR *) & conn->error_msg,
		      sizeof (conn->error_msg), &conn->size_error_msg);
	    Tcl_AppendResult (interp, conn->error_msg, 0);
	    return TCL_ERROR;
	}
    }

    conn->rc = SQLAllocHandle (SQL_HANDLE_DBC, henv, &conn->hdbc);

    if (conn->rc != SQL_SUCCESS)
    {
	SQLError (henv, conn->hdbc, SQL_NULL_HANDLE,
		  (SQLCHAR *) & conn->sql_state,
		  &conn->native_error,
		  (SQLCHAR *) & conn->error_msg,
		  sizeof (conn->error_msg), &conn->size_error_msg);
	Tcl_AppendResult (interp, conn->error_msg, 0);
	return TCL_ERROR;
    }

    /* Connect to database */
    conn->rc = SQLConnect (conn->hdbc,
			   (SQLCHAR *) & conn->database, SQL_NTS,
			   (SQLCHAR *) & conn->user, SQL_NTS,
			   (SQLCHAR *) & conn->password, SQL_NTS);

    if (conn->rc != SQL_SUCCESS)
    {
	SQLError (henv, conn->hdbc, SQL_NULL_HANDLE,
		  (SQLCHAR *) & conn->sql_state,
		  &conn->native_error,
		  (SQLCHAR *) & conn->error_msg,
		  sizeof (conn->error_msg), &conn->size_error_msg);
	Tcl_AppendResult (interp, conn->error_msg, 0);
	return TCL_ERROR;
    }


    num_connect++;

    snprintf (conn->id, MAX_ID_LENGTH, "db2sql%d", (int) conn->hdbc);

    /* Create TCL channel for read and write */
    conn_channel = Tcl_CreateChannel (&Db2_ConnType,
				      conn->id,
				      (ClientData) conn,
				      TCL_READABLE | TCL_WRITABLE);

    Tcl_SetChannelOption (interp, conn_channel, "-buffering", "line");
    Tcl_RegisterChannel (interp, conn_channel);

    Tcl_SetResult (interp, conn->id, TCL_VOLATILE);

    return TCL_OK;
}

/*
   TCL syntax:

   db2_connect db_handle

   Return database handle
*/

int Db2_disconnect (ClientData cData, Tcl_Interp * interp, int argc,
		    char *argv[])
{
    Tcl_Channel conn_channel;
    char id[MAX_ID_LENGTH + 1];
    short num_fields;
    SQLHANDLE hdbc, hstmt;


    /* db2sqlX.Y.Z where X is database handle, Y is database statement, Z is fields count */

    if (argv[1] && sscanf (argv[1], "db2sql%d.%d.%d", &hdbc, &hstmt, &num_fields) < 1)
    {
	Tcl_AppendResult (interp, argv[1], " is not a valid connection", 0);
	return TCL_ERROR;
    }

    snprintf (id, MAX_ID_LENGTH, "db2sql%d", (int) hdbc);

    conn_channel = Tcl_GetChannel (interp, id, 0);

    if (conn_channel == NULL)
    {
	Tcl_AppendResult (interp, argv[1], " is not a valid connection", 0);
	return TCL_ERROR;
    }

    Tcl_ResetResult (interp);

    return Tcl_UnregisterChannel (interp, conn_channel);
}

/*
    TCL syntax:

    db2_exec handle sql_code

    Return database handle
*/

int Db2_exec (ClientData cData, Tcl_Interp * interp, int argc, char *argv[])
{
    Tcl_Channel conn_channel;
    Db2Connection *conn;
    SQLHANDLE hstmt;
    char buff[MAX_ID_LENGTH + 1];

    if (argc != 3)
    {
	Tcl_AppendResult (interp, "Wrong number of arguments", 0);
	return TCL_ERROR;
    }

    conn_channel = Tcl_GetChannel (interp, argv[1], 0);

    if (conn_channel == NULL)
    {
	Tcl_AppendResult (interp, argv[1], " is not a valid connection", 0);
	return TCL_ERROR;
    }

    conn = (Db2Connection *) Tcl_GetChannelInstanceData (conn_channel);

    conn->rc = SQLAllocHandle (SQL_HANDLE_STMT, conn->hdbc, &hstmt);
    if (conn->rc != SQL_SUCCESS)
    {
	SQLError (henv, conn->hdbc, hstmt,
		  (SQLCHAR *) & conn->sql_state,
		  &conn->native_error,
		  (SQLCHAR *) & conn->error_msg,
		  sizeof (conn->error_msg), &conn->size_error_msg);
	Tcl_AppendResult (interp, conn->error_msg, 0);
	return TCL_ERROR;
    }

    conn->rc = SQLExecDirect (hstmt, argv[2], SQL_NTS);
    if (conn->rc != SQL_SUCCESS)
    {
	SQLError (henv, conn->hdbc, hstmt,
		  (SQLCHAR *) & conn->sql_state,
		  &conn->native_error,
		  (SQLCHAR *) & conn->error_msg,
		  sizeof (conn->error_msg), &conn->size_error_msg);
	Tcl_AppendResult (interp, conn->error_msg, 0);
	return TCL_ERROR;
    }

    /* Return result by template db2sqlX.Y */
    snprintf (buff, MAX_ID_LENGTH, "%s.%d", conn->id, SQL_NULL_HANDLE);
    Tcl_AppendResult (interp, buff, 0);

    conn->rc = SQLFreeHandle (SQL_HANDLE_STMT, hstmt);
    if (conn->rc != SQL_SUCCESS)
    {
	SQLError (henv, conn->hdbc, hstmt,
		  (SQLCHAR *) & conn->sql_state,
		  &conn->native_error,
		  (SQLCHAR *) & conn->error_msg,
		  sizeof (conn->error_msg), &conn->size_error_msg);
	Tcl_AppendResult (interp, conn->error_msg, 0);
	return TCL_ERROR;
    }

    return TCL_OK;
}

/*
    TCL syntax:

    db2_select handle sql_code

    Return database handle
*/

int Db2_select (ClientData cData, Tcl_Interp * interp, int argc, char *argv[])
{
    Tcl_Channel conn_channel;
    Db2Connection *conn;
    SQLHANDLE hstmt;
    short num_fields;
    char buff[MAX_ID_LENGTH + 1];

    if (argc != 3)
    {
	Tcl_AppendResult (interp, "Wrong number of arguments", 0);
	return TCL_ERROR;
    }

    conn_channel = Tcl_GetChannel (interp, argv[1], 0);

    if (conn_channel == NULL)
    {
	Tcl_AppendResult (interp, argv[1], " is not a valid connection", 0);
	return TCL_ERROR;
    }

    conn = (Db2Connection *) Tcl_GetChannelInstanceData (conn_channel);

    conn->rc = SQLAllocHandle (SQL_HANDLE_STMT, conn->hdbc, &hstmt);
    if (conn->rc != SQL_SUCCESS)
    {
	SQLError (henv, conn->hdbc, hstmt,
		  (SQLCHAR *) & conn->sql_state,
		  &conn->native_error,
		  (SQLCHAR *) & conn->error_msg,
		  sizeof (conn->error_msg), &conn->size_error_msg);
	Tcl_AppendResult (interp, conn->error_msg, 0);
	return TCL_ERROR;
    }

    conn->rc = SQLExecDirect (hstmt, argv[2], SQL_NTS);
    if (conn->rc != SQL_SUCCESS)
    {
	SQLError (henv, conn->hdbc, hstmt,
		  (SQLCHAR *) & conn->sql_state,
		  &conn->native_error,
		  (SQLCHAR *) & conn->error_msg,
		  sizeof (conn->error_msg), &conn->size_error_msg);
	Tcl_AppendResult (interp, conn->error_msg, 0);
	return TCL_ERROR;
    }


    conn->rc = SQLNumResultCols (hstmt, &num_fields);
    if (conn->rc != SQL_SUCCESS)
    {
	SQLError (henv, conn->hdbc, hstmt,
		  (SQLCHAR *) & conn->sql_state,
		  &conn->native_error,
		  (SQLCHAR *) & conn->error_msg,
		  sizeof (conn->error_msg), &conn->size_error_msg);
	Tcl_AppendResult (interp, conn->error_msg, 0);
	return TCL_ERROR;
    }

    snprintf (buff, MAX_ID_LENGTH, "%s.%d.%d", conn->id, hstmt, num_fields);
    Tcl_AppendResult (interp, buff, 0);

    return TCL_OK;
}

/*
    TCL syntax:

    db2_fetchrow handle ?number?

    Return database handle
*/

int Db2_fetchrow (ClientData cData, Tcl_Interp * interp, int argc,
		  char *argv[])
{
    int i = 1;
    int num_col = 0;
    Tcl_Channel conn_channel;
    Db2Connection *conn;
    SQLPOINTER *buff;
    SQLINTEGER size_buff;
    char id[MAX_ID_LENGTH + 1];
    SQLHANDLE hdbc, hstmt;
    short num_fields;

    if (argc > 3 || argc < 2)
    {
	Tcl_AppendResult (interp, "Wrong number of arguments", 0);
	return TCL_ERROR;
    }

    if (sscanf (argv[1], "db2sql%d.%d.%d", &hdbc, &hstmt, &num_fields) < 3)
    {
	Tcl_AppendResult (interp, argv[1], " is not a valid statement", 0);
	return TCL_ERROR;
    }

    snprintf (id, MAX_ID_LENGTH, "db2sql%d", (int) hdbc);

    if (argv[2])
    {
	num_col = atoi (argv[2]);
    }

    conn_channel = Tcl_GetChannel (interp, id, 0);

    if (conn_channel == NULL)
    {
	Tcl_AppendResult (interp, argv[1], " is not a valid connection", 0);
	return TCL_ERROR;
    }

    conn = (Db2Connection *) Tcl_GetChannelInstanceData (conn_channel);

    conn->rc = SQLFetch (hstmt);

    if (conn->rc == SQL_NO_DATA_FOUND)
    {
	return TCL_OK;
    }

    if (conn->rc != SQL_SUCCESS)
    {
	SQLError (henv, conn->hdbc, hstmt,
		  (SQLCHAR *) & conn->sql_state,
		  &conn->native_error,
		  (SQLCHAR *) & conn->error_msg,
		  sizeof (conn->error_msg), &conn->size_error_msg);
	Tcl_AppendResult (interp, conn->error_msg, 0);
	return TCL_ERROR;
    }

    size_buff = 4096;
    buff = (SQLPOINTER *) malloc (size_buff);

    if (num_col)
    {
	conn->rc = SQLGetData (hstmt,
			       num_col, SQL_C_CHAR, buff, size_buff, NULL);

	Tcl_AppendElement (interp, (char *) buff);
	//Tcl_AppendResult(interp, (char *)buff, " ", 0);
    }
    else
    {
	for (i = 0; i < num_fields; i++)
	{
	    conn->rc = SQLGetData (hstmt,
				   i + 1, SQL_C_CHAR, buff, size_buff, NULL);

	    Tcl_AppendElement (interp, (char *) buff);
	    //Tcl_AppendResult(interp, (char *)buff, " ", 0);
	}
    }

    free (buff);

    return TCL_OK;
}

/*
    TCL syntax: 

    db2_exec handle sql_code

    Return database handle
*/

int Db2_finish (ClientData cData, Tcl_Interp * interp, int argc, char *argv[])
{
    Tcl_Channel conn_channel;
    Db2Connection *conn;
    char id[MAX_ID_LENGTH + 1];
    SQLHANDLE hdbc, hstmt;
    short num_fields;
    char *p;

    if (sscanf (argv[1], "db2sql%d.%d.%d", &hdbc, &hstmt, &num_fields) < 3)
    {
	Tcl_AppendResult (interp, argv[1], " is not a valid statement", 0);
	return TCL_ERROR;
    }

    snprintf (id, MAX_ID_LENGTH, "db2sql%d", (int) hdbc);

    conn_channel = Tcl_GetChannel (interp, id, 0);

    if (conn_channel == NULL)
    {
	Tcl_AppendResult (interp, argv[1], " is not a valid connection", 0);
	return TCL_ERROR;
    }

    conn = (Db2Connection *) Tcl_GetChannelInstanceData (conn_channel);

    conn->rc = SQLFreeHandle (SQL_HANDLE_STMT, hstmt);

    if (conn->rc != SQL_SUCCESS)
    {
	SQLError (henv, conn->hdbc, hstmt,
		  (SQLCHAR *) & conn->sql_state,
		  &conn->native_error,
		  (SQLCHAR *) & conn->error_msg,
		  sizeof (conn->error_msg), &conn->size_error_msg);
	Tcl_AppendResult (interp, conn->error_msg, 0);
	return TCL_ERROR;
    }

    return TCL_OK;
}

/*
    TCL syntax: 

    db2_getnumrows handle

    Return number of columns in query
*/

int Db2_getnumrows (ClientData cData, Tcl_Interp * interp, int argc,
		    char *argv[])
{
    SQLHANDLE hdbc, hstmt;
    short num_fields;
    char buff[32];

    if (sscanf (argv[1], "db2sql%d.%d.%d", &hdbc, &hstmt, &num_fields) < 3)
    {
	Tcl_AppendResult (interp, argv[1], " is not a valid statement", 0);
	return TCL_ERROR;
    }

    snprintf (buff, 32, "%d", num_fields);

    Tcl_AppendResult (interp, buff, 0);

    return TCL_OK;
}

/*
    db2tcl super command
*/

int Db2_db2 (ClientData cData, Tcl_Interp * interp, int argc, char *argv[])
{
    if (argc < 2 || argv[1] == NULL )
    {
	Tcl_AppendResult (interp, "Wrong number of arguments", 0);
	return TCL_ERROR;
    }

    if (strncmp (argv[1], "fetchrow", 8) == 0)
    {
	Db2_fetchrow (cData, interp, argc - 1, argv + 1);
    }
    else if (strncmp (argv[1], "select", 6) == 0)
    {
	Db2_select (cData, interp, argc - 1, argv + 1);
    }
    else if (strncmp (argv[1], "finish", 6) == 0)
    {
	Db2_finish (cData, interp, argc - 1, argv + 1);
    }
    else if (strncmp (argv[1], "exec", 4) == 0)
    {
	Db2_exec (cData, interp, argc - 1, argv + 1);
    }
    if (strncmp (argv[1], "connect", 7) == 0)
    {
	Db2_connect (cData, interp, argc - 1, argv + 1);
    }
    else if (strncmp (argv[1], "disconnect", 10) == 0)
    {
	Db2_disconnect (cData, interp, argc - 1, argv + 1);
    }
    return TCL_OK;
}

int Db2_test (ClientData clientData, Tcl_Interp * interp, int objc, Tcl_Obj *CONST objv[])
{
    int i;
    printf("db2_test\n");
    for(i=0; i < objc; i++)
    {
	printf(" objv[%d] = %s\n", i, Tcl_GetStringFromObj(objv[i], NULL));
    }
    return 0;
}