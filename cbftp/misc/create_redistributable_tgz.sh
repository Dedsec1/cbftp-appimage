#!/bin/bash
SCRIPTPATH=`pwd`"/"`dirname $0`
cd $SCRIPTPATH/..
DIFF=`svn diff|wc -l`
if [ $DIFF -gt 0 ]; then
  echo "Error: this is not a clean svn copy."
  exit 1
fi
VERSION=`svn info|grep Revision|awk '{ print $2 }'`
echo $VERSION > .version
VERSION="r"$VERSION
FILES=`find . -name "*.cpp" -o -name "*.h" -o -name Makefile -o -name Makefile.inc -o -name .version -o -name start_with_gdb.sh -o -name README|xargs`
tar czf cbftp-$VERSION.tar.gz $FILES --transform "s,^\.,cbftp-$VERSION,"
rm .version