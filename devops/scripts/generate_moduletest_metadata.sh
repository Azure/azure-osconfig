#!/bin/bash
# Generates test recipe configuration for all modules published with MIMs and Test Recipes
# MIM Path        : src/modules/mim
# Test Recipe Path: src/modules/test/recipes
if ! [ -x "$(command -v jq)" ]; then
  echo 'Error: jq is not installed.' >&2
  exit 1
fi

# Absolute path to this script
SCRIPT=$(readlink -f $0)
SCRIPTPATH=`dirname $SCRIPT`
BaseDir=`realpath $SCRIPTPATH/../..`
mimBaseDir=$BaseDir/src/modules/mim
modulesBaseDir=$BaseDir/build/modules
recipeBaseDir=$BaseDir/src/modules/test/recipes
testMetaDataDestPath=$BaseDir/build/modules/test/testplate.json

modules=$(find $modulesBaseDir -name '*.so')
testJSON="[]"
while IFS= read -r line ;
  do 
  modulename=$(basename $line | cut -f1 -d'.');
  mimpath=$(find $mimBaseDir -name "*$modulename*.json")
  recipepath=$(find $recipeBaseDir -iname "*$modulename*.json")

  if [ ! -z "$recipepath" ] && [ ! -z "$mimpath" ]; then
    # Create entry if both mim+recipe are found
    echo "Found '$modulename' module adding to recipe configuration"
    json="[{ \"ModuleName\": \"$modulename\", \"ModulePath\": \"$line\", \"MimPath\": \"$mimpath\", \"TestRecipesPath\": \"$recipepath\" }]"
    testJSON=$(echo $testJSON | jq --argjson testMetadata "$json" '. |= . + $testMetadata')
  fi
done <<< $modules

echo $testJSON | jq '.' > $testMetaDataDestPath
echo "Test recipe configuration written to $testMetaDataDestPath"