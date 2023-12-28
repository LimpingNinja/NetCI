/* ban.c */

string site,mask,comment,perm;
int snum,smask;

#define CAN_MOD (priv(caller_object()))

recycle() {
  destruct(this_object());
  return 1;
}

static init() {
  comment="<no comment>";
}

set_snum(arg) {
  if (CAN_MOD) snum=arg;
}

set_smask(arg) {
  if (CAN_MOD) smask=arg;
}

get_snum(arg) {
  return snum;
}

get_smask(arg) {
  return smask;
}

set_site(arg) {
  if (CAN_MOD) site=arg;
}

set_mask(arg) {
  if (CAN_MOD) mask=arg;
}

set_perm(arg) {
  if (CAN_MOD) perm=arg;
}

set_comment(arg) {
  if (CAN_MOD) comment=arg;
}

get_site(arg) {
  return site;
}

get_mask(arg) {
  return mask;
}

get_perm(arg) {
  return perm;
}

get_comment(arg) {
  return comment;
}
