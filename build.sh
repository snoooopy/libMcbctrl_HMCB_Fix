#!/bin/bash
# libmcbctrl 一键编译（Ubuntu 20.04/22.04/24.04 通用，g++ 9~13 均可）
# 用法：在本目录执行 ./build.sh
# 产物：libmcbctrl.so.1.0.0.131（旧产物自动备份为 .bak）
#
# 说明：
# - 三个排除项：RequestSocket_qt（Qt 变体符号冲突）/ StdAfx（PCH 桩）/
#   ECS_funcnew（与 ECS_func 声明冲突）
# - -Wl,-soname：无 SONAME 时 gcc9/cmake3.16 链接产物会在 NEEDED 记相对
#   路径，ctest 从 build/tests 跑即断链；加 SONAME 后记纯文件名由 RUNPATH 定位
# - GCC9 的 std::filesystem 在独立库须 -lstdc++fs（GCC10+ 已并入 libstdc++）
set -euo pipefail
cd "$(dirname "$0")"

command -v g++ >/dev/null 2>&1 || { echo "错误：缺 g++（sudo apt install build-essential）" >&2; exit 1; }
[ -e /usr/include/boost/asio.hpp ] || { echo "错误：缺 boost 头文件（sudo apt install libboost-dev）" >&2; exit 1; }

OUT=libmcbctrl.so.1.0.0.131
[ -f "$OUT" ] && cp -a "$OUT" "$OUT.bak" && echo "旧产物备份为 $OUT.bak"

# GCC < 10 需显式链 stdc++fs
EXTRA=()
GCC_MAJOR="$(g++ -dumpversion | cut -d. -f1)"
if [ "$GCC_MAJOR" -lt 10 ]; then EXTRA+=(-lstdc++fs); fi

g++ -shared -fPIC -std=c++17 -O2 \
    $(ls src/*.cpp | grep -vE "RequestSocket_qt|StdAfx|ECS_funcnew") \
    -Isrc -Wl,-soname,"$OUT" \
    -o "$OUT" -lpthread "${EXTRA[@]}"

echo "编译完成：$OUT（$(stat -c%s "$OUT") 字节，g++ $(g++ -dumpversion)）"
