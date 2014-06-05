cd $1
NAME=$1
sed -i "s/sim-profile /$\@ \.\//" runme_small.sh 
sed -i "s/sim-profile /$\@ \.\//" runme_large.sh
#mv a.sh runme_large.sh
cd ..
less $1/runme_small.sh

#make


#for FILE in results.large; do
#	#NEWFILE=$(echo ${FILE}|sed -e 's/\.arm//')	
#	NEWFILE=results.small
#	echo "mv $FILE $NEWFILE"
#	mv $FILE $NEWFILE
	
#done


