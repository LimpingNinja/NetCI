Section 6 - Sitelist

The sitelist controls who can access the server in what ways.  The
list is stored in /etc/sitelist and is in the following format:

<permissions>:<site>:<mask>

permission: This is either p(ermit), r(egistration), or f(orbid).  If
the mud has global registration turned on, registration site permissions
are ignored.

site: Four-part decimal Internet address, specified as a.b.c.d,
      of the host or network to which this parameter applies.

mask: Four-part decimal Internet address, specified as a.b.c.d
      the mask is used to determine which parts of the address
      are to be used for comparison, it is ANDed with both
      the address of the host and the address in the directive.

The parameters are applied in the reverse of the order specified
in the configuration file, and the first one that matches is used.
Therefore, you should put the more-specific parameters before the
general ones.

Example:
	f:0.0.0.0:0.0.0.0		<- Forbid everyone else
	r:198.17.145.0:255.255.255.255	<- Permit class C subnet w/reg.
	p:198.17.145.1:255.255.255.255	<- Permit everyone from this site

When changes are made to the sitelist file, you must use the @updateban
command to reload it into the mud.

See also: @listban(5), @updateban(5)

