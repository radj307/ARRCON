
$MYDIR="$PSScriptRoot"

$INSTALL_DIR="out/install"
$WIN_EXT="windows-release/bin"
$LINUX_EXT="linux-release/bin"

$WIN_DIR="$MYDIR/$INSTALL_DIR/$WIN_EXT"
$LINUX_DIR="$MYDIR/$INSTALL_DIR/$LINUX_EXT"

cd "$WIN_DIR"
$win_ver = ./ARRCON.exe -vq
echo "[WINDOWS]"
echo "Reported Version: $win_ver"

cd "$LINUX_DIR"
$linux_ver = wsl ./ARRCON -vq
echo "[LINUX]"
echo "Reported Version: $linux_ver"

cd "$MYDIR"
echo "Done."
