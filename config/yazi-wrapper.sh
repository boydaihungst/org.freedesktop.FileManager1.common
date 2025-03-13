#!/usr/bin/env bash

# $1 = BDUS_METHOD Can be:
#
# ShowFolders assumes that the specified URIs are folders; the file manager is supposed to show a window with the contents of each folder. Calling this method with file:///etc as the single element in the array of URIs will cause the file manager to show the contents of /etc as if the user had navigated to it. The behavior for more than one element is left up to the implementation; commonly, multiple windows will be shown, one for each folder.
#
# ShowItems doesn't make any assumptions as to the type of the URIs. The file manager is supposed to select the passed items within their respective parent folders. Calling this method on file:///etc as the single element in the array of URIs will cause the file manager to show a file listing for "/", with "etc" highlighted. The behavior for more than one element is left up to the implementation.
#
# ShowItemProperties should cause the file manager to show a "properties" window for the specified URIs. For local Unix files, these properties can be the file permissions, icon used for the files, and other metadata.

# Read more here: https://www.freedesktop.org/wiki/Specifications/file-manager-interface/

BDUS_METHOD="$1"
shift 1 # skip BDUS_METHOD
declare -a item

quote_string() {
  local input="$1"
  echo "'${input//\'/\'\\\'\'}'"
}

# decode url string + add quote
for arg in "$@"; do
  decoded_arg=$(printf '%b' "${arg//%/\\x}")
  item+=("$(quote_string "$decoded_arg")")
done

cmd="/usr/bin/yazi"
termcmd="${TERMCMD:-/usr/bin/kitty --class yazi}"

case "$BDUS_METHOD" in
# Since yazi can handle both files, folders
"ShowFolders" | "ShowItems")
  eval "$termcmd -- $cmd ${item[@]}" &
  disown
  ;;
"ShowItemProperties")
  YAZI_ID=999999
  eval "$termcmd -- $cmd --client-id $YAZI_ID ${item[@]}" &
  disown
  # Increase this if yazi take too long to load
  sleep 0.5
  ya emit-to $YAZI_ID spot
  ;;
esac
