TOOL=/home/qali/my-small-projects/Shell/simulateBat.sh

for DIR in *; do
	if [ "$DIR" == "." -o "$DIR" == ".." -o "DIR" == "24" -o ! -d $DIR ]; then
		echo $DIR
		continue
	fi


	echo "<< Deal with Benchmarks in: $DIR"
	cd $DIR

		
		echo "$TOOL $DIR stack.out_5 5 0"
		$TOOL $DIR stack.out_5 5 0
		#echo "$TOOL $DIR stack.out_5 5 1"
		#$TOOL $DIR stack.out_5 5 1

		if [ $? != 0 ]; then
			echo "Error in $DIR"
				exit
		fi
	cd ..
done

if [ $? -ne 0 ]; then
	exit 
fi

