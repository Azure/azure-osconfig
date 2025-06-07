#!/bin/sh

usage()
{
  echo "Usage: $0 [ -b | --basedir BASEDIR ] [ -d | --distro DISTRO ] [ -u | --builddir BUILDDIR ]
                  [ -x | --no-docker-build ] [ -n | --no-build ]"
  echo "By default BASEDIR is current directory, all distros are tested"
  echo "Example usage: ./devops/scripts/run-module-tests.sh -d ubuntu-22.04-amd64 -u build"
  exit 2
}

PARSED_ARGUMENTS=$(getopt -a -n $0 -o b:d:u:xn --long basedir:,distro:,builddir:,no-docker-build,no-build -- "$@")
VALID_ARGUMENTS=$?

if [ "$VALID_ARGUMENTS" != "0" ]; then
  usage
fi

BASEDIR=`pwd`

eval set -- "$PARSED_ARGUMENTS"
while :
do
  case "$1" in
    -b | --basedir)          BASEDIR="$2"      ; shift 2 ;;
    -d | --distro)           DISTRO="$2"       ; shift 2 ;;
    -u | --builddir)         BUILDDIR="$2"       ; shift 2 ;;
    -x | --no-docker-build)  NODOCKERBUIILD=1  ; shift ;;
    -n | --no-build)         NOBUILD=1         ; shift ;;
    --) shift; break ;;
    *) echo "Unexpected option: $1 - this should not happen."
       usage ;;
  esac
done


if [ -z "$DISTRO" ]; then
	DISTROS=$(ls -d $BASEDIR/devops/docker/*)
else
	DISTROS="$BASEDIR/devops/docker/$DISTRO"
fi


for DISTRO in $DISTROS; do
  DISTRO_NAME=$(basename $DISTRO)
  IMGNAME=moduletest-$DISTRO_NAME:latest
  if [ -z "$NODOCKERBUILD" ]; then
  	docker build -t $IMGNAME $DISTRO
  fi
  if [ -z "$BUILDDIR" ]; then
  	BDIR="build-$DISTRO_NAME"
  else
  	BDIR="$BUILDDIR"
  fi
  docker run -v $BASEDIR:/git/azure-osconfig -w /git/azure-osconfig $IMGNAME ./devops/scripts/run-module-tests-int.sh $BDIR $NOBUILD
done
