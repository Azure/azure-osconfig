#!/bin/bash

template=SecurityBaselineTests.json

(
  cat "${template}-header";
  ./mof-to-json.awk -v remediate=1 "${@}"
  cat "${template}-mid";
  ./mof-to-json.awk -v remediate=0 "${@}"
  cat "${template}-footer";
)