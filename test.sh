#!/bin/bash
 
./store &

#comandi eseguiti in una subshell in modo che
#la wait non consideri il processo store
(
for (( i=0; i<50; i++ ))
do
    ./client $i 1 >>testout.log &
done

wait

for (( i=0; i<30; i++ ))
do
    ./client $i 2 >>testout.log &
done

for (( i=30; i<50; i++ ))
do
    ./client $i 3 >>testout.log &
done
 
wait
)

kill -2 `pidof store`
