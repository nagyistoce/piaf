#!/bin/bash
echo "Analyze piaf and it's children plugins woth valgrind"
valgrind --error-limit=no --trace-children=yes --log-file=/dev/shm/piaf ./piaf

