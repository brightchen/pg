#!/bin/bash

# Script to build and run HelloWorld applications in multiple languages

echo "Building and running HelloWorld applications..."

# C
echo
echo "=== C ==="
gcc HelloWorld.c -o HelloWorld_c
./HelloWorld_c

# Java
echo
echo "=== Java ==="
javac HelloWorld.java
java HelloWorld

# Python
echo
echo "=== Python ==="
python3 HelloWorld.py

# Go
echo
echo "=== Go ==="
go build -o HelloWorld_go HelloWorld.go
./HelloWorld_go

# Rust
echo
echo "=== Rust ==="
rustc -o HelloWorld_rs HelloWorld.rs
./HelloWorld_rs

echo
echo "All applications executed successfully!"