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

### FAQ

##### 1. What is this?
This is a database driven command interpreter for Networks. Just as the
name implies. It allows you to write code in a C-like language that is 
interpreted and translated to byte code, then stored in a database, in 
order to make an interactive, network-connected, environment. In most 
cases, systems like this were used to create MUDs (Multi-user dungeons)
and this is not an exception.

The popularity of NetCI as a MUD server was minimal, it was operating in
a space where LPMud was popular and already had a fair throttle on the 
server/interpreter/lib pattern. The Mudlib written for the server (and
included) is based after TinyMUD and very minimal. The interpreter itself
also has some limitations (non-dynamic arrays, no mappings, etc.) which
could have prevented some complexity.

##### 2. Why work on this?
Why not? I want to apply some WD-40 to my C coding, I like MUDs, and
I want to see what I can do with the interpreter on this codebase for
purely educational purposes.

On top of that, I want to use this as a base for teaching my young son
how to code. I missed the boat with my older children, both in getting 
them interested in MUDs and coding, and maybe this will be different!

##### 3. What is the future here?
The question can apply to MUDs in general. Their heyday is well past,
but I plan on doing the following initially:

1. Cleaning up the C code structure.
2. Adding/changing functionality of the interpreter to bring it inline with my standards.
3. Creating an testsuite library, similar to **lil** for MudOS.
4. Creating a new mudlib that acts as a base mudlib for design.

Now, is there any future use in a production environment? Probably not,
but that doesn't mean you should ignore this. If you have spare cycles
and nostalgia. Feel free to submit a pull request.

### Compiling on Debian-based linux (tested)

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
 
