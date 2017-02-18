#! /bin/sh

# check parameters
if [ $# -ne 1 ];then
  echo "Error: inputs not valid"
  echo "usage: ./config.sh <supervisor board directory>"
  exit
fi

echo "######################################"
echo "####     CONFIGURATION SCRIPT     ####"
echo "######################################"
echo "##### board: $1"

# check if the program exists
if [ ! -f ./bin/configurator ];then
  echo "####         BUILD PROGRAM        ####"
  make
fi

# run configuration program
echo "####           CONFIGURE          ####"
./bin/configurator tmp_cfg.txt
if [ $? -ne 0 ];then
  echo "####             ERROR            ####"
  exit
fi

# wait until the board is available
echo "####          WAIT BOARD          ####"
busy=1
until [ $busy -eq 0 ];do
  ls $1 > /dev/null 2> /dev/null
  busy=$?
done

# wait 5 additional seconds
sleep 5

# copy file
echo "####         WRITE CONFIG         ####"
cp tmp_cfg.txt $1/cfg.txt
cp tmp_cfg.txt ./config/cfg.txt

# reset pending information
timestamp=`date +%s`
timestamp=$(($timestamp / 60))
echo $timestamp > ./config/offset.txt
rm $1/CNT.TXT
rm $1/SNS.TXT
rm $1/WTR.TXT

# remove the temporary file
rm tmp_cfg.txt

# reset board
echo "####     RESET BOARD MANUALLY     ####"

