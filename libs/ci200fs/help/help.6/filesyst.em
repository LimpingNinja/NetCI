
Section 6 - filesystem

The filesystem consists of two layers; a tree stored in the database,
with nodes representing directories and files, and containing their
associated attributes (permissions and owners), and the actual
physical files and directories.

The database filesystem does not necessarily correspond to the
actual filesystem; with hide() and unhide() it is possible to
to change the database filesystem tree so it contains entries that
do not correspond to physical files, and lacks entries that exist
in the physical filesystem.

See Also: hide(7), unhide(7)

