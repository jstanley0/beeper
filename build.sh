#!/bin/bash
g++ -g beeper.cpp -pthread -lrt -lpigpiod_if2 -o beeper
