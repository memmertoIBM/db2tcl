#!/bin/sh
# the next line restarts using tclsh \
exec tclsh "$0" "$@"

#lappend auto_path /usr/lib/tcl_local $env(RDS_TCL_SCRIPTS)/lib

load "../src/.libs/libdb2tcl.so"

puts "Executing test1:"

puts "Connect to database SAMPLE..."

set conn1 [db2_connect SAMPLE]

puts "Drop table db2tcl if exist..."

catch [ 
    db2_exec $conn1 "DROP TABLE db2tcl"
]

puts "Create table db2tcl..."

db2_exec $conn1 "CREATE TABLE db2tcl (ID INTEGER NOT NULL PRIMARY KEY, NAME CHAR(25))"

puts "Insert data into table db2tcl..."

db2_exec $conn1 "INSERT INTO db2tcl VALUES (1,'West')" 
db2_exec $conn1 "INSERT INTO db2tcl VALUES (2,'Gour')" 
db2_exec $conn1 "INSERT INTO db2tcl VALUES (3,'Manger')" 
db2_exec $conn1 "INSERT INTO db2tcl VALUES (4,'Slava')" 

puts "Disconnect from database SAMPLE..."

db2_disconnect $conn1

