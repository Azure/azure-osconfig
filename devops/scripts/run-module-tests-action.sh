#!/bin/bash

if [ $# -ne 2 ]; then
	echo "Usage: $0 <sources_path> <build_path>"
	echo "  sources_path: path to the sources directory"
	echo "  build_path:   path to the build directory"
	exit 1
fi

sources_path=$1
if [ ! -d $sources_path ]; then
	echo "Invalid sources path $sources_path: must be a directory"
	exit 1
fi

recipes_path="$sources_path/src/modules/test/recipes/"
if [ ! -d $recipes_path ]; then
	echo "Inavlid recipes path $recipes_path: must be a directory"
	exit 1
fi

build_path=$2
if [ ! -d $build_path ]; then
	echo "Invalid build path $build_path: must be a directory"
	exit 1
fi

executable_path="$build_path/modules/test/moduletest"
if [ ! -e $executable_path ]; then
	echo "Executable $executable_path not found"
	exit 1
fi

chmod +x $executable_path

result=0
recipes=$(find $recipes_path -name "*.json")
for recipe in $recipes; do
	recipe_name=$(basename $recipe | tr '[:upper:]' '[:lower:]' | sed 's/\.[^.]*$//' | sed 's/\(test\|tests\)$//')
	modules=$(cat $recipe | grep '\.so' | awk -F'"' '{print $4}' | head -n 1)
	if [ -z "$modules" ]; then
		echo "[ SKIP ] no modules found in recipe $recipe_name" | tee -a $build_path/module-test.log
		continue
	fi

	module_name=${modules%%.so}
	module_path="$build_path/modules/$module_name/src/so/"
	skip=
	for module in $modules; do
		if [ ! -f "$module_path/$module_name.so" ]; then
            echo "[ SKIP ] module $module lodaed in recipe $recipe_name not found" | tee -a $build_path/module-test.log
			skip=1
			break
		fi
  	done

	if [ ! -z "$skip" ]; then
		continue
	fi

	echo "[ RUN ] $recipe_name" >> $build_path/module-test.log
	output=$($executable_path $recipe --bin $module_path)
	rc=$?
	echo "$output" >> $build_path/module-test.log
	if [ $rc -ne 0 ]; then
		echo "[ FAIL ] $recipe_name" | tee -a $build_path/module-test.log
		result=1
	else
		echo "[ PASS ] $recipe_name" | tee -a $build_path/module-test.log
	fi
done

if [ $result -eq 0 ]; then
	echo "[ PASS ] All tests passed"
else
	echo "[ FAIL ] See $build_path/module-test.log for details"
fi

exit $result
