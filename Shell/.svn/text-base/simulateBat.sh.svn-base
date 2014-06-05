TOOL1=/home/qali/my-small-projects/Shell/simulate.sh
TOOL2=/home/qali/my-small-projects/pwl/MemoryAllocator
TOOL3=/home/qali/my-small-projects/pwl/WearCompute
#TOOL=/home/qali/my-small-projects/Shell/tool.sh
	

for DIR in *; do
	if [ "$DIR" == "." -o "$DIR" == ".." ]; then
		echo $DIR
		continue
	fi


	echo "<< Deal with: $DIR"
	echo "<< Deal with: $DIR" >>/home/qali/log
	cd $DIR	

		
	##1. pin simulator
	#$TOOL1 $1
	#chmod a+x *.sh

	##2. memoryallocator 13 stack.out_4 5 0/1/2
	echo "$TOOL2 $1 $2 $3 $4"
	echo "$TOOL2 $1 $2 $3 $4" >>/home/qali/log
	$TOOL2 $1 $2 $3 $4 2>>/home/qali/log
		
	##3. WearCompute
	#echo "$TOOL3 $DIR 0"
	#$TOOL3 $DIR 0

	#echo "$TOOL3 $DIR 1"
	#$TOOL3 $DIR 1
	#echo "$TOOL3 $DIR 2"
	#$TOOL3 $DIR 2

	#echo "cat  stack.out_5--0  stack.out_5--1  stack.out_5--2 >> ~/wear.txt"
	#cat  stack.out_5--0  stack.out_5--1  stack.out_5--2 >> ~/wear.txt stats

	##4. memory allocator
	#$TOOL 1024 5 results.txt
	
	if [ $? != 0 ]; then
		echo "Error in $DIR"
		echo "Error in $DIR" >>/home/qali/log
			exit
	fi
	cd ..
done

if [ $? -ne 0 ]; then
	exit 
fi

