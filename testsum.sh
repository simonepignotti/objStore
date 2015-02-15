#!/bin/bash

#conto del numero di errori
err=`grep 'KO\|ERR' testout.log | wc -l`

#selezione delle righe con i risultati ottenuti dai client di test
#e dei loro campi numerici (tipo di test e operazioni effettuate e riuscite)
grep "successful operations on" testout.log | cut -d' ' -f2,4,8 >formtestout.log

declare -a temp

#numero di test effettuati per ogni tipo
test1=0
test2=0
test3=0
#operazioni effettuate per tipo di test
perf1=0
perf2=0
perf3=0

#temp[0] -> tipo di test
#temp[1] -> operazioni effettuate con successo
#temp[2] -> operazioni effettuate
while read -a temp
do
	if [ ${temp[0]} == 1 ]; then
		succ1+=${temp[1]}
		perf1+=${temp[2]}
		test1=$[test1 + 1]
	else if [ ${temp[0]} == 2 ]; then
			succ2+=${temp[1]}
			perf2+=${temp[2]}
			test2=$[test2 + 1]
	else if [ ${temp[0]} == 3 ]; then
				succ3+=${temp[1]}
				perf3+=${temp[2]}
				test3=$[test3 + 1]
			fi
		fi
	fi
done <formtestout.log

if [ $test1 -gt 0 ]; then
	p1=`bc <<< "scale = 4; (($succ1 / $perf1) * 100)"`
	echo "$test1 type 1 tests executed with $p1% percentage of success"
fi
if [ $test2 -gt 0 ]; then
	p2=`bc <<< "scale = 4; (($succ2 / $perf2) * 100)"`
	echo "$test2 type 2 tests executed with $p2% percentage of success"
fi
if [ $test3 -gt 0 ]; then
	p3=`bc <<< "scale = 4; (($succ3 / $perf3) * 100)"`
	echo "$test3 type 3 tests executed with $p3% percentage of success"
fi

testtot=$[ test1 + test2 + test3 ]
if [ $testtot -gt 0 ]; then
	ptot=`bc <<< "scale = 4; ((($succ1 + $succ2 + $succ3) / ($perf1 + $perf2 + $perf3)) * 100)"`
	echo "$testtot tests totally executed with $ptot% percentage of success"
fi

if [ $err -gt 0 ]; then
	echo "$err errors detected (during communications or system calls)"
fi
