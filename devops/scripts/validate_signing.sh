#!/bin/sh
# Validate Microsoft Release Signing Keys
# Requires: curl, ar (build-essentials) and gpg
if [ -z "$1" ]; then
    echo 'Error: missing file input.\n./validate_signing.sh <input-file>' >&2
    exit 1
fi
if ! [ -f "$1" ]; then
    echo "input-file '$1' does not exist." >&2
    exit 1
fi
if ! [ -x "$(command -v curl)" ]; then
  echo 'Error: curl is not installed.' >&2
  exit 1
fi
if ! [ -x "$(command -v ar)" ]; then
  echo 'Error: ar is not installed.' >&2
  exit 1
fi
if ! [ -x "$(command -v gpg)" ]; then
  echo 'Error: gpg is not installed.' >&2
  exit 1
fi
trap 'rm -rf "$tempdir"' EXIT
tempdir=`mktemp -d`
key=$tempdir/key.asc

# Download and import the Microsoft release signing key
curl -sSL https://packages.microsoft.com/keys/microsoft.asc --output $key
gpg --import $key

# Extract Debian file
ar -x $1 --output $tempdir

# Combine debian-binary, control.tar.gz, data.tar.gz
cat $tempdir/debian-binary $tempdir/control.tar.gz $tempdir/data.tar.gz > $tempdir/combined-contents

# Verify keys
gpg --verify $tempdir/_gpgorigin $tempdir/combined-contents
[ $? -eq 0 ] && echo "\nVALID Microsoft Signature" || (echo "\nINVALID Microsoft Signature!" >&2 && exit 1)
