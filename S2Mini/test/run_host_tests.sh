#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
test_build=$(mktemp -d)
trap 'rm -rf "$test_build"' EXIT
for name in protocol initialization sync_guard brightness rgb_status sony_status; do
  c++ -std=c++17 -fsanitize=address,undefined -g -I include -I lib/ATEMbase "test/$name.cpp" -o "$test_build/$name"
  "$test_build/$name"
done
c++ -std=c++17 -fsanitize=address,undefined -g -I test/oled_host -I test/host -I include \
  test/oled_status.cpp -o "$test_build/oled_status"
"$test_build/oled_status"
c++ -std=c++17 -DESP32 -fsanitize=address,undefined -g \
  -I test/host -I lib/ATEMbase -I lib/ATEMmin \
  test/transport.cpp lib/ATEMbase/ATEMbase.cpp lib/ATEMmin/ATEMmin.cpp -o "$test_build/transport"
"$test_build/transport"
