/* verb.h */

static read_verb_file(arg) {
  int filepos,linecount,pos;
  string cmdline,cmdname,args,arg1,arg2;

  while (cmdline=next_verb(this_object(),NULL)) remove_verb(cmdline);
  while (cmdline=fread(arg,filepos)) {
    linecount++;
    while (leftstr(cmdline,1)==" ")
      cmdline=rightstr(cmdline,strlen(cmdline)-1);
    while (rightstr(cmdline,1)=="\n")
      cmdline=leftstr(cmdline,strlen(cmdline)-1);
    while (rightstr(cmdline,1)=="\r")
      cmdline=lefstr(cmdline,strlen(cmdline)-1);
    while (rightstr(cmdline,1)==" ")
      cmdline=leftstr(cmdline,strlen(cmdline)-1);
    if (cmdline)
      if (leftstr(cmdline,1)!="#") {
       pos=instr(cmdline,1," ");
        if (pos) {
          cmdname=leftstr(cmdline,pos-1);
          args=rightstr(cmdline,strlen(cmdline)-pos);
        } else {
          cmdname=cmdline;
          args=0;
        }
        if (cmdname=="verb" || cmdname=="xverb") {
          pos=instr(args,1,",");
          if (pos) {
            arg1=leftstr(args,pos-1);
            arg2=rightstr(args,strlen(args)-pos);
            while (rightstr(arg1,1)==" ") arg1=leftstr(arg1,strlen(arg1)-1);
            while (leftstr(arg2,1)==" ") arg2=rightstr(arg2,strlen(arg2)-1);
          } else {
            arg1=args;
            arg2=0;
          }
          if (!arg2) {
            if (this_player()) write(this_player(),"/obj/player.vrb: "+
                                     "syntax error in line #"+
                                     itoa(linecount)+"\n");
            return 1;
          }
          if (cmdname=="verb") add_verb(arg1,arg2);
          else add_xverb(arg1,arg2);
        } else {
          if (this_player()) write(this_player(),"/obj/player.vrb: "+
                                   "syntax error in line #"+
                                   itoa(linecount)+"\n");
          return 1;
        }
     }
  }
  return 0;
}
