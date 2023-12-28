/* strings.h */

/*
 * This file includes the following commands:
 * 
 * pad(s,len,padstr)
 * string s,padstr;
 * int len;
 * Pads string s to len characters, using padstr, if there
 * is no padstr space is assumed.
 *
 * center(s,len,padstr)
 * string;
 * int len;
 * centers the string s in a space len characters long (padstr
 * is optional)
 *
 */

static pad(s,len,padstr) {
  int count;
  
  if (!padstr) padstr=" ";
  if (len<0) {
    len=len*(-1);
    count=len-strlen(s);
    if (count < 0) return rightstr(s,(len));
    while (count--) s=padstr+s;
    return s;
  } else {
    count=len-strlen(s);
    if (count < 0) return leftstr(s,(len));
    while (count--) s+=padstr;
    return s;
  }
}

static center(s,len,padstr) {
  int left,right;
  
  if (!padstr) padstr=" ";
  right=strlen(s)+(len-strlen(s))/2;
  s=pad(s,right,padstr);
  s=pad(s,-len,padstr);
  return s;
}








