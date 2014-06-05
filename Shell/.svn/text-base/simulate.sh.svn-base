RUN=/home/qali/my-small-projects/Shell/run.sh
PIN_DIR=/home/qali/Ins/pin-2.13-61206-gcc.4.4.7-linux
PIN=$PIN_DIR/pin
#PIN_TOOL=$PIN_DIR/source/tools/SimpleExamples/obj-ia32/pwlWriteDist.so
PIN_TOOL=$PIN_DIR/source/tools/SimpleExamples/obj-intel64/pwlStackHeap.so
#PIN_TOOL=$PIN_DIR/source/tools/SimpleExamples/obj-intel64/pwlBaseline.so
#PIN_TOOL=$PIN_DIR/source/tools/ManualExamples/obj-intel64/malloctrace.so

PIN_OPTION="-d $1"	
	
	
#echo "==$RUN $PIN $PIN_TOOL $PIN_OPTION"
$RUN $PIN $PIN_TOOL "$PIN_OPTION"	


if [ $? -ne 0 ]; then
	exit 
fi


