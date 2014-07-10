#!/bin/bash

aclocal && libtoolize && autoheader && automake --add-missing && autoconf
