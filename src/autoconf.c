/* autoconf.c */

/* automatically configures autoconf.h for *IX machines */

#include <stdio.h>
#include <stdlib.h>

int main();
int search_for_gcc();

int search_for_gcc() {
  char buf[1024];
  char *path;
  int position,last_pos,count;
  FILE *infile;

  path=getenv("PATH");
  if (!path) return 0;
  last_pos=0;
  position=0;
  count=0;
  while (path[position]) {
    while (path[position]!=':' && path[position]!='\0') {
      if (count<1019) {
        buf[count]=path[position];
        count++;
      }
      position++;
    }
    if (count) {
      if (buf[count-1]!='/') {
        buf[count]='/';
        count++;
      }
      buf[count++]='g';
      buf[count++]='c';
      buf[count++]='c';
      buf[count]='\0';
      if ((infile=fopen(buf,"r"))) {
        fclose(infile);
        return 1;
      }
    }
    count=0;
    if (path[position]==':') position++;
  }
  return 0;
}

int main(argc, argv)
  int argc;
  char *argv[];
{
  FILE *makefile,*autoconf,*infile;
  char buf[1024];
  int has_gcc;

  has_gcc=search_for_gcc();
#ifdef USE_GCC
#ifdef USE_CC
  fprintf(stderr,"You cannot #define both USE_GCC and USE_CC. Aborting...\n");
  exit(1);
#endif
  has_gcc=1;
#endif
#ifdef USE_CC
  has_gcc=0;
#endif
#ifndef __STDC__
#ifndef USE_CC
  if (!has_gcc) {
    fprintf(stderr,"You do not have an ANSI C compiler. Aborting...\n");
    exit(1);
  }
#endif
#endif
  if (has_gcc) {
#ifdef USE_GCC
    fprintf(stdout,"Assuming gcc is in your path...\n");
#else
    fprintf(stdout,"Found gcc in your path...\n");
#endif
  } else {
#ifdef USE_CC
    fprintf(stdout,"Using cc -ansi...\n");
#else
    fprintf(stdout,"Didn't find gcc in your path, will use cc -ansi...\n");
#endif
  }
  remove("Makefile");
  remove("autoconf.h");
  makefile=fopen("Makefile","w");
  if (!makefile) {
    fprintf(stderr,"Couldn't open Makefile for writing. Aborting...\n");
    exit(1);
  }
  autoconf=fopen("autoconf.h","w");
  if (!autoconf) {
    fclose(makefile);
    remove("Makefile");
    fprintf(stderr,"Couldn't open autoconf.h for writing. Aborting...\n");
    exit(1);
  }
  fputs("/* autoconf.h */\n",autoconf);
  fputs("\n",autoconf);
  fputs("/* automatically generated, do not modify */\n",autoconf);
  fputs("\n",autoconf);
  fputs("CC=",makefile);
  if (has_gcc) {
    fputs("gcc",makefile);
    fputs("#define USE_GCC\n",autoconf);
  } else {
    fputs("#define USE_CC\n",autoconf);
    fputs("cc",makefile);
  }
  fputc('\n',makefile);
  fputs("CCFLAGS=",makefile);
  if (!has_gcc) {
    fputs(" -ansi",makefile);
  }
#ifdef DEBUG
  if (has_gcc) {
    fputs(" -ggdb -pg",makefile);
  } else {
    fputs(" -g",makefile);
  }
  fprintf(stdout,"Using debug settings...\n");
#else
  fputs(" -O2",makefile);
#endif
  fputc('\n',makefile);
  fputs("DEFS=",makefile);
#ifdef DEBUG
  fputs(" -DDEBUG",makefile);
#endif
  fputc('\n',makefile);
  fputs("LFLAGS=",makefile);
  if ((infile=fopen("/usr/lib/libnsl.a","r"))) {
    fputs("#define USE_NSL\n",autoconf);
    fputs(" -lnsl",makefile);
    fclose(infile);
    fprintf(stdout,"Found /usr/lib/libnsl.a, adding nsl library to linker flags...\n");
  } else if ((infile=fopen("/usr/lib/libnsl.so","r"))) {
    fputs("#define USE_NSL\n",autoconf);
    fputs(" -lnsl",makefile);
    fclose(infile);
    fprintf(stdout,"Found /usr/lib/libnsl.so, adding nsl library to linker flags...\n");
  }
  if ((infile=fopen("/usr/lib/libsocket.a","r"))) {
    fputs("#define USE_SOCKET\n",autoconf);
    fputs(" -lsocket",makefile);
    fclose(infile);
    fprintf(stdout,"Found /usr/lib/libsocket.a, adding socket library to linker flags...\n");
  } else if ((infile=fopen("/usr/lib/libsocket.so","r"))) {
    fputs("#define USE_SOCKET\n",autoconf);
    fputs(" -lsocket",makefile);
    fclose(infile);
    fprintf(stdout,"Found /usr/lib/libsocket.so, adding socket library to linker flags...\n");
  }
  fputc('\n',makefile);
  fputc('\n',makefile);
  infile=fopen("autoconf.in","r");
  if (!infile) {
    fclose(makefile);
    fclose(autoconf);
    remove("Makefile");
    remove("autoconf.h");
    fprintf(stderr,"Couldn't open autoconf.in for reading. Aborting...\n");
    exit(1);
  }
  while (fgets(buf,1024,infile)) fputs(buf,makefile);
  fclose(makefile);
#ifndef SEEK_SET
  fputs("#define USE_SEEK_SET\n",autoconf);
  fputs("#define SEEK_SET 0\n",autoconf);
  fprintf(stdout,"SEEK_SET not defined, assuming it is 0...\n");
#endif
  if ((infile=fopen("/usr/include/linux/kernel.h","r"))) {
    fclose(infile);
    fputs("#define USE_LINUX\n",autoconf);
    fprintf(stdout,"Found /usr/include/linux/kernel.h, assuming this is a Linux system...\n");
    if ((infile=fopen("/usr/include/linux/ipx.h","r"))) {
      fclose(infile);
      fputs("#define USE_LINUX_IPX\n",autoconf);
      fprintf(stdout,"Found /usr/include/linux/ipx.h, including IPX support...\n");
    }
  } else if ((infile=fopen("/usr/include/unistd.h","r"))) {
    fclose(infile);
    fputs("#define USE_POSIX\n",autoconf);
    fprintf(stdout,"Found /usr/include/unistd.h, assuming this is a POSIX system...\n");
  } else {
    fputs("#define USE_BSD\n",autoconf);
    fprintf(stdout,"Did not find /usr/include/unistd.h, assuming this is a BSD system...\n");
  }
  fclose(autoconf);
  fprintf(stdout,"Automatic configuration complete.\n");
  exit(0);
  return 0;
}
