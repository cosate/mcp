#!/bin/sh
set -e

NGINX_DIR="/home/cosate/code/nginx"
MCP_MODULE_DIR="/home/cosate/code/mcp/server"
INSTALL_PREFIX="/home/cosate/code/ai.com"

CXX="g++"
CXXFLAGS="-std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -Werror -g -I/home/cosate/code/mcp/third_party"

echo "[1] 进入 nginx 目录"
cd "$NGINX_DIR"

# 若已构建过，清理
if [ -f Makefile ]; then
  echo "[1.5] 清理旧构建"
  make clean
fi

echo "[2] configure (忽略重复执行错误)"
./auto/configure --prefix="$INSTALL_PREFIX" --add-module="$MCP_MODULE_DIR" || echo "[INFO] configure 已执行过"

MFILE="objs/Makefile"
[ -f "$MFILE" ] || { echo "[FATAL] 缺少 $MFILE"; exit 1; }

echo "[3] 备份 -> $MFILE.backup"
cp "$MFILE" "$MFILE.backup"

echo "[4] 应用最小补丁 (模拟 diff 差异)"

# 4.1 插入 CXX / CXXFLAGS
if ! grep -q '^CXX[[:space:]]*=' "$MFILE"; then
  sed -i "/^CC[[:space:]]*=/i CXX = ${CXX}\nCXXFLAGS = ${CXXFLAGS}\n" "$MFILE"
  echo "    [+] 插入 CXX / CXXFLAGS"
else
  echo "    [=] 已存在 CXX，跳过"
fi

# 4.2 修改 LINK = $(CC) -> LINK = $(CXX)
if grep -q '^[[:space:]]*LINK[[:space:]]*=[[:space:]]*\$(CC)' "$MFILE"; then
  sed -i 's/^[[:space:]]*LINK[[:space:]]*=[[:space:]]*\$(CC)/LINK = $(CXX)/' "$MFILE"
  echo "    [+] LINK 改为 \$(CXX)"
else
  echo "    [=] LINK 已无需修改"
fi

# 4.3 替换 .cpp 目标的下一行编译命令
TMPF="$MFILE.tmp.$$"
awk '
  /[.]cpp/ {
    print; getline nx
    gsub(/\$\(CC\)/,"$(CXX)",nx)
    gsub(/CFLAGS/,"CXXFLAGS",nx)
    print nx
    next
  }
  { print }
' "$MFILE" > "$TMPF" && mv "$TMPF" "$MFILE"
echo "    [+] 已处理 .cpp 编译命令 (若存在)"

echo "[5] 关键变量预览"
grep -E '^(CC|CXX|CXXFLAGS|LINK)[[:space:]]*=' "$MFILE" || true

echo "[6] 构建"
if command -v nproc >/dev/null 2>&1; then
  JOBS="$(nproc)"
else
  JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
fi
make -j"$JOBS"

echo "[7] 安装 -> $INSTALL_PREFIX"
make install

echo "[DONE] 完成：nginx + C++17 模块已构建并链接。"
