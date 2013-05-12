#!/bin/bash

fstunion $1 $2 \
    | fstrmepsilon \
    | fstdeterminize --delta=1e-8 \
