#!/bin/sh
# Why? because I use it on a lot of developments on ../../ from here, that's why

for i in $(ls *.h) ; do
  echo '###' $(basename -s .h $i) '###'
  find ../../ -name $(basename -s .h $i).h | while read line ; do md5sum $line ; done | sort -u
  echo ------
  find ../../ \( -name $(basename -s .h $i).c -o -name $(basename -s .h $i).cpp \) | while read line ; do md5sum $line ; done | sort -u
  echo ------
  echo
done
