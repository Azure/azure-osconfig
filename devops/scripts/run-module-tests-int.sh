#!/bin/sh
BUILDDIR=$1
SRCDIR=`pwd`
mkdir -p /etc/osconfig
cp -r ./src/adapters/pnp/daemon/osconfig.json /etc/osconfig/osconfig.json

if [ -z "$2" ]; then
	mkdir $BUILDDIR
	cd $BUILDDIR
	cmake $SRCDIR/src -DCMAKE_C_COMPILER="/usr/bin/gcc" -DCMAKE_CXX_COMPILER="/usr/bin/g++" -DCMAKE_build-type=Release -Duse_prov_client=ON -Dhsm_type_symm_key=ON -DCOMPILE_WITH_STRICTNESS=ON -DBUILD_TESTS=ON -DBUILD_SAMPLES=ON -DBUILD_ADAPTERS=ON -Duse_default_uuid=O
	cmake --build . --config Debug  --target install
fi

cd $BUILDDIR

result=0
recipes=$(ls -d $SRCDIR/src/modules/test/recipes/compliance/*.json)

for recipe in $recipes; do
  name=$(basename $recipe | tr '[:upper:]' '[:lower:]' | sed 's/\.[^.]*$//' | sed 's/\(test\|tests\)$//')

  echo -n "testing $name ... "
  # Weirdness to make sure that $PPID != 1, which breaks logging
  echo "./modules/test/moduletest $recipe --bin modules/bin --verbose" > $name.sh
  chmod a+x $name.sh

  if output=$(./$name.sh); then
    echo passed
  else
    echo failed
    result=1
    echo "::warning file=$name.log::Error(s) in module-test for '$name'"
  fi

  echo "$output"
  echo "$output" > ../../$name.log
done

exit $result
