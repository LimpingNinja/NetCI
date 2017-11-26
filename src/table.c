/* table.c */

#include "config.h"
#include "object.h"
#include "constrct.h"
#include "table.h"

#define TABLE_SIZE 69

struct hte
{
  char *key;
  char *datum;
  struct hte *next;
};

static struct hte *table[TABLE_SIZE];

static int get_hash_num(char *key) {
  int hash_num,count;

  hash_num=0;
  count=0;
  while (*key)
    hash_num=(((++count)*(*(key++)))+hash_num)%TABLE_SIZE;
  return hash_num;
}

void init_table() {
  int loop;

  loop=0;
  while (loop<TABLE_SIZE) {
    table[loop++]=NULL;
  }
}

void write_table(FILE *outfile) {
  int loop;
  struct hte *curr_hte;

  loop=0;
  while (loop<TABLE_SIZE) {
    curr_hte=table[loop++];
    while (curr_hte) {
      fprintf(outfile,"%ld\n%s",(long) strlen(curr_hte->key),curr_hte->key);
      fprintf(outfile,"%ld\n%s",(long) strlen(curr_hte->datum),curr_hte->datum);
      curr_hte=curr_hte->next;
    }
  }
}

char *get_from_table(char *key) {
  int hash_num,cmpval;
  struct hte *curr_hte;

  hash_num=get_hash_num(key);
  curr_hte=table[hash_num];
  while (curr_hte) {
    cmpval=strcmp(key,curr_hte->key);
    if (!cmpval) return curr_hte->datum;
    if (cmpval>0) break;
    curr_hte=curr_hte->next;
  }
  return NULL;
}

void add_to_table(char *key, char *datum) {
  int hash_num;
  struct hte *curr_hte,*prev_hte,*new_hte;

  if (!key) return;
  hash_num=get_hash_num(key);
  curr_hte=table[hash_num];
  prev_hte=NULL;
  while (curr_hte) {
    if (strcmp(key,curr_hte->key)>=0) break;
    prev_hte=curr_hte;
    curr_hte=curr_hte->next;
  }
  if (curr_hte)
    if (!strcmp(key,curr_hte->key)) {
      if (datum) {
        FREE(curr_hte->datum);
        curr_hte->datum=copy_string(datum);
      } else {
        if (prev_hte) prev_hte->next=curr_hte->next;
        else table[hash_num]=curr_hte->next;
        FREE(curr_hte->key);
        FREE(curr_hte->datum);
        FREE(curr_hte);
      }
      return;
    }
  if (!datum) return;
  new_hte=MALLOC(sizeof(struct hte));
  new_hte->key=copy_string(key);
  new_hte->datum=copy_string(datum);
  new_hte->next=curr_hte;
  if (prev_hte) prev_hte->next=new_hte;
  else table[hash_num]=new_hte;
}

void delete_from_table(char *key) {
  add_to_table(key,NULL);
}
