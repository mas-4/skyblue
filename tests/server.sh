#!/bin/bash
# The development server: routing, live rebuilds and the error page.
bin=$1
tmp=$(mktemp -d /tmp/skyblue-test-XXXXXX)
port=$((20000 + RANDOM % 20000))
pid=

cleanup() {
  [ -n "$pid" ] && kill $pid 2> /dev/null
  rm -rf "$tmp"
}
trap cleanup EXIT

mkdir -p "$tmp/site/_imports" "$tmp/site/blog"
printf 'title=Served\n' > "$tmp/site/meta"
printf '<h1>${meta.title}</h1>\n' > "$tmp/site/index.html"
printf 'blog\n' > "$tmp/site/blog.html"
printf 'post\n' > "$tmp/site/blog/post.html"
printf 'body {}\n' > "$tmp/site/style.css"

NO_COLOR=1 "$bin" -h 127.0.0.1:$port -i "$tmp/site" 2> "$tmp/log" &
pid=$!
for i in $(seq 50); do
  curl -s -o /dev/null "http://127.0.0.1:$port/" && break
  sleep 0.1
done

failed=0
check() { # check <name> <expected> <actual>
  if [ "$2" = "$3" ]; then
    echo "testing server: $1 ... ok"
  else
    echo "testing server: $1 ... FAILED: expected '$2', got '$3'"
    failed=1
  fi
}
get() { curl -s "http://127.0.0.1:$port$1"; }
code() { curl -s -o /dev/null -w '%{http_code} %{content_type}' "http://127.0.0.1:$port$1"; }

check 'index' '<h1>Served</h1>' "$(get /)"
check '/blog' 'blog' "$(get /blog)"
check '/blog/' 'blog' "$(get /blog/)"
check '/blog/post' 'post' "$(get /blog/post)"
check '/blog/post.html' 'post' "$(get /blog/post.html)"
check 'css type' '200 text/css; charset=utf-8' "$(code /style.css)"
check 'query string' '<h1>Served</h1>' "$(get '/?x=1')"
check 'percent-encoding' 'post' "$(get /blog/%70ost)"
check '404' '404 text/html; charset=utf-8' "$(code /nope)"
check 'meta is private' '404 text/html; charset=utf-8' "$(code /meta)"
check 'HEAD' '200' "$(curl -s -I -o /dev/null -w '%{http_code}' "http://127.0.0.1:$port/")"
check 'POST' '405' "$(curl -s -o /dev/null -w '%{http_code}' -X POST "http://127.0.0.1:$port/")"

sleep 0.05
printf '<p>new</p>\n' > "$tmp/site/new.html"
check 'new file' '<p>new</p>' "$(get /new)"

printf 'title=Changed\n' > "$tmp/site/meta"
check 'edited meta' '<h1>Changed</h1>' "$(get /)"

printf '<sky-include path="missing.html" />\n' > "$tmp/site/broken.html"
check 'build error' '500 text/html; charset=utf-8' "$(code /)"
get / | grep -q "can&#39;t find &#39;missing.html&#39;"
check 'error page shows it' 0 $?

rm "$tmp/site/broken.html"
check 'fixed' '<h1>Changed</h1>' "$(get /)"

exit $failed
