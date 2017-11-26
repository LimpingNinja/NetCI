int w,x,y,z;
#include <strings.h>

#define SITELIST "/etc/sitelist"
#define SITEOBJ atoo("/sys/site")

site_status(arg) {
  int test;
  object curr;
 
  make_addr(arg);
  test=((w<<24)+(x<<16)+(y<<8)+z);
  curr=next_child(SITEOBJ);
  while (curr) {
    if ((curr.get_snum()^test)&curr.get_smask()) {
      curr=next_child(curr);
    } else {
      return (curr.get_perm());
    }
  }
  return 1;
}

reload_sites() {
  if (priv(caller_object())) {
    clear_sites();
    load_sites();
    return 1;
  } else return 0;
}

load_sites(arg) {
  string line,perm,site,mask;
  int pos,c,snum,smask,valid,count;

  pos=0;
  count=1;
  line=fread(SITELIST,pos);
  while (line) {
    valid=0;
    perm="";
    site="";
    mask="";
    c=instr(line,1,":");
    if (c) perm=leftstr(line,1);  
    if (perm=="f" || perm=="p" || perm=="r") valid=1;  /* check first char */
    c=instr(line,c+1,":");
    if (c) {
      site=midstr(line,3,c-3);
      mask=midstr(line,c+1,strlen(line)-c-1);
    }
    if (site && mask) valid=1;
    else
      syslog("** ERROR ** - line "+itoa(count)+" malforms in /etc/sitelist");
    if (valid) {
      make_addr(site);
      if (valid_addr(w,x,y,z)) {     /* if the site is valid */
	snum=((w<<24)+(x<<16)+(y<<8)+z);
	make_addr(mask);
	if (valid_mask(w,x,y,z)) {
	  smask=((w<<24)+(x<<16)+(y<<8)+z);
	  create_site(perm,site,mask,snum,smask);
	} else syslog("Invalid mask on line "+itoa(count)+" of /etc/sitelist");
      } else syslog("Invalid site on line "+itoa(count)+" of /etc/sitelist");
    }
    line=fread(SITELIST,pos);
    count=count+1;
  }
}

make_addr(arg) {
  int c,c2;
 
  c=instr(arg,1,".");
  if (c) w=atoi(leftstr(arg,c-1));
  c2=c+1;
  c=instr(arg,c2,".");
  if (c) x=atoi(midstr(arg,c2,(c-c2)));
  c2=c+1;
  c=instr(arg,c2,".");
  if (c) {
    y=atoi(midstr(arg,c2,c-c2));
    z=atoi(rightstr(arg,strlen(arg)-c));
  }
}

static valid_mask(a,b,c,d) {
  if ((a >= 0) && (a<=255))
    if ((b >= 0) && (b<=255))
      if ((c>=0) && (c<=255))
        if ((d>=0) && (c<=255))
          return 1;
  return 0;
}

static valid_addr(a,b,c,d) {
  if ((a >= 0) && (a<255))
    if ((b >= 0) && (b<255))
      if ((c>=0) && (c<255))
        if ((d>=0) && (c<255))
          return 1;
  return 0;
}

static create_site(perm,site,mask,snum,smask) {
  object o;
  
  o=new("/sys/site");
  if (!o)
    return 0;
  o.set_snum(snum);
  o.set_smask(smask);
  o.set_perm(perm);
  o.set_mask(mask);
  o.set_site(site);
  return 1;
}

static clear_sites() {
  object o;

  if (caller_object()==this_object()) {
    o=next_child(SITEOBJ);
    while (o) {
      o.recycle();
      o=next_child(o);
    }
  }
}

list_sites() {
  object o;
  
  o=next_child(SITEOBJ);
  this_player().listen(pad("site",20,".")+pad("mask",20,".")+
		       pad("permissions",30,".")+"\n");
  while (o) {
    this_player().listen(pad(o.get_site(),20)+pad(o.get_mask(),20));
    if (o.get_perm()=="f") this_player().listen("Forbid Access\n");
    if (o.get_perm()=="r") this_player().listen("Registration Access\n");
    if (o.get_perm()=="p") this_player().listen("Permit Access\n");
    o=next_child(o);
  }
  this_player().listen(pad("",70,".")+"\n");
}
