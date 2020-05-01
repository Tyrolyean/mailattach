#!/bin/bash


if [ -z "$1" ]
  then
	  echo "Cleaning in working directory!"
else
	echo "Cleaning in $1!"
	cd $1
fi

for d in */ ; do
	DIR_DATE=$(echo $d| head -c 10)
	expration_DATE=$(date -d "-10 days" --iso-8601)
	echo $expration_DATE vs $DIR_DATE
	if [[ "$expration_DATE" > "$DIR_DATE" ]]
	then
		echo "Purging old directory $d..."
		rm -rf $d
	fi
done

