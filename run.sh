#!/bin/bash

rm -f analyze stdout.txt *~ core.*
if [ "x$1" = "xrelease" ]; then
    gcc -O2 analyze.cpp -o analyze

    chmod +x analyze

    echo "********************handle the GSE98638_HCC.TCell.S5063.TPM.txt**********************"
    ./analyze data/GSE98638_HCC.TCell.S5063.TPM.txt data/GSE98638_HCC.TCell.S5063.TPM.minimal.txt PADI4

    echo "*******************handle the GSE99254_NSCLC.TCell.S12346.TPM.txt********************"
    ./analyze data/GSE99254_NSCLC.TCell.S12346.TPM.txt data/GSE99254_NSCLC.TCell.S12346.TPM.minimal.txt PADI4

    echo "*******************handle the GSE108989_CRC.TCell.S11138.TPM.txt********************"
    ./analyze data/GSE108989_CRC.TCell.S11138.TPM.txt data/GSE108989_CRC.TCell.S11138.TPM.minimal.txt PADI4
else
    # gcc -g3 -D_DEBUG analyze.cpp -o analyze
    gcc -g3  analyze.cpp -o analyze

    chmod +x analyze

    ./analyze test/test.data test/stdout.txt PADI4

    result=`diff test/expect.txt test/stdout.txt`
    echo ""

    if [ "x$result" = "x" ]; then
	echo "Secceed!"
    else
	echo "Test case fail:"
	echo "$result"
    fi
fi



