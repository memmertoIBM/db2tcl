/* $Id$ */

#include "db2tclcmds.h"

#include <string.h>

static SQLHANDLE henv = SQL_NULL_HANDLE;
static int num_connect = 0;

/*---------------------------------------*/
int Db2DelConnection(ClientData cData, Tcl_Interp *interp)
{
    Db2Connection *conn;

    conn = (Db2Connection*)cData;

    /* Disconnect from database */
    conn->rc = SQLDisconnect(conn->hdbc);

    if (conn->rc != SQL_SUCCESS)
    {                 
        SQLError(henv, conn->hdbc, SQL_NULL_HANDLE,                 
        (SQLCHAR *)&conn->sql_state,                   
        &conn->native_error,                           
        (SQLCHAR *)&conn->error_msg,                   
        sizeof(conn->error_msg),                       
        &conn->size_error_msg);                        
        Tcl_AppendResult(interp, conn->error_msg, 0);           
        return TCL_ERROR;                                       
    }

    /* Free all handles */
    SQLFreeHandle(SQL_HANDLE_DBC, conn->hdbc);

    num_connect--;

    if (num_connect == 0)
    {
	SQLFreeHandle(SQL_HANDLE_ENV, henv);
	henv = SQL_NULL_HANDLE;
    }

    Tcl_EventuallyFree((ClientData) conn, TCL_DYNAMIC);

    return 0;
}

int
Db2InputProc(ClientData cData, char *buf, int bufSize, int *errorCodePtr)
{
    Db2Connection *conn;

    conn = (Db2Connection*)cData;

    return 0;
}

int
Db2OutputProc(ClientData cData, char *buf, int bufSize, int *errorCodePtr)
{
    Db2Connection *conn;

    conn = (Db2Connection*)cData;

    return 0;
}

/*---------------------------------------*/

Tcl_ChannelType Db2_ConnType = {
        "db2sql",                      /* channel type */
	NULL,
        Db2DelConnection,              /* closeproc */
        Db2InputProc,                  /* inputproc */
        Db2OutputProc,                 /* outputproc */
};

/*
**
**   Syntax:
**
**   db2_connect dbname [[-user] username] [[-password] password]]
**
**   Return database handle
*/

int Db2_connect(
		ClientData cData, 
		Tcl_Interp *interp, 
		int argc, 
		char *argv[])
{
    int i;
    Tcl_Channel conn_channel;
    Db2Connection *conn;

    if (argc == 1)
    {
    	Tcl_AppendResult(interp, "db2_connect: database name missing\n", 0);
	return TCL_ERROR;
    }
    
    if (argc > 6)
    {
    	Tcl_AppendResult(interp, "Wrong number arguments to db2_connect\n", 0);
	return TCL_ERROR;
    }

    conn = (Db2Connection*) malloc(sizeof(Db2Connection));
    memset(conn, '\0', (sizeof(Db2Connection)));

    strncpy(conn->database, argv[1], 18);

    if (argc > 2)
    {
    	i = 2;
	while (i + 1 < argc)
	{
	    if(strcmp(argv[i], "-user") == 0)
	    {
	        strncpy(conn->user, argv[i + 1], 18);
		i += 2;
	    }
	    else if (strcmp(argv[i], "-password") == 0)
	    {
	        strncpy(conn->password, argv[i + 1], 18);
		i += 2;
	    }
	    else
	    {
	        Tcl_AppendResult(interp, "Bad option to db2_connect: ", argv[i], 0);
	    	return TCL_ERROR;
	    }
	}
    }

    if (henv == SQL_NULL_HANDLE)
    {
	conn->rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    
        if (conn->rc != SQL_SUCCESS)
        {
            SQLError(henv, SQL_NULL_HANDLE, SQL_NULL_HANDLE,
            (SQLCHAR *)&conn->sql_state,
            &conn->native_error,
            (SQLCHAR *)&conn->error_msg,
            sizeof(conn->error_msg),
            &conn->size_error_msg);
            Tcl_AppendResult(interp, conn->error_msg, 0);
            return TCL_ERROR;
        }
    }
    
    conn->rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, &conn->hdbc);
    if (conn->rc != SQL_SUCCESS)
    {
        SQLError(henv, conn->hdbc, SQL_NULL_HANDLE,
        (SQLCHAR *)&conn->sql_state,
        &conn->native_error,
        (SQLCHAR *)&conn->error_msg,
        sizeof(conn->error_msg),
        &conn->size_error_msg);
        Tcl_AppendResult(interp, conn->error_msg, 0);
        return TCL_ERROR;
    }
 
    conn->rc = SQLConnect(conn->hdbc,
                    (SQLCHAR *)&conn->database, SQL_NTS,
                    (SQLCHAR *)&conn->user, SQL_NTS,
                    (SQLCHAR *)&conn->password, SQL_NTS);

    if (conn->rc != SQL_SUCCESS)
    {
        SQLError(henv, conn->hdbc, SQL_NULL_HANDLE,
        (SQLCHAR *)&conn->sql_state,
        &conn->native_error,
        (SQLCHAR *)&conn->error_msg,
        sizeof(conn->error_msg),
        &conn->size_error_msg);
        Tcl_AppendResult(interp, conn->error_msg, 0);
        return TCL_ERROR;
    }
 

    num_connect++;

    sprintf(conn->id, "db2sql%d", (int)conn->hdbc);

    conn_channel = Tcl_CreateChannel(&Db2_ConnType, 
				    conn->id, 
				    (ClientData) conn,
                                    TCL_READABLE | TCL_WRITABLE);

    Tcl_SetChannelOption(interp, conn_channel, "-buffering", "line");
    Tcl_RegisterChannel(interp, conn_channel);

    Tcl_SetResult(interp, conn->id, TCL_VOLATILE);

    return TCL_OK;
}

int Db2_disconnect(ClientData cData, 
		   Tcl_Interp *interp, 
		   int argc, 
		   char *argv[])
{
    Tcl_Channel conn_channel;
    char buff[256];
    char *p;

    if (argc != 2)
    {
	Tcl_AppendResult(interp, "Wrong number of arguments db2_disconnect\n", 0);
	return TCL_ERROR;
    }

    /* db2sqlX.Y.Z */

    p = strchr(argv[1], '.');
    
    if (p)
    {
	strncpy(buff, argv[1], (size_t)(p - argv[1]));
	*(buff + (p - argv[1])) = '\0';
	conn_channel = Tcl_GetChannel(interp, buff, 0);
    }
    else
    {
	conn_channel = Tcl_GetChannel(interp, argv[1], 0);
    }
    
    if (conn_channel == NULL)
    {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, argv[1], " is not a valid connection\n", 0);
        return TCL_ERROR;
    }

    Tcl_ResetResult(interp);

    return Tcl_UnregisterChannel(interp, conn_channel);
}

int Db2_exec(ClientData cData,
             Tcl_Interp *interp,
             int argc,
             char *argv[])
{
    Tcl_Channel conn_channel;
    Db2Connection *conn;
    SQLHANDLE hstmt;
    char buff[256];

    conn_channel = Tcl_GetChannel(interp, argv[1], 0);

    if (conn_channel == NULL)
    {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, argv[1], " is not a valid connection\n", 0);
        return TCL_ERROR;
    }

    conn = (Db2Connection*) Tcl_GetChannelInstanceData(conn_channel);

    conn->rc = SQLAllocHandle(SQL_HANDLE_STMT, conn->hdbc, &hstmt);
    if (conn->rc != SQL_SUCCESS)
    {
        SQLError(henv, conn->hdbc, hstmt,
        (SQLCHAR *)&conn->sql_state,
        &conn->native_error,
        (SQLCHAR *)&conn->error_msg,
        sizeof(conn->error_msg),
        &conn->size_error_msg);
        Tcl_AppendResult(interp, conn->error_msg, 0);
        return TCL_ERROR;
    }
 
    conn->rc = SQLExecDirect(hstmt, argv[2], SQL_NTS);
    if (conn->rc != SQL_SUCCESS)
    {
        SQLError(henv, conn->hdbc, hstmt,
        (SQLCHAR *)&conn->sql_state,
        &conn->native_error,
        (SQLCHAR *)&conn->error_msg,
        sizeof(conn->error_msg),
        &conn->size_error_msg);
        Tcl_AppendResult(interp, conn->error_msg, 0);
        return TCL_ERROR;
    }
 
    /* Return result by type dbsql2X.Y */
    sprintf(buff, "%s.%d", conn->id, SQL_NULL_HANDLE);
    Tcl_AppendResult(interp, buff, 0);

    conn->rc = SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    if (conn->rc != SQL_SUCCESS)
    {
        SQLError(henv, conn->hdbc, hstmt,
        (SQLCHAR *)&conn->sql_state,
        &conn->native_error,
        (SQLCHAR *)&conn->error_msg,
        sizeof(conn->error_msg),
        &conn->size_error_msg);
        Tcl_AppendResult(interp, conn->error_msg, 0);
        return TCL_ERROR;
    }
 
    return TCL_OK;
}

int Db2_select(ClientData cData,
             Tcl_Interp *interp,
             int argc,
             char *argv[])
{
    Tcl_Channel conn_channel;
    Db2Connection *conn;
    SQLHANDLE hstmt;
    short num_fields;
    char buff[256];

    conn_channel = Tcl_GetChannel(interp, argv[1], 0);

    if (conn_channel == NULL)
    {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, argv[1], " is not a valid connection\n", 0);
        return TCL_ERROR;
    }

    conn = (Db2Connection*) Tcl_GetChannelInstanceData(conn_channel);

    conn->rc = SQLAllocHandle(SQL_HANDLE_STMT, conn->hdbc, &hstmt);
    if (conn->rc != SQL_SUCCESS)
    {
        SQLError(henv, conn->hdbc, hstmt,
        (SQLCHAR *)&conn->sql_state,
        &conn->native_error,
        (SQLCHAR *)&conn->error_msg,
        sizeof(conn->error_msg),
        &conn->size_error_msg);
        Tcl_AppendResult(interp, conn->error_msg, 0);
        return TCL_ERROR;
    }

    conn->rc = SQLExecDirect(hstmt, argv[2], SQL_NTS);
    if (conn->rc != SQL_SUCCESS)
    {
        SQLError(henv, conn->hdbc, hstmt,
        (SQLCHAR *)&conn->sql_state,
        &conn->native_error,
        (SQLCHAR *)&conn->error_msg,
        sizeof(conn->error_msg),
        &conn->size_error_msg);
        Tcl_AppendResult(interp, conn->error_msg, 0);
        return TCL_ERROR;
    }


    conn->rc = SQLNumResultCols(hstmt, &num_fields);
    if (conn->rc != SQL_SUCCESS)
    {
        SQLError(henv, conn->hdbc, hstmt,
        (SQLCHAR *)&conn->sql_state,
        &conn->native_error,
        (SQLCHAR *)&conn->error_msg,
        sizeof(conn->error_msg),
        &conn->size_error_msg);
        Tcl_AppendResult(interp, conn->error_msg, 0);
        return TCL_ERROR;
    }
    
    sprintf(buff, "%s.%d.%d", conn->id, hstmt, num_fields);
    Tcl_AppendResult(interp, buff, 0);

    return TCL_OK;
}

int Db2_fetchrow(ClientData cData,
                 Tcl_Interp *interp,
                 int argc,
                 char *argv[])
{
    int i = 1;
    int no_list = 0;
    int num_col = 0;
    Tcl_Channel conn_channel;
    Db2Connection *conn;
    SQLPOINTER *buff;
    SQLINTEGER size_buff;
    char id[18];
    SQLHANDLE hdbc, hstmt;
    short num_fields;

    if (argc > 4)
    {
        Tcl_AppendResult(interp, "Wrong number of arguments", 0);
        return TCL_ERROR;
    }
    
    while(argv[i])
    {
	if (strcmp(argv[i], "-nolist"))
	{
	    no_list = 1;
	}
	else if (strcmp(argv[i], "-numcol"))
        {
	    if (argv[i + 1])
	    {
	        num_col = atoi(argv[i]);
	    }
	}
	i++;
    }
    
    sscanf(argv[1], "db2sql%d.%d.%d", &hdbc, &hstmt, &num_fields);

    sprintf(id, "db2sql%d", (int)hdbc);

    conn_channel = Tcl_GetChannel(interp, id, 0);

    
    if (conn_channel == NULL)
    {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, argv[1], " is not a valid connection\n", 0);
        return TCL_ERROR;
    }

    conn = (Db2Connection*) Tcl_GetChannelInstanceData(conn_channel);

    conn->rc = SQLFetch(hstmt);

    if (conn->rc == SQL_NO_DATA_FOUND)
    {
	return TCL_OK;
    }
    
    if (conn->rc != SQL_SUCCESS)
    {
        SQLError(henv, conn->hdbc, hstmt,
        (SQLCHAR *)&conn->sql_state,
        &conn->native_error,
        (SQLCHAR *)&conn->error_msg,
        sizeof(conn->error_msg),
        &conn->size_error_msg);
        Tcl_AppendResult(interp, conn->error_msg, 0);
        return TCL_ERROR;
    }

    size_buff = 4096;
    buff = (SQLPOINTER*) malloc(size_buff);

    if (num_col)
    {
	    conn->rc = SQLGetData(hstmt,
			      num_col,
			      SQL_C_CHAR,
			      buff,
			      size_buff,
			      NULL);

	    if (no_list)
	    {
    		Tcl_AppendElement(interp, (char *)buff);
	    }
	    else
	    {
		Tcl_AppendResult(interp, (char *)buff, " ", 0);
	    }
    }
    else
    {
	for(i = 0; i < num_fields; i++)
	{
	    conn->rc = SQLGetData(hstmt,
			      i + 1,
			      SQL_C_CHAR,
			      buff,
			      size_buff,
			      NULL);

	    if (no_list)
	    {
    		Tcl_AppendElement(interp, (char *)buff);
	    }
	    else
	    {
		Tcl_AppendResult(interp, (char *)buff, " ", 0);
	    }
	}
    }
    
    free(buff);

    return TCL_OK;    
}

int Db2_finish(ClientData cData,
               Tcl_Interp *interp,
               int argc,
               char *argv[])
{
    Tcl_Channel conn_channel;
    Db2Connection *conn;
    SQLHANDLE hdbc, hstmt;
    short num_fields;    
    char *p;
    char buff[256];

    p = strchr(argv[1], '.');
    
    if (p)
    {
	strncpy(buff, argv[1], (size_t)(p - argv[1]));
	*(buff + (p - argv[1])) = '\0';
	conn_channel = Tcl_GetChannel(interp, buff, 0);
    }
    else
    {
	conn_channel = Tcl_GetChannel(interp, argv[1], 0);
    }

    if (conn_channel == NULL)
    {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, argv[1], " is not a valid connection\n", 0);
        return TCL_ERROR;
    }

    conn = (Db2Connection*) Tcl_GetChannelInstanceData(conn_channel);

    sscanf(argv[1], "db2sql%d.%d.%d", &hdbc, &hstmt, &num_fields);

    conn->rc = SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

    if (conn->rc != SQL_SUCCESS)
    {
        SQLError(henv, conn->hdbc, hstmt,
        (SQLCHAR *)&conn->sql_state,
        &conn->native_error,
        (SQLCHAR *)&conn->error_msg,
        sizeof(conn->error_msg),
        &conn->size_error_msg);
        Tcl_AppendResult(interp, conn->error_msg, 0);
        return TCL_ERROR;
    }

    return TCL_OK;    
}

