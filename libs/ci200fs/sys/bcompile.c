/* bcompile.c */

static init() {
  if (!prototype(this_object())) destruct(this_object());
}

bcompile() {
  alarm(0,"docompile");
}

bsync() {
  alarm(0,"dosync");
}

static dosync() {
  itoo(0).sync();
}

static docompile() {
  compile_object("/boot");
}
