#!/bin/bash
. /opt/esp/idf/export.sh > /dev/null 2>&1
cmake "$@"
