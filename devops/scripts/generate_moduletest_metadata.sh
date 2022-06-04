#!/bin/bash
# Generates test metadata for all modules published with MIMs and Test Recipes
# MIM Path        : src/modules/mim
# Test Recipe Path: src/modules/test/recipes
# Requires: jq
if ! [ -x "$(command -v jq)" ]; then
  echo 'Error: jq is not installed.' >&2
  exit 1
fi

# Absolute path to this script
SCRIPT=$(readlink -f $0)
SCRIPTPATH=`dirname $SCRIPT`

mimBaseDir=$SCRIPTPATH/../../src/modules/mim
modulesBaseDir=$SCRIPTPATH/../../build/modules
recipeBaseDir=$SCRIPTPATH/../../src/modules/test/recipes
testMetaDataDestPath=$SCRIPTPATH/../../build/modules/test/testplate.json

modules=$(find $modulesBaseDir -name '*.so')
testJSON="[]"
while IFS= read -r line ;
  do 
  modulename=$(basename $line | cut -f1 -d'.');
  mimpath=$(find $mimBaseDir -name "*$modulename*.json")
  recipepath=$(find $recipeBaseDir -iname "*$modulename*.json")

  if [ ! -z "$recipepath" ] && [ ! -z "$mimpath" ]; then
    # Create entry if both mim+recipe are found
    json="[{ \"ModulePath\": \"$line\", \"MimPath\": \"$mimpath\", \"TestRecipesPath\": \"$recipepath\" }]"
    testJSON=$(echo $testJSON | jq --argjson testMetadata "$json" '. |= . + $testMetadata')
  fi
done <<< $modules

echo $testJSON | jq '.' > $testMetaDataDestPath
echo "Test metadata written to $testMetaDataDestPath"