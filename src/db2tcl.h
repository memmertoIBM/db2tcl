/* $Id$ */

#ifndef LIBDB2TCL_H
#define LIBDB2TCL_H

#include <tcl.h>
#ifdef _WINDOWS
__declspec( dllexport ) Db_Init(Tcl_Interp *interp);
__declspec( dllexport ) Db2tcl_Init(Tcl_Interp *interp);
__declspec( dllexport ) Db2tcl_SafeInit(Tcl_Interp *interp);
#else
extern int Db_Init(Tcl_Interp *interp);
extern int Db2tcl_Init(Tcl_Interp *interp);
extern int Db2tcl_SafeInit(Tcl_Interp *interp);
#endif
#endif	 /* LIBDB2TCL_H */
