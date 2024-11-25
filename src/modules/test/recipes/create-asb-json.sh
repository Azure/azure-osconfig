#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

template=SecurityBaselineTests.json

(
  cat "${template}-header";
  ./mof-to-json.awk -v remediate=1 "${@}"
  cat "${template}-mid";
  ./mof-to-json.awk -v remediate=0 "${@}"
  cat "${template}-footer";
)
