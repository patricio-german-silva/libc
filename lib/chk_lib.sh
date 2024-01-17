#!/bin/sh
# Why? because I use it on a lot of developments on ../../ from here, that's why
if [ $# -ne 0 ] ; then
  PATRN="${1}.h"
else
  PATRN="*.h"
fi

for i in $(ls ${PATRN}) ; do
  echo '###' $(basename -s .h $i) '###'
  find ../../ -name $(basename -s .h $i).h | sort -u | while read line ; do md5sum $line ; done
  echo ------
  find ../../ \( -name $(basename -s .h $i).c -o -name $(basename -s .h $i).cpp \) | sort -u | while read line ; do md5sum $line ; done
  echo ------
  echo
done
