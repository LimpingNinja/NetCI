
Section 7 - table_set

table_set(key,datum)

string key,datum;

CI maintains a table of string keys and associated string data. This
call associates a string data item with a key in this table. The
calling object must be set PRIV. 0 is returned on success, 1 on failure.

See Also: table_delete(7), table_get(7)

