# NetCI
NetCI - Network Command Interpreter

This is version 1.9.6 of NetCI.

Currently, the source code is being maintained by Kevin Morgan 
(LimpingNinja@github), with the original code having been written by 
Patrick Wetmore. The ci200fs mudlib that accompanies the codebase was 
written by Gregory Blake.

There is not currently a mailing list, documentation is minimal, but you
will be able to compile this easily on Linux as of this version by simple
use of build-essentials (or equivalent) on your system. 

### Compiling on Debian

Ensure that you have at least build-essentials installed:

```
sudo apt-get install build-essentials
```

Then simply:

```
make
```

There should be no gotcha's or surprises.

### Running The Server

Simply run the server with **./netci** or with **./netci \<inifile\>** 

#### Configuration Options
All of the command line values below are saved in the **netci.ini** file, you can edit this file as opposed to running the command line options. The default INI file values should be appropriate for running your system without modification, but feel free to modify as necessary as opposed to modifying the **src/config.h** file.

#### Command Line Options

| Option  | Description  |  
|---|---|
| *-load=[filename]* | Specifies the database file to load the world from |
| *-save=[filename]* | Specifies the databse file to save the world to |
| *-panic=[filename]* | Specifies the destination of the panic database on critical error |
| *-filesystem=[directory]* | This is the path to the mudlib/filesystem to use for the interpreter |
| *-syslog=[filename]* | The filename for the system log, essentially server related output |
| *-xlog=[filename]* | The filename for the transaction log |
| *-xlogsize=[#]* | The amount to store within the transaction log in KB |
| *-tmpdb=[filename]* | Specifies the location of the temporary database files to use when operating |
| *-create* | Force a recreation of the database file from the filesystem/mudlib |
| *-multi* | Sets the interface type to multi (this is default) |
| *-single* | Sets the interface type to single-mode|
| *-detach* | Detach the NetCI session from the current |
| *-noisy* | Verbose operating mode, good for console logging |
| *-version* | Prints out the current version of NetCI |
| *-protocol=[tcp\|ipx\|netbios]* | Determines the protocol to run the server under |
| *-port=[#]* | Set the operant TCP port for the server |
| *-ipxnet=[hex]* | IPX network configuration |
| *-ipxnode=[hex]* | IPX node configuration |
| *-ipxsocket=[hex]* | IPX socket configuration |
| *-node=[name]* | Sets the NetBios node name for the server |
| *-nbport=[#]* | Set the operant NetbBios port for the server |
 
