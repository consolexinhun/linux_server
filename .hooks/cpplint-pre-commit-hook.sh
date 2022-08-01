#!/bin/sh

if git rev-parse --verify HEAD >/dev/null 2>&1
then
    against=HEAD
else
    # Initial commit: diff against an empty tree object
    against=abcd5dc642cb6eb9a060e54bf8d69288fbee4798
fi

# Redirect output to stderr.
exec 1>&2

cpplint=cpplint
sum=0

# for cpp
for file in $(git diff-index --name-status $against -- | grep -E '\.[ch](pp)?$' | awk '{print $2}'); do
    $cpplint $file
    sum=$(expr ${sum} + $?)
done

if [ ${sum} -eq 0 ]; then
    exit 0
else
    exit 1
fi
