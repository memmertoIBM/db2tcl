/* $Id$ */

#include "db2tcl.h"
#include "db2tclcmds.h"

int Db_Init (Tcl_Interp * interp)
{
    /* register all Db2tcl commands */

    Tcl_CreateCommand (interp, "db2_connect", Db2_connect,
		       (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand (interp, "db2_disconnect", Db2_disconnect,
		       (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand (interp, "db2_exec", Db2_exec,
		       (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand (interp, "db2_select", Db2_select,
		       (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand (interp, "db2_fetchrow", Db2_fetchrow,
		       (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand (interp, "db2_finish", Db2_finish,
		       (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand (interp, "db2_getnumrows", Db2_finish,
		       (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand (interp, "db2", Db2_db2,
		       (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateObjCommand (interp, "db2_test", Db2_test,
                       (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_PkgProvide (interp, "db2tcl", "1.0");

    return TCL_OK;
}

int Db_SafeInit (Tcl_Interp * interp)
{
    return Db_Init (interp);
}
