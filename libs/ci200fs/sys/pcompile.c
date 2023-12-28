object player_who_called;

static init() {
  if (!prototype(this_object())) destruct(this_object());
}

pcompile() {
  alarm(0,"docompile");
  player_who_called=caller_object();
}

static docompile() {
  object o;

  o=compile_object("/obj/player");
  if (!player_who_called) return;
  if (o)
    player_who_called.listen("Object #"+itoa(otoi(o))+" compiled.\n");
  else
    player_who_called.listen("Compile failed.\n");
}
