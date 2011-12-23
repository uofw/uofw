#!/bin/sh

###
# This program generates a diff file from different PRX versions, comparing functions with each other.
# It's very slow, especially for big modules (it can take up to an hour for huge modules, probably)
# It creates a lot of files in the <PRX file>-diff directory. The most useful created file is "mod.diff", it contains all the differences, including the ones from data.
####

# Generates a list of functions
function genlist
{
    local file=$1

    local i=1
    for f in `cat $file|grep "; Subroutine"|sed -e "s/.*outine //" -e "s/ .*//"`; do
        echo $i $f
        i=$(( $i + 1 ))
    done
}

# Add two matching functions (from old & new PRX) in the matching list
function match
{
    list1=`echo "$list1"|grep -v "^$1 "`
    list2=`echo "$list2"|grep -v "^$2 "`
    matchlist[$1]=$2
    echo "$1 <=> $2"
}

# Get a function content
function getfunc
{
    file=$1
    func=$2
    start=`cat -n $file|grep "Subroutine.*$func"|sed -e "s/^[^0-9]*//" -e "s/[^0-9].*//"`
    start=`expr $start + 3`
    content=`cat $file|sed -n -e "$start,/; ====/p"`
    echo "$content"|head -n $(( `echo "$content"|wc -l` - 2 ))|sed -e "s/; Refs.*//" -e "s/loc_......../loc/" -e "s/Subroutine sub_......../Subroutine sub/" -e "s/0x........: //" -e "s/Address.*//" -e "s/Data ref.*/Data ref/"
}

# Get data sections from a PRX file
function getdata
{
    startaddr=`cat $1|grep ".rodata"|sed -e "s/.*Address //" -e "s/ .*//"`
    startaddr=`printf "%d\n" $startaddr`
    for i in `cat $1|sed -n -e "5,$ p"|sed -n -e "/Section/,$ p"|grep "^0x........ "|sed -e "s/ - ................$//" -e "s/ - /_/" -e "s/ | /|/g" -e "s/ /,/g"`; do
        addr=`echo $i|sed -e "s/_.*//"`
        intv=`printf "%d\n" $addr`
        offset=`expr $intv - $startaddr`
        offset=`printf "0x%08X\n" $offset`
        echo $i|sed -e "s/^0x........_/$offset /" -e "s/,/ /g" -e "s/|/ | /g"
    done
}

if [ $# -ne 2 ]; then
    echo "Usage: ./diff.sh <old> <new>"
    exit 1
fi

echo "diff $1 $2"
prxtmp1="re1"
prxtmp2="re2"
difftmp1="diff1"
difftmp2="diff2"
difftmp="diff"

dir=`echo $1|sed -e "s/.*\///"`
dir="$dir-diff"

if [ ! -e $dir ]; then
    mkdir $dir
fi

cd $dir

if [ ! -f $prxtmp1 ]; then
    prxtool -w ../$1 > $prxtmp1
fi
if [ ! -f $prxtmp2 ]; then
    prxtool -w ../$2 > $prxtmp2
fi

list1=`genlist $prxtmp1`
numlist1=`echo "$list1"|wc -l`
list2=`genlist $prxtmp2`
numlist2=`echo "$list2"|wc -l`

list1back=$list1
list2back=$list2

echo "list 1:"
echo "$list1"
echo "list 2"
echo "$list2"

# regroup functions by name
for num1 in `seq $numlist1`; do
    name=`echo "$list1"|grep "^$num1 "|sed -e "s/[^ ]* //"`
    num2=`echo "$list2"|grep "$name"|sed -e "s/ .*//"`
    if [ -n "$num2" ]; then
        match $num1 $num2
    fi
done

echo "Storing functions"

# put each function in a file
a1=`echo func-1-*`
a2=`echo func-2-*`
if [ "$a1" = "func-1-*" ] || [ "$a2" = "func-2-*" ]; then
    for name in `echo "$list1back"|sed -e "s/.* //"`; do
        num=`echo "$list1back"|grep $name|sed -e "s/ .*//"`
        getfunc $prxtmp1 $name > "func-1-$num"
    done

    for name in `echo "$list2back"|sed -e "s/.* //"`; do
        num=`echo "$list2back"|grep $name|sed -e "s/ .*//"`
        getfunc $prxtmp2 $name > "func-2-$num"
    done
fi

echo "Doing diff"

# put each diff in a file
a=`echo diff-*`
if [ "$a" = "diff-*" ]; then
    for l1 in `echo "$list1"|sed -e "s/ .*//"`; do
        for l2 in `echo "$list2"|sed -e "s/ .*//"`; do
            diff "func-1-$l1" "func-2-$l2" > diff-$l1-$l2
        done
    done
fi

echo "Regrouping functions by number of differences"

# regroup functions by number of differences
for l1 in `echo "$list1"|sed -e "s/ .*//"`; do
    min=9999999
    found=0
    for l2 in `echo "$list2"|sed -e "s/ .*//"`; do
        numdiff=`wc -l diff-$l1-$l2|sed -e "s/ .*//"`
        numl1=`wc -l func-1-$l1|sed -e "s/ .*//"`
        numl2=`wc -l func-2-$l2|sed -e "s/ .*//"`
        if [ $numdiff -lt `expr \( $numl1 + $numl2 \) \* 9 / 10` ] && [ $numdiff -lt $min ]; then
            min=$numdiff
            min2=$l2
            found=1
        fi
    done
    if [ $found -eq 1 ]; then
        match $l1 $min2
    fi
done

# generate mod.diff
rm -f mod.diff
touch mod.diff

for num1 in `seq $numlist1`; do
    num2=${matchlist[$num1]}
    if [ ! "$num2" = "" ]; then
        name1=`echo "$list1back"|grep "^$num1 "|sed -e "s/.* //"`
        name2=`echo "$list2back"|grep "^$num2 "|sed -e "s/.* //"`
        if [ ! -f "diff-$num1-$num2" ]; then
            diff "func-1-$num1" "func-2-$num2" > diff-$num1-$num2
        fi
        echo "$name1 <=> $name2"
        if [ "$name1" = "$name2" ]; then
            if [ `cat diff-$num1-$num2|wc -l` -ne 0 ]; then
                echo "; ================================" >> mod.diff
                echo "; $name1" >> mod.diff
            fi
        else
            echo "; ================================" >> mod.diff
            echo "; $name1 renamed to $name2" >> mod.diff
        fi

        cat diff-$num1-$num2 >> mod.diff
    fi
done

for removed in `echo "$list1"|sed -e "s/ .*//"`; do
    echo "; ================================" >> mod.diff
    name1=`echo "$list1back"|grep "^$removed "|sed -e "s/.* //"`
    echo "; $name1 has been removed" >> mod.diff
done

for added in `echo "$list2"|sed -e "s/ .*//"`; do
    echo "; ================================" >> mod.diff
    name2=`echo "$list2back"|grep "^$added "|sed -e "s/.* //"`
    echo "; $name2 has been added" >> mod.diff
    cat func-2-$added >> mod.diff
done

# diff data
echo "; ================================" >> mod.diff
echo "; data" >> mod.diff

getdata $prxtmp1 > data1
getdata $prxtmp2 > data2

diff data1 data2 >> mod.diff

cd ..

