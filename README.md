# skyblue

I'm sick of ultra complex static site generators.

Especially when the functionality could be easily reproduced if html
implemented these basic features:

1. HTML includes (just like #include in c/cpp, drop the included file
   wholecloth into the including file)
2. HTML templates (the reverse of #include, drop the templated file directly
   into the template, using named slots)
3. HTML markdown (create a template for each markdown file, generate a list
   of links, and convert every file within a directory into an html page
   using the template)

The closest thing to doing just that I've found is [Sergey](https://sergey.cool/)
which is a nice little project written as a single file of JavaScript.

That's cool and all, and I use it, but its JavaScript (🤮😜).

It used to be C++, which is also gross. Now it's written in c′, a tiny
derivative of C. And, you know, my dog died, and I wanted to make something
named after her. Pic coming soon.

So, this is for you, Sky Blue.

# Building

skyblue is written in c′ and compiled with
[cprime](https://github.com/mas-4/cprime), the c′ compiler. Install cprime
(or check it out next to this repository), then:

```
make                              # builds ./skyblue
make test                         # golden tests and dev server tests
sudo make install                 # /usr/local/bin/skyblue
make install PREFIX=~/.local      # or ~/.local/bin/skyblue
```

The Makefile uses `../cprime/cprime` if it exists, otherwise `cprime` from
your `PATH`; `make CPRIME=/path/to/cprime` overrides it. cprime targets
x86-64 Linux.

# Usage

To build a site:

```
skyblue -i ./src -o ./build
```

The output directory defaults to `./build`. If anything is wrong, skyblue
prints every problem it finds and writes nothing:

```
src/blog.html:3:5: error: can't find 'nav.html' (looked for src/nav.html and src/_imports/nav.html)
    3 |     <sky-include path="nav.html" />
      |     ^
```

To use the development server:

```
skyblue -i ./src -s                    # http://127.0.0.1:8080
skyblue -i ./src -h 0.0.0.0:3000
```

The server rebuilds the site whenever a file changes. If the build fails,
every page shows the errors until you fix them. `/blog` serves `blog.html`,
like the `.htaccess` file skyblue writes for Apache.

`-v` is verbose mode, which I find useful.

# Syntax

## Include

```html
<sky-include path="nav.html" />
```

Drops `nav.html` in place and renders it too, so includes can include other
includes. Paths are relative to the file they're in. If the file isn't
there, skyblue looks in `_imports/`, so `<sky-include path="nav.html" />`
works from any page. A path starting with `/` is relative to the site's root.

## Markdown include

```html
<sky-md-include path="hello.md" />
```

Renders `hello.md` as HTML and drops it in place. Frontmatter is stripped.

## Templates

```html
<sky-template path="template.html" />
<sky-block name="content">
   Hello, world!
</sky-block>
```

Will render with this template:

```html
<!DOCTYPE html>
<body>
    <sky-slot name="content" />
</body>
```

into

```html
<!DOCTYPE html>
<body>
    Hello, world!
</body>
```

A template can use a template too. To pass a slot through a layout, put a
`<sky-slot>` inside one of its blocks:

```html
<!-- _imports/article.html -->
<sky-template path="base.html" />
<sky-block name="body">
    <article><sky-slot name="content" /></article>
</sky-block>
```

## Markdown

```html
<sky-template path="template.html" />
<sky-block name="content">
    <sky-markdown
       directory="posts"
       template="post.html"
       sort="date"
       order="descending"
    />
</sky-block>
```

Every `.md` file in `posts/` becomes a page, rendered into the `content`
slot of `post.html`, and the tag is replaced with a list of links to them:

```html
<ul>
<li><a href="/posts/hello">Hello</a></li>
</ul>
```

| Attribute   | Default     | |
|-------------|-------------|-|
| `directory` | (required)  | relative to this file |
| `template`  | (required)  | the template for each post |
| `slot`      | `content`   | the slot the post goes in |
| `sort`      | file name   | a frontmatter key to sort by |
| `order`     | `ascending` | or `descending` |
| `title`     | `title`     | the frontmatter key for the link text |
| `item`      | a `<ul>`    | a template for each entry of the list |

Two pages can list the same directory with the same template (say, the blog
page and the home page); the posts are only built once.

For more than a list of links, give it an `item` template. It's rendered
once per post, in place of the `<ul>`, with that post's frontmatter:

```html
<div class="cards">
  <sky-markdown directory="projects" template="project.html" item="card.html" />
</div>
```

```html
<!-- _imports/card.html -->
<a class="card" href="${post.url}">
  <img src="${frontmatter.image}" alt="" />
  <h2>${post.title}</h2>
</a>
```

Frontmatter goes at the very top of a markdown file:

```
---
title: Hello
date: "2024-01-31"
---
```

Markdown supports tables, strikethrough, task lists and autolinks.

## Source listings

`<sky-source>` shows a file as source code: HTML-escaped, highlighted,
and one `<span class="line" data-line="N">` per line, so CSS can number
the lines. It's meant to go inside a `<pre>`:

```html
<pre><code><sky-source path="../skyblue/src/render.dcdx" from="void Render.slot" to="}" /></code></pre>
```

`from` starts the listing at the first line that begins with it (ignoring
indentation), and `to` ends it at the next line, indented the same, that
begins with it, so the example is one function. Without `from` it's the
whole file; without `to` it runs to the end. Paths can be outside the site.

`<sky-sources item="source.html" />` lists every file the page was built
from, in the order skyblue used them: the page (or markdown post), its
templates, its includes, and the item template itself. Each one is
rendered with the item template, which gets `${source.path}` (relative to
the site), `${source.code}` (the whole file, as `<sky-source>` would show
it) and `${source.lines}`:

```html
<details>
  <summary>${source.path}, ${source.lines} lines</summary>
  <pre><code>${source.code}</code></pre>
</details>
```

Put it in your main template and every page carries its own source.

Templates are highlighted with skyblue's own lexer, C and c′ with a small
highlighter, shell scripts and makefiles for comments and strings, and
markdown frontmatter by key. The spans' classes are
`sky`, `var`, `tag`, `str`, `com`, `kw`, `type`, `ego`, `pp` and `num`.

## Variables

Finally, you can use variables from a `meta` file in the root of the site
anywhere:

```
site_title=skyblue
# comments and blank lines are fine
```

```html
<h1>${meta.site_title}</h1>
```

will render

```html
<h1>skyblue</h1>
```

and in the template for markdown posts you can use the frontmatter variables
like so:

```html
<h1>${frontmatter.title}</h1>
```

Post templates and `item` templates also get `${post.url}` (like
`/projects/maudlin`) and `${post.title}` (the link text).

`${sky.…}` is about skyblue and the build, and works anywhere:

| Variable | Value |
|---|---|
| `${sky.version}` | the commit skyblue was built from |
| `${sky.compiler}` | the version of cprime that compiled skyblue |
| `${sky.date}`, `${sky.time}` | when the site was built: `2026-10-17`, `18:22 UTC` (or [`SOURCE_DATE_EPOCH`](https://reproducible-builds.org/specs/source-date-epoch/)) |
| `${sky.pages}`, `${sky.files}` | how many pages were rendered and files copied |
| `${sky.ms}` | how many milliseconds the build took |

Only `${meta.…}`, `${frontmatter.…}`, `${post.…}`, `${source.…}` and
`${sky.…}` are variables, so JavaScript template literals like
`` `${user.name}` `` are left alone. To write a literal
`${meta.x}`, write `$${meta.x}`. Variables aren't escaped, so values can
contain HTML.

## What gets published

| In the source | In the output |
|---|---|
| `*.html` | rendered |
| `*.md` | only through `<sky-markdown>` |
| `meta`, `_imports/` | nothing |
| hidden files | nothing, except `.htaccess` and `.well-known/` |
| everything else (CSS, JS, images…) | copied as is |

If you don't have an `.htaccess`, skyblue writes one that serves `/blog`
from `blog.html`.

Nothing inside an HTML comment is processed, so you can comment out a tag.

And that's all there is to it.

# Coming from bluesky

skyblue used to be called bluesky. Old sites still build:

- `<bluesky-…>` tags still work, with a warning. Run
  `sed -i 's/<\(\/\?\)bluesky-/<\1sky-/g'` on your HTML files to update them.
- `name=` still works wherever `path=` is expected.

Some things work differently because they were bugs:

- Frontmatter in an md-include file is no longer rendered as text.
- Markdown tables, strikethrough and task lists actually work now.
- CSS and JS files are copied as they are. They used to go through the
  template engine, which mangled `${…}` in JavaScript.
- Errors stop the build, instead of producing a half-rendered site.
- Include cycles are reported instead of crashing.
