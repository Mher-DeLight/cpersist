#!/bin/sh
set -eu

OS="$(uname -s)"
case "$OS" in
  Linux)
    echo "Detected Linux system"
    ;;
  *)
    echo "Your Operating System doesn't support release installation. Please refer to the README.md to install from HEAD."
    exit 1
    ;;
esac

if [ "$#" -gt 1 ]; then
  echo "Usage: $0 [project-directory]" >&2
  exit 1
fi

if [ "$#" -eq 1 ]; then
  PROJECT_DIR="$1"
else
  PROJECT_DIR="$(pwd)"
fi

REPO="Mher-DeLight/cpersist"

echo "Fetching latest release..."

API_URL="https://api.github.com/repos/$REPO/releases"

RELEASE_URL=$(
  curl -fsSL "$API_URL" |
    grep '"browser_download_url":' |
    grep '\.tar\.gz"' |
    head -n 1 |
    sed 's/.*"browser_download_url": "\(.*\)".*/\1/'
)

if [ -z "$RELEASE_URL" ]; then
  echo "Could not find a release asset." >&2
  exit 1
fi

echo "Downloading latest release..."

TMP_FILE="$(mktemp)"
trap 'rm -f "$TMP_FILE"' EXIT

curl -fL "$RELEASE_URL" -o "$TMP_FILE"

mkdir -p "$PROJECT_DIR/include/cpersist"
tar -xzf "$TMP_FILE" -C "$PROJECT_DIR/include/cpersist"

echo "Installed to $PROJECT_DIR/include/cpersist"
echo "Add the following lines in the appropriate section of your CMakeLists.txt"
echo " ===== "
echo "set(CMAKE_CXX_STANDARD 20)"
echo "set(CMAKE_CXX_STANDARD_REQUIRED ON)"
echo "set(CMAKE_CXX_EXTENSIONS OFF)"
echo
echo "find_package(OpenSSL REQUIRED)"
echo "target_link_libraries(your_cmake_target PRIVATE \${CMAKE_SOURCE_DIR}/include/cpersist/src/cpersist.a OpenSSL::SSL OpenSSL::Crypto)"
echo "target_include_directories(your_cmake_target PRIVATE \${CMAKE_SOURCE_DIR}/include/cpersist/include)"
echo " ===== "
