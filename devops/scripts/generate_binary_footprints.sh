#!/bin/bash
# Generates the binary footprints used to check for binary size issues
# By default, uses the latest published package (prod/insiders-fast) unless
# regex filter is given.
# Usage: ./generate_binary_footprint [filter]
if ! [ -x "$(command -v jq)" ]; then
  echo 'Error: jq is not installed.' >&2
  exit 1
fi

# Absolute path to this script
SCRIPT=$(readlink -f $0)
SCRIPTPATH=`dirname $SCRIPT`
BaseDir=`realpath $SCRIPTPATH/../binary-footprint-baseline`

distros=("ubuntu-20.04", "debian-9")
distroPathUbuntu2004="https://packages.microsoft.com/ubuntu/20.04/prod/pool/main/o/osconfig/"
distroPathDebian9="https://packages.microsoft.com/debian/9/multiarch/prod/pool/main/o/osconfig/"
JSON="[]"
for distro in ${distros[@]}; do distro=$(echo $distro | tr -d ',');

  case $distro in
    ("ubuntu-20.04") distroPath=$distroPathUbuntu2004;;
    ("debian-9")     distroPath=$distroPathDebian9;;
  esac

  for arch in "x86_64", "aarch64", "armv7l"; do arch=$(echo $arch | tr -d ',');
    wget -q -O tmp.html $distroPath
    size=$((tr -s ' ' <<< `cat tmp.html | grep -E ">osconfig.*$1$arch.deb<" | tail -1`) | cut -d ' ' -f 5 | tr -d '\r\n')
    file=$((tr -s ' ' <<< `cat tmp.html | grep -E ">osconfig.*$1$arch.deb<" | tail -1`) | grep -o '>osconfig.*<' | tr -d '<>')
    rm tmp.html
    echo "Generating binary footprint for $file ($distro/$arch): $size bytes"
    json="[{ \"distro\": \"$distro\", \"arch\": \"$arch\", \"sizeBytes\": $size, \"file\": \"$file\"}]"
    JSON=$(echo $JSON | jq --argjson packageInfo "$json" '. |= . + $packageInfo')
  done
done

echo $JSON | jq '.' > $BaseDir/baseline.json
