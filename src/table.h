/* table.h */

void init_table();
void write_table(FILE *outfile);
char *get_from_table(char *key);
void add_to_table(char *key, char *datum);
void delete_from_table(char *key);
