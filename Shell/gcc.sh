
TEST=SMALL_PROBLEM_SIZE
CPPFLAGS=
RUN_OPTIONS=
CC=gcc
STATIC=-static


	echo "Current in: $(pwd)"	
	LPATH=$(pwd)	
	DIR=${LPATH##*/}	

	
	if [[ $DIR == "automotive-bitcount" ]]; then		
		RUN_OPTIONS="1125000"
	elif [[ $DIR == "automotive-susan" ]]; then
		RUN_OPTIONS="input_large.pgm Output/output_large.smoothing.pgm -s"
	elif [[ $DIR == "consumer-jpeg" ]]; then
		RUN_OPTIONS="-dct int -ppm -outfile Output/output_large_decode.ppm input_large.jpg"
	elif [[ $DIR == "consumer-lame" ]]; then
		CPPFLAGS="-DHAVEMPGLIB -DLAMEPARSE -DNDEBUG -D__NO_MATH_INLINES -O -DLAMESNDFILE"
		RUN_OPTIONS="-S large.wav Output/output_large.mp3"
	elif [[ $DIR == "consumer-typeset" ]]; then
		CPPFLAGS="-DOS_UNIX=1 -DOS_DOS=0 -DOS_MAC=0 -DDB_FIX=0 -DUSE_STAT=1 -DSAFE_DFT=0 -DCOLLATE=1 -DLIB_DIR=\"lout.lib\" -DFONT_DIR=\"font\" -DMAPS_DIR=\"maps\" -DINCL_DIR=\"include\" -DDATA_DIR=\"data\" -DHYPH_DIR=\"hyph\" -DLOCALE_DIR=\"locale\" -DCHAR_IN=1 -DCHAR_OUT=0 -DLOCALE_ON=1 -DASSERT_ON=1 -DDEBUG_ON=0  -DPDF_COMPRESSION=0"
		RUN_OPTIONS="-x -I data/include -D data/data -F data/font -C data/maps -H data/hyph large.lout"
	elif [[ $DIR == "network-dijkstra" ]]; then
		RUN_OPTIONS="input.dat"	
	elif [[ $DIR == "network-patricia" ]]; then
		RUN_OPTIONS="large.udp"
	elif [[ $DIR == "office-ispell" ]]; then
		CPPFLAGS="-Dconst="
		RUN_OPTIONS="-a -d $(PROJ_SRC_DIR)/americanmed+ < large.txt"
	elif [[ $DIR == "security-blowfish" ]]; then
		RUN_OPTIONS="print_test_data"
	elif [[ $DIR == "security-rijndael" ]]; then
		RUN_OPTIONS="output_large.enc Output/output_large.dec d 1234567890abcdeffedcba09876543211234567890abcdeffedcba0987654321"
	elif [[ $DIR == "security-sha" ]]; then
		RUN_OPTIONS="input_large.asc"	
	elif [[ $DIR == "telecomm-adpcm" ]]; then
		RUN_OPTIONS="large.adpcm"
	elif [[ $DIR == "telecomm-CRC32" ]]; then
		RUN_OPTIONS="large.pcm"
	elif [[ $DIR == "telecomm-FFT" ]]; then
		RUN_OPTIONS="8 32768 -i"
	elif [[ $DIR == "telecomm-gsm" ]]; then
		CPPFLAGS="-DSTUPID_COMPILER -DNeedFunctionPrototypes=1 -DSASR"
		RUN_OPTIONS="-fps -c large.au"
fi
	
	if [[ $1 == "" ]]; then
		echo "==$CC -D$TEST -O3 $CPPFLAGS *.c $STATIC -lm -o main.ori"
		$CC -D$TEST -O3 $CPPFLAGS *.c $STATIC -lm -o main.ori
	else
		echo "==$CC -D$TEST -O3 -g $CPPFLAGS -v $1 $STATIC -lm -o main.ori"
		$CC -D$TEST -O3 -g $CPPFLAGS -v $1 $STATIC -lm -o main.ori
	fi


	if [ $? -ne 0 ]; then
		exit 
	fi

