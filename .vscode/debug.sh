#!/bin/bash

set -euxo pipefail

sudo /usr/bin/gdb "$@"
