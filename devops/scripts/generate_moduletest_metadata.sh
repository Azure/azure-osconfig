#!/bin/bash
# Generates test recipe configuration for all modules published with MIMs and Test Recipes
# MIM Path        : src/modules/mim
# Test Recipe Path: src/modules/test/recipes
if ! [ -x "$(command -v jq)" ]; then
  echo 'Error: jq is not installed.' >&2
  exit 1
fi
if [ $# -ne 2 ]; then
  echo 'Usage: ./generate_moduletest_metadata.sh testplace.json /azure-osconfig/build/modules' >&2
  exit 1
fi

# Absolute path to this script
SCRIPT=$(readlink -f $0)
SCRIPTPATH=`dirname $SCRIPT`
BaseDir=`realpath $SCRIPTPATH/../..`
mimBaseDir=$BaseDir/src/modules/mim
recipeBaseDir=$BaseDir/src/modules/test/recipes
testMetaDataDestPath=$1
modulesBaseDir=$2

# Sample modules are prefixed with their language
prefix=(cpp)
normalizeModuleName()
{
  local m=$1
  for i in $prefix; do
    m=$(echo $m | sed -e "s/^$i//")
  done
  echo $m
}

modules=$(find $modulesBaseDir -name '*.so')
json="{ \"Modules\" : [], \"Recipes\" : [] }"
while IFS= read -r line ;
  do 
  modulename=$(basename $line | cut -f1 -d'.');
  # normalize module names (module samples are prefixed with the module language)
  modulename=$(normalizeModuleName $modulename)
  mimpath=$(find $mimBaseDir -name "*$modulename*.json")
  recipepath=$(find $recipeBaseDir -iname "*$modulename*.json")

  if [ ! -z "$recipepath" ] && [ ! -z "$mimpath" ]; then
    # Create entry if both mim+recipe are found
    echo "Found '$modulename' module adding to recipe configuration"
    modulePath="[\"$line\"]"
    recipes="[{ \"ModuleName\": \"$modulename\", \"ModulePath\": \"$line\", \"MimPath\": \"$mimpath\", \"TestRecipesPath\": \"$recipepath\" }]"
    json=$(echo $json | jq --argjson modulePath "$modulePath" '.Modules |= . + $modulePath')
    json=$(echo $json | jq --argjson recipes "$recipes" '.Recipes |= . + $recipes')
  fi
done <<< $modules

echo $json | jq '.' > $testMetaDataDestPath
echo "Test recipe configuration written to $testMetaDataDestPath"