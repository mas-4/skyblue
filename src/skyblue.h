// skyblue: a static site generator, structured as a small compiler.
//
//   lex.dcdx     source text -> tokens
//   parse.dcdx   tokens -> documents (ASTs)
//   render.dcdx  documents -> HTML (includes, templates, variables, listings)
//   site.dcdx    finds the input files, builds every page, writes the output
//   server.dcdx  development server that rebuilds when the input changes
//
// Everything is allocated from one arena. A build never frees memory;
// the dev server throws the whole arena away before each rebuild.

#define _DEFAULT_SOURCE
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct Arena Arena;
typedef struct Chunk Chunk;
typedef struct Str Str;
typedef struct Buf Buf;
typedef struct Map Map;
typedef struct MapEntry MapEntry;
typedef struct Source Source;
typedef struct Loc Loc;
typedef struct Attr Attr;
typedef struct Token Token;
typedef struct Node Node;
typedef struct Doc Doc;
typedef struct Slot Slot;
typedef struct Render Render;
typedef struct Output Output;
typedef struct Site Site;
typedef struct Config Config;

//
// base.dcdx: memory, strings, maps, files
//

struct Arena {
  Chunk *chunks;

  void *alloc(long size); // zeroed
  void reset(void);       // free everything
};

extern Arena arena;
void *alloc(long size);

// An immutable slice of bytes. Not NUL-terminated. A Str whose ptr is
// NULL means "absent", as opposed to the empty string.
struct Str {
  char *ptr;
  long len;

  bool eq(Str s);
  bool eqz(char *s);
  int cmp(Str s);
  bool starts_with(char *prefix);
  bool ends_with(char *suffix);
  long find(char *needle, long from);  // -1 if not found
  long find_char(char c, long from);   // -1 if not found
  long rfind_char(char c);             // -1 if not found
  Str slice(long from, long to);       // [from, to), clamped
  Str from(long i);
  Str trim();
  bool is_blank();
  bool is_null();
  char *cstr(); // a NUL-terminated copy
};

// A string literal as a Str.
#define S(lit) ((Str){lit, sizeof(lit) - 1})

// For printf: printf("%.*s", SF(s))
#define SF(s) (int)(s).len, (s).ptr

Str str(char *s);
Str strf(char *fmt, ...);

// A growable byte buffer.
struct Buf {
  char *ptr;
  long len;
  long cap;

  void push(Str s);
  void pushz(char *s);
  void pushc(int c);
  void fmt(char *fmt, ...);
  void vfmt(char *fmt, va_list ap);
  void html(Str s); // push with HTML special characters escaped
  Str str();
};

struct MapEntry {
  Str key;
  void *val;
};

// A hash map from Str to pointers.
struct Map {
  MapEntry *slots;
  long cap;
  long len;

  void *get(Str key);
  void put(Str key, void *val);
};

Str path_join(Str a, Str b);
Str path_dir(Str path);
Str path_clean(Str path);
Str path_rel(Str root, Str path); // `path` relative to directory `root`
bool is_dir(Str path);
bool is_file(Str path);
Str *list_dir(Str dir, long *n); // sorted entry names, without . and ..
bool read_file(Str path, Str *out);
bool write_file(Str path, Str data);
bool copy_file(Str from, Str to);
bool make_dirs(Str dir);

//
// diag.dcdx: diagnostics and logging
//

struct Source {
  Str path; // as shown in messages, e.g. "site/blog.html"
  Str text;
  bool warned_legacy;
};

Source *new_source(Str path, Str text);

// A position in a source file.
struct Loc {
  Source *src;
  long off;

  void error(char *fmt, ...);
  void warn(char *fmt, ...);
};

void error(char *fmt, ...);
void warn(char *fmt, ...);
void info(char *fmt, ...);
void debug(char *fmt, ...);
void reset_diags(void);

extern int nerrors;
extern int nwarnings;
extern Buf diag_log; // every diagnostic, uncolored, for the dev server

//
// lex.dcdx
//

typedef enum {
  TK_TEXT,    // literal HTML
  TK_VAR,     // ${meta.key}, ${frontmatter.key} or ${post.key}
  TK_TAG,     // <sky-name attr="value"> or <sky-name ... />
  TK_END_TAG, // </sky-name>
  TK_EOF,
} TokenKind;

struct Attr {
  Attr *next;
  Loc loc;
  Str name;
  Str value;
  bool used;
};

struct Token {
  Token *next;
  TokenKind kind;
  Loc loc;
  Str text;  // TK_TEXT: the text. TK_TAG/TK_END_TAG: name without prefix.
             // TK_VAR: the namespace.
  Str key;   // TK_VAR
  Attr *attrs;
  bool self_closing;

  Str attr(char *name);  // absent (is_null) if missing; marks it used
  void check_attrs();    // warn about attributes that were never used
};

Token *lex(Source *src);

//
// parse.dcdx
//

typedef enum {
  ND_TEXT,       // literal HTML
  ND_VAR,        // ${ns.key}
  ND_INCLUDE,    // <sky-include path>
  ND_MD_INCLUDE, // <sky-md-include path>
  ND_SLOT,       // <sky-slot name>
  ND_LIST,       // <sky-markdown directory template [item] ...>
  ND_BLOCK,      // <sky-block name> ... </sky-block>
} NodeKind;

struct Node {
  Node *next;
  NodeKind kind;
  Loc loc;

  Str text;  // ND_TEXT
  Str ns;    // ND_VAR
  Str key;   // ND_VAR
  Str path;  // ND_INCLUDE, ND_MD_INCLUDE; ND_LIST: the directory
  Str name;  // ND_SLOT, ND_BLOCK; ND_LIST: slot the post goes into
  Node *body; // ND_BLOCK

  // ND_LIST
  Str templ;
  Str item;  // template for each entry of the list, or absent
  Str sort;
  Str title;
  bool descending;
};

// A parsed HTML file: a page, a template, or an include.
struct Doc {
  Source *src;
  Node *body;    // in a templated doc, only ND_BLOCK nodes
  Str templ;     // <sky-template path>, or absent
  Loc templ_loc;
  bool active;   // being rendered right now (to catch cycles)
};

Doc *parse(Source *src);

//
// markdown.dcdx
//

Str split_frontmatter(Source *src, Map *fm); // fills fm, returns the body
Str markdown(Str text);

//
// render.dcdx
//

struct Slot {
  Str html;
  Loc loc;   // the block that filled it
  bool used;
};

struct Render {
  Site *site;
  Buf *out;
  Map *fm;        // frontmatter, while rendering a markdown post
  Str fm_path;    // the markdown file, for messages
  Map *post;      // ${post.*}: url and title, while rendering a post
  Map *slots;     // slot name -> Slot*, while rendering a template

  bool doc(Doc *doc); // false if the template couldn't be rendered
  void nodes(Doc *doc, Node *node);
  void node(Doc *doc, Node *node);
  void var(Node *node);
  void include(Doc *doc, Node *node);
  void md_include(Doc *doc, Node *node);
  void slot(Node *node);
  void list(Doc *doc, Node *node);
};

Str render_page(Site *site, Doc *doc);

//
// site.dcdx
//

struct Output {
  Output *next;
  Str path;      // relative to the output directory
  Str content;   // rendered pages
  Str copy_from; // static files: the source file
  Str origin;    // the file this came from
  Str templ;     // markdown posts: the template used
};

struct Site {
  Str root;      // the input directory
  Str outdir;    // skipped while walking the input, if it's inside it
  Map meta;      // key -> Str*
  Map docs;      // path -> Doc*
  Map outputs;   // output path -> Output*
  Output *first;
  Output *last;
  int npages;
  int nfiles;

  void build();
  void walk(Str dir);
  void load_meta();
  Source *read(Str path, Loc loc);
  Doc *load(Str path, Loc loc);
  Str resolve(Doc *from, Str path, Loc loc, bool imports);
  Output *find(Str path);
  Output *add(Str path, Str origin, Loc loc);
  bool write(Str outdir);
};

Site *build_site(char *input, char *output);

//
// server.dcdx
//

void serve(char *input, char *output, char *addr);

//
// main.dcdx
//

struct Config {
  char *input;
  char *output;
  char *serve;   // host:port, or NULL to build
  bool verbose;
};

extern Config config;
