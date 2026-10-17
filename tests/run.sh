#!/bin/bash
# Golden tests. Each tests/cases/<name>/site is built with
# `skyblue -i site -o out`, then compared with:
#
#   want/    the expected output tree (cases that build)
#   stderr   the expected diagnostics, exactly
#   fail     present if the build must fail and write nothing
#
# `tests/run.sh ./skyblue --update` rewrites want/ and stderr from the
# current output. Review the diff before keeping it.
bin=$(realpath "$1")
update=$2
cd "$(dirname "$0")"
failed=0

for dir in cases/*/; do
  dir=${dir%/}
  name=$(basename "$dir")
  (cd "$dir" && rm -rf out && NO_COLOR=1 SOURCE_DATE_EPOCH=0 "$bin" -i site -o out > /dev/null 2> got-stderr; echo $? > got-status)
  status=$(cat "$dir/got-status")

  if [ "$update" = --update ]; then
    mv "$dir/got-stderr" "$dir/stderr"
    rm -rf "$dir/want"
    [ -d "$dir/out" ] && mv "$dir/out" "$dir/want"
    rm -f "$dir/got-status"
    echo "updated $name"
    continue
  fi

  ok=1
  if [ -e "$dir/fail" ]; then
    [ "$status" != 0 ] || { echo "  expected the build to fail"; ok=; }
    [ ! -e "$dir/out" ] || { echo "  expected nothing to be written"; ok=; }
  else
    [ "$status" = 0 ] || { echo "  exit status $status"; ok=; }
    diff -r "$dir/want" "$dir/out" > /dev/null || { diff -r "$dir/want" "$dir/out" | head -20; ok=; }
  fi
  diff "$dir/stderr" "$dir/got-stderr" > /dev/null || { diff "$dir/stderr" "$dir/got-stderr" | head -20; ok=; }

  if [ -n "$ok" ]; then
    echo "testing $name ... ok"
    rm -rf "$dir/out" "$dir/got-stderr" "$dir/got-status"
  else
    echo "testing $name ... FAILED (output left in $dir)"
    failed=1
  fi
done

[ "$update" = --update ] && exit 0

./server.sh "$bin" || failed=1

[ $failed = 0 ] && echo OK
exit $failed
