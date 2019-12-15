#!/bin/bash

FILE="TestResults-$(git rev-parse --short HEAD).txt"
FILE_RUNTIME="TestResults-$(git rev-parse --short HEAD)-runtime.txt"
FILE_OVERHEAD="TestResults-$(git rev-parse --short HEAD)-overhead.txt"

printf "Running the makefile!\n\n"

make -f Makefile

printf "Finished make, continuing with tests:\n"
printf "Results will be saved in $FILE\n\n";

# Run each of the 18 profiles with 10 different sizes (180 tests)

sizes=(5 100 500 1000 5000 10000 50000 100000 500000 1000000 )
sizeProfiles=(uniform uniform normal1 normal1 fixed8 fixed8 fixed16 fixed16 fixed24 fixed24 fixed104 fixed104 fixed200
    fixed200 increase increase decrease decrease)
allocProfiles=(oneinthree cluster oneinthree cluster oneinthree cluster oneinthree cluster oneinthree cluster oneinthree
    cluster oneinthree cluster oneinthree cluster oneinthree cluster)

SUMME=0
profileIndex=0

echo "" > ${FILE}
echo "" > ${FILE_RUNTIME}
echo "" > ${FILE_OVERHEAD}


while [[ ${profileIndex} -lt  ${#sizeProfiles[@]} ]]
do
    echo "\"${sizeProfiles[profileIndex]} ${allocProfiles[profileIndex]}\"" >> ${FILE}
    echo "\"runtime ${sizeProfiles[profileIndex]} ${allocProfiles[profileIndex]}\"" >> ${FILE_RUNTIME}
    echo "\"overhead ${sizeProfiles[profileIndex]} ${allocProfiles[profileIndex]}\"" >> ${FILE_OVERHEAD}
    sizeIndex=0
    while [[ ${sizeIndex} -lt  ${#sizes[@]} ]]
    do
        echo "Testing Profile ${sizeProfiles[$profileIndex]} ${allocProfiles[$profileIndex]}, Size ${sizes[sizeIndex]}"
        RES=$(./testit 1 ${sizes[sizeIndex]} ${sizeProfiles[$profileIndex]} ${allocProfiles[$profileIndex]} | grep '[^\.]')
        VALUE=$(echo "$RES" | grep 'Points' | grep -o -E -e '[+\-\.0-9]*')
        RUNTIME=$(echo "$RES" | grep 'Runtime' | grep -o -E -e '[+\-\.0-9]*')
        OVERHEAD=$(echo "$RES" | grep 'overhead' | grep -o -E -e '[+\-\.0-9]*')
        SUMME=`echo ${SUMME} + ${VALUE} | bc`
        echo "${sizes[sizeIndex]},${VALUE}" >> ${FILE}
        echo "${sizes[sizeIndex]},${RUNTIME}" >> ${FILE_RUNTIME}
        echo "${sizes[sizeIndex]},${OVERHEAD}" >> ${FILE_OVERHEAD}
        echo "Profile $profileIndex, Size $sizeIndex done."
        ((sizeIndex++))
    done
    printf "\n\n" >> ${FILE}
    printf "\n\n" >> ${FILE_RUNTIME}
    printf "\n\n" >> ${FILE_OVERHEAD}
    ((profileIndex++))
done

printf "\nTotal sum: %s\n" "$SUMME"

gnuplot -c plot.gp ${FILE} > "TestResults-$(git rev-parse --short HEAD).png"
gnuplot -c plot.gp ${FILE_RUNTIME} > "TestResults-$(git rev-parse --short HEAD)-runtime.png"
gnuplot -c plot.gp ${FILE_OVERHEAD} YLOG > "TestResults-$(git rev-parse --short HEAD)-overhead.png"

exit 0;