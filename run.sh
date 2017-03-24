#!/bin/bash
cp ./plugins/.libs/example.so ~/.oul/plugins/ 
cp ./plugins/.libs/qmon.so ~/.oul/plugins/ 
LD_LIBRARY_PATH=liboul/.libs beasy/.libs/beasy -d
