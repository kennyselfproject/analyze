#!/bin/bash

rm -f analyze stdout.txt *~
gcc -O2 analyze.cpp -o analyze
chmod +x analyze
echo "********************handle the GSE98638_HCC.TCell.S5063.TPM.txt**********************"
./analyze GSE98638_HCC.TCell.S5063.TPM.txt GSE98638_HCC.TCell.S5063.TPM.minimal.txt
echo "*******************handle the GSE99254_NSCLC.TCell.S12346.TPM.txt********************"
./analyze GSE99254_NSCLC.TCell.S12346.TPM.txt GSE99254_NSCLC.TCell.S12346.TPM.minimal.txt
echo "*******************handle the GSE108989_CRC.TCell.S11138.TPM.txt********************"
./analyze GSE108989_CRC.TCell.S11138.TPM.txt GSE108989_CRC.TCell.S11138.TPM.minimal.txt



