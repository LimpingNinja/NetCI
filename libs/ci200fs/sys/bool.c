#include <sys.h>
#include <flags.h>
#include <gender.h>

object parent_object;
var arg1;
var arg2;
int operator;

#define OR_OPERATOR 10
#define AND_OPERATOR 20
#define NOT_OPERATOR 30
#define PARENTHESIS_OPERATOR 40
#define PRESENT_OPERATOR 50
#define HOLDING_OPERATOR 51
#define CALL_OPERATOR 52
#define GENDER_OPERATOR 53
#define TRADITIONAL_OPERATOR 60


/* i'm commenting parse_bool_exp because it's actually semi-complex.
   CI-C is not the best language to write boolean expression parsers,
   should the urge ever overcome you to do such a thing. */

eliminate_parentheses() {
  if (operator==PARENTHESIS_OPERATOR) {
    destruct(this_object());
    return arg1.eliminate_parentheses();
  }
  if (operator==AND_OPERATOR || operator==OR_OPERATOR ||
      operator==NOT_OPERATOR)
    arg1=arg1.eliminate_parentheses();
  if (operator==AND_OPERATOR || operator==OR_OPERATOR)
    arg2=arg2.eliminate_parentheses();
  return this_object();
}

get_bool_exp(prev_oper) {
  string s;

  if (prev_oper>operator) s="(";
/*  s+="{#"+itoa(otoi(this_object()))+"}"; */
  if (operator==OR_OPERATOR)
    s+=arg1.get_bool_exp(operator)+" | "+
       arg2.get_bool_exp(operator);
  else if (operator==AND_OPERATOR)
    s+=arg1.get_bool_exp(operator)+" & "+
       arg2.get_bool_exp(operator);
  else if (operator==NOT_OPERATOR)
    s+="!"+arg1.get_bool_exp(operator);
  else if (operator==PARENTHESIS_OPERATOR)
    s+="("+arg1.get_bool_exp(operator)+")";
  else if (operator==PRESENT_OPERATOR)
    if (arg2)
      s+="present(\""+arg1+"\","+make_num(arg2)+")";
    else
      s+="present("+arg1+",*VOID*)";
  else if (operator==HOLDING_OPERATOR)
    s+="holding(\""+arg1+"\")";
  else if (operator==CALL_OPERATOR)
    if (arg1)
      s+="call("+make_num(arg1)+",\""+arg2+"\")";
    else
      s+="call(*VOID*,\""+arg2+"\")";
  else if (operator==GENDER_OPERATOR) {
    if (arg1==GENDER_NONE) s+="gender(none)";
    else if (arg1==GENDER_MALE) s+="gender(male)";
    else if (arg1==GENDER_FEMALE) s+="gender(female)";
    else if (arg1==GENDER_NEUTER) s+="gender(neuter)";
    else if (arg1==GENDER_PLURAL) s+="gender(plural)";
    else if (arg1==GENDER_SPIVAK) s+="gender(spivak)";
    else s+="gender(unknown)";
  } else if (operator==TRADITIONAL_OPERATOR)
    if (arg1)
      s+=make_num(arg1);
    else
      s+="*VOID*";
  else s+="*** Unknown Operator ***";
  if (prev_oper>operator) s+=")";
  return s;
}

parse_bool_exp(s,error_rcpt) {
  int count,len,loop,in_quote,done;
  string left_half,right_half;
  object new_bool;

/* eat white space. i hate white space. */
  while (leftstr(s,1)==" ") s=rightstr(s,strlen(s)-1);
  while (rightstr(s,1)==" ") s=leftstr(s,strlen(s)-1);

/* if there's nothing left, return a magic cookie */
  if (!s) {
    error_rcpt.listen("@lock: null expression encountered\n");
    destruct(this_object());
    return -1;
  }

/* first, search for the | operator, which has lowest precedence */
  count=0;
  loop=1;
  len=strlen(s);
  done=0;
  while (!done && loop<=len) {
    if (!count && !in_quote && midstr(s,loop,1)=="|") done=1;
    else if (in_quote) {
      if (midstr(s,loop,1)=="\"") in_quote=0;
    } else if (midstr(s,loop,1)=="(") count++;
    else if (midstr(s,loop,1)==")")
      if (count) count--;
      else {
        error_rcpt.listen("@lock: mismatched parentheses\n");
        return -1;
      }
    loop++;
  }
  if (done) {             /* woo-hoo, we found a happy | operator */
    left_half=leftstr(s,loop-2);
    new_bool=new("/sys/bool");
    if (new_bool.parse_bool_exp(left_half,error_rcpt)==(-1)) {
      destruct(this_object());
      return -1;
    }
    arg1=new_bool;
    new_bool=new("/sys/bool");
    if (new_bool.parse_bool_exp(rightstr(s,len-loop+1),error_rcpt)==(-1)) {
      arg1.recycle_bool();
      destruct(this_object());
      return -1;
    }
    arg2=new_bool;
    operator=OR_OPERATOR;
    return 0;
  }

/* now search for the & operator - next lowest precedence */
  count=0;
  loop=1;
  len=strlen(s);
  done=0;
  while (!done && loop<=len) {
    if (!count && !in_quote && midstr(s,loop,1)=="&") done=1;
    else if (in_quote) {
      if (midstr(s,loop,1)=="\"") in_quote=0;
    } else if (midstr(s,loop,1)=="(") count++;
    else if (midstr(s,loop,1)==")")
      if (count) count--;
      else {
        error_rcpt.listen("@lock: mismatched parentheses\n");
        return -1;
      }
    loop++;
  }
  if (done) {             /* woo-hoo, we found a happy & operator */
    left_half=leftstr(s,loop-2);
    new_bool=new("/sys/bool");
    if (new_bool.parse_bool_exp(left_half,error_rcpt)==(-1)) {
      destruct(this_object());
      return -1;
    }
    arg1=new_bool;
    new_bool=new("/sys/bool");
    if (new_bool.parse_bool_exp(rightstr(s,len-loop+1),error_rcpt)==(-1)) {
      arg1.recycle_bool();
      destruct(this_object());
      return -1;
    }
    arg2=new_bool;
    operator=AND_OPERATOR;
    return 0;
  }

/* have we got a ! operator, perhaps? */
  if (leftstr(s,1)=="!") {
    left_half=rightstr(s,strlen(s)-1);
    new_bool=new("/sys/bool");
    if (new_bool.parse_bool_exp(left_half,error_rcpt)==(-1)) {
      destruct(this_object());
      return -1;
    }
    arg1=new_bool;
    operator=NOT_OPERATOR;
    return 0;
  }

/* highest precedence is the parenthesis */
  if (leftstr(s,1)=="(") {
    count=1;
    len=strlen(s);
    loop=2;
    in_quote=0;
    while (count && loop<=len) {
      if (in_quote) {
        if (midstr(s,loop,1)=="\"") in_quote=0;
      } else {
        if (midstr(s,loop,1)=="(") count++;
        else if (midstr(s,loop,1)==")") count--;
        else if (midstr(s,loop,1)=="\"") in_quote=1;
      }
      loop++;
    }
    if (count) {
      error_rcpt.listen("@lock: mismatched parentheses\n");
      destruct(this_object());
      return -1;
    }
    if (in_quote) {
      error_rcpt.listen("@lock: mismatched quotation marks\n");
      destruct(this_object());
      return -1;
    }
    left_half=midstr(s,2,loop-3);
    s=rightstr(s,len-loop+1);
    while (leftstr(s,1)==" ") s=rightstr(s,strlen(s)-1);
    if (s) {
      error_rcpt.listen("@lock: trailing garbage\n");
      destruct(this_object());
      return -1;
    }
    operator=PARENTHESIS_OPERATOR;
    new_bool=new("/sys/bool");
/*    error_rcpt.listen("in parentheses we have \""+left_half+"\"\n"); */
    if (new_bool.parse_bool_exp(left_half,error_rcpt)==(-1)) {
      destruct(this_object());
      return -1;
    }
    arg1=new_bool;
    return 0;
  }

/* is it the present("name",location) operator? */
  if (leftstr(s,7)=="present" && (midstr(s,8,1)==" " ||
                                   midstr(s,8,1)=="(")) {
    len=strlen(s);
    loop=instr(s,1,"(");
    if (!loop) {
      error_rcpt.listen("@lock: couldn't find argument list for present()\n");
      destruct(this_object());
      return -1;
    }
    loop=instr(s,loop,"\"");
    if (!loop) {
      error_rcpt.listen("@lock: first argument to present() not string\n");
      destruct(this_object());
      return -1;
    }
    count=instr(s,loop+1,"\"");
    if (!count) {
      error_rcpt.listen("@lock: mismatched quotation marks\n");
      destruct(this_object());
      return -1;
    }
    arg1=midstr(s,loop+1,count-loop-1);
    loop=instr(s,count,",");
    if (!loop) {
      error_rcpt.listen("@lock: no second argument to present()\n");
      destruct(this_object());
      return -1;
    }
    count=instr(s,loop,")");
    if (!count) {
      error_rcpt.listen("@lock: mismatched parentheses\n");
      destruct(this_object());
      return -1;
    }
    right_half=midstr(s,loop+1,count-loop-1);
    while (leftstr(right_half,1)==" ")
      right_half=rightstr(right_half,strlen(right_half)-1);
    while (rightstr(right_half,1)==" ")
      right_half=leftstr(right_half,strlen(right_half)-1);
    arg2=error_rcpt.find_object(right_half);
    if (!arg2) {
      error_rcpt.listen("@lock: couldn't find second argument to present()\n");
      destruct(this_object());
      return -1;
    }
    s=rightstr(s,strlen(s)-count);
    while (leftstr(s,1)==" ") s=rightstr(s,strlen(s)-1);
    if (s) {
      error_rcpt.listen("@lock: trailing garbage\n");
      destruct(this_object());
      return -1;
    }
    operator=PRESENT_OPERATOR;
    return 0;
  }

/* is it the holding("object") operator? */
  if (leftstr(s,7)=="holding" && (midstr(s,8,1)==" " ||
                                  midstr(s,8,1)=="(")) {
    len=strlen(s);
    loop=instr(s,1,"(");
    if (!loop) {
      error_rcpt.listen("@lock: couldn't find argument list for holding()\n");
      destruct(this_object());
      return -1;
    }
    loop=instr(s,loop,"\"");
    if (!loop) {
      error_rcpt.listen("@lock: argument to holding() not string\n");
      destruct(this_object());
      return -1;
    }
    count=instr(s,loop+1,"\"");
    if (!count) {
      error_rcpt.listen("@lock: mismatched quotation marks\n");
      destruct(this_object());
      return -1;
    }
    arg1=midstr(s,loop+1,count-loop-1);
    loop=instr(s,count,")");
    if (!loop) {
      error_rcpt.listen("@lock: mismatched parentheses\n");
      destruct(this_object());
      return -1;
    }
    s=rightstr(s,strlen(s)-loop);
    while (leftstr(s,1)==" ") s=rightstr(s,strlen(s)-1);
    if (s) {
      error_rcpt.listen("@lock: trailing garbage\n");
      destruct(this_object());
      return -1;
    }
    operator=HOLDING_OPERATOR;
    return 0;
  }


/* is it the call(object,"function") operator? */
  if (leftstr(s,4)=="call" && (midstr(s,5,1)==" " ||
                               midstr(s,5,1)=="(")) {
    len=strlen(s);
    loop=instr(s,1,"(");
    if (!loop) {
      error_rcpt.listen("@lock: couldn't find argument list for call()\n");
      destruct(this_object());
      return -1;
    }
    count=instr(s,loop+1,",");
    if (!count) {
      error_rcpt.listen("@lock: no second argument to call()\n");
      destruct(this_object());
      return -1;
    }
    arg1=midstr(s,loop+1,count-loop-1);
    while (leftstr(arg1,1)==" ") arg1=rightstr(arg1,strlen(arg1)-1);
    while (rightstr(arg1,1)==" ") arg1=leftstr(arg1,strlen(arg1)-1);
    arg1=error_rcpt.find_object(arg1);
    if (!arg1) {
      error_rcpt.listen("@lock: couldn't find first argument to call()\n");
      destruct(this_object());
      return -1;
    }
    loop=instr(s,count+1,"\"");
    if (!loop) {
      error_rcpt.listen("@lock: second argument to call() not string\n");
      destruct(this_object());
      return -1;
    }
    count=instr(s,loop+1,"\"");
    if (!count) {
      error_rcpt.listen("@lock: mismatched quotation marks\n");
      destruct(this_object());
      return -1;
    }
    arg2=midstr(s,loop+1,count-loop-1);
    loop=instr(s,count+1,")");
    if (!loop) {
      error_rcpt.listen("@lock: mismatched parentheses\n");
      destruct(this_object());
      return -1;
    }
    s=rightstr(s,strlen(s)-loop);
    while (leftstr(s,1)==" ") s=rightstr(s,strlen(s)-1);
    if (s) {
      error_rcpt.listen("@lock: trailing garbage\n");
      destruct(this_object());
      return -1;
    }
    if (!(error_rcpt.get_flags() & FLAG_PROGRAMMER)) {
      error_rcpt.listen("Permission denied.\n");
      destruct(this_object());
      return -1;
    }
    operator=CALL_OPERATOR;
    return 0;
  }

/* is it one of the gender() operators? */
  if (s=="gender(none)") {
    operator=GENDER_OPERATOR;
    arg1=GENDER_NONE;
    return 0;
  }
  if (s=="gender(male)") {
    operator=GENDER_OPERATOR;
    arg1=GENDER_MALE;
    return 0;
  }
  if (s=="gender(female)") {
    operator=GENDER_OPERATOR;
    arg1=GENDER_FEMALE;
    return 0;
  }
  if (s=="gender(neuter)") {
    operator=GENDER_OPERATOR;
    arg1=GENDER_NEUTER;
    return 0;
  }
  if (s=="gender(plural)") {
    operator=GENDER_OPERATOR;
    arg1=GENDER_PLURAL;
    return 0;
  }
  if (s=="gender(spivak)") {
    operator=GENDER_OPERATOR;
    arg1=GENDER_SPIVAK;
    return 0;
  }

/* oof, we've gotten down to an ATOMIC OBJECT, woo-hoo! */
  operator=TRADITIONAL_OPERATOR;
  arg1=error_rcpt.find_object(s);
  if (!arg1) {
    error_rcpt.listen("@lock: couldn't find atomic object\n");
    destruct(this_object());
    return -1;
  }
  return 0;
}

recycle_bool() {
  if (parent_object && caller_object()!=parent_object) return 0;
  if (operator==AND_OPERATOR || operator==OR_OPERATOR ||
      operator==NOT_OPERATOR || operator==PARENTHESIS_OPERATOR)
    arg1.recycle_bool();
  if (operator==AND_OPERATOR || operator==OR_OPERATOR)
    arg2.recycle_bool();
  destruct(this_object());
  return 1;
}

set_parent_object(po) {
  if (parent_object) return;
  parent_object=po;
  if (operator==AND_OPERATOR || operator==OR_OPERATOR ||
      operator==NOT_OPERATOR || operator==PARENTHESIS_OPERATOR)
    arg1.set_parent_object(this_object());
  if (operator==AND_OPERATOR || operator==OR_OPERATOR)
    arg2.set_parent_object(this_object());
}

eval(getter) {
  if (operator==AND_OPERATOR)
    return arg1.eval(getter) && arg2.eval(getter);
  if (operator==OR_OPERATOR)
    return arg1.eval(getter) || arg2.eval(getter);
  if (operator==NOT_OPERATOR)
    return !arg1.eval(getter);
  if (operator==TRADITIONAL_OPERATOR)
    if (arg1)
      return (getter==arg1 || location(arg1)==getter);
    else
      return 0;
  if (operator==PRESENT_OPERATOR)
    if (arg2)
      return present(arg1,arg2);
    else
      return 0;
  if (operator==HOLDING_OPERATOR)
    if (this_player())
      return present(arg1,this_player());
    else
      return 0;
  if (operator==CALL_OPERATOR)
    if (arg1)
      if (call_other(arg1,arg2))
        return 1;
      else
        return 0;
    else
      return 0;
  if (operator==GENDER_OPERATOR)
    return this_player().get_gender()==arg1;
  if (operator==PARENTHESIS_OPERATOR)
    return arg1.eval();
  return 1;
}
