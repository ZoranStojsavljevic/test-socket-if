#!/bin/bash
gcc if_c.c -o if_c
./if_c
for i in {1..40}; do VBoxManage hostonlyif remove vboxnet$i; done
