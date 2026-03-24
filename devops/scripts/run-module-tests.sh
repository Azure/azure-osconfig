#!/bin/bash
set -euo pipefail

usage()
{
  echo "Usage: $0 [ -b | --basedir BASEDIR ] [ -d | --distro DISTRO ] [ -u | --builddir BUILDDIR ]
                  [ -x | --no-docker-build ] [ -n | --no-build ]"
  echo "By default BASEDIR is current directory, all distros are tested"
  echo "Example usage: ./devops/scripts/run-module-tests.sh -d ubuntu-22.04-amd64 -u build"
  exit 2
}

PARSED_ARGUMENTS=$(getopt -a -n "$0" -o b:d:u:xnh --long basedir:,distro:,builddir:,no-docker-build,no-build,help -- "$@")

BASEDIR=`pwd`
NODOCKERBUIILD=''
DISTRO=''
NOBUILD=''
BUILDDIR=''
eval set -- "$PARSED_ARGUMENTS"
while true
do
  case "$1" in
    -b | --basedir)          BASEDIR="$2"      ; shift 2 ;;
    -d | --distro)           DISTRO="$2"       ; shift 2 ;;
    -u | --builddir)         BUILDDIR="$2"       ; shift 2 ;;
    -x | --no-docker-build)  NODOCKERBUIILD=1  ; shift ;;
    -n | --no-build)         NOBUILD=1         ; shift ;;
    -h | --help)             usage             ; shift ;;
    --) shift; break ;;
    *) echo "Unexpected option: $1 - this should not happen."
       usage ;;
  esac
done

if [ $# -ne 0 ] ; then
    echo "Unexpected option: $1 - this should not happen."
    usage
fi

if [ -z "$DISTRO" ]; then
    shopt -s globstar nullglob
    DOCKERFILES=( "$BASEDIR"/devops/docker/**/Dockerfile )
else
    if [ ! -f "$BASEDIR/devops/docker/$DISTRO/Dockerfile" ] ; then
      echo "Unabel to read $BASEDIR/devops/docker/$DISTRO/Dockerfile "
      exit 1
    fi
    DOCKERFILES=( "$BASEDIR/devops/docker/$DISTRO/Dockerfile" )
fi


for DOCKERFILE in "${DOCKERFILES[@]}"; do
  DISTRO_NAME=${DOCKERFILE%/Dockerfile}
  DISTRO_NAME=${DISTRO_NAME##${BASEDIR}/devops/docker/}
  IMGNAME=moduletest-$DISTRO_NAME:latest
  if [ -z "$NODOCKERBUIILD" ]; then
    docker build -t "$IMGNAME" -f "$DOCKERFILE"  $PWD
  fi
  if [ -z "$BUILDDIR" ]; then
  	BDIR="build-$DISTRO_NAME"
  else
  	BDIR="$BUILDDIR"
  fi
  docker run -v "$BASEDIR:/git/azure-osconfig" -w /git/azure-osconfig "$IMGNAME" ./devops/scripts/run-module-tests-int.sh "$BDIR" "$NOBUILD"
done
