#! /bin/sh

# check parameters
if [ $# -ne 1 ];then
    echo "Error: inputs not valid"
    echo "usage: ./update.sh <supervisor board directory>"
    exit
fi

echo "######################################"
echo "####         READER SCRIPT        ####"
echo "######################################"
echo "##### board: $1"

# infinitive loop
until [ 1 -eq 0 ];do
    # update time offset
    timestamp=`date +%s`
    timestamp=$(($timestamp / 60))
    echo $timestamp > ./config/offset.txt

    # sleep until next update
    echo "####          WAIT 1 DAY          ####"
    # 60 seconds x 60 minutes x 24 hours = 86400 seconds
    sleep 86400

    # wait until the board has provided data
    echo "####          WAIT BOARD          ####"
    # wait until the board is idle
    busy=1
    until [ $busy -eq 0 ];do
        ls $1 > /dev/null 2> /dev/null
        busy=$?
    done

    # wait 5 additional seconds
    sleep 5

    # copy and remove files from the board
    echo "####          COPY FILES          ####"
    if [ -f $1/SNS.TXT ]; then
        mv $1/SNS.TXT ./data/sns.txt
    else
        touch ./data/sns.txt
    fi
    if [ -f $1/WTR.TXT ]; then
        mv $1/WTR.TXT ./data/wtr.txt
    else
        touch ./data/wtr.txt
    fi
    rm $1/CNT.TXT

    # process data: save into database and send to the cloud
    echo "####     PROCESS AND SAVE DATA    ####"
    ./bin/reader main.db ./data/sns.txt ./data/wtr.txt \
                         ./config/cfg.txt ./config/offset.txt \
                         ./config/cloud.txt > /dev/null 2> /dev/null

    # remove temporary files
    rm -f ./data/sns.txt ./data/wtr.txt

    # write and send email
    ./bin/shower main.db ./config/cfg.txt ./config/offset.txt \
                         ./data/script.gpl ./data/data.res \
                         ./data/email.txt > /dev/null 2> /dev/null
    if [ $? -eq 0 ];then
        gnuplot ./data/script.gpl
        epstopdf humi.eps
        epstopdf temp.eps
        epstopdf mois.eps
        pdfjoin humi.pdf temp.pdf mois.pdf \
                --outfile report.pdf > /dev/null 2> /dev/null
        rm -f humi.eps temp.eps mois.eps humi.pdf temp.pdf mois.pdf
        cat ./config/emails.txt | while read line
        do
            mutt -s "Daily report" $line -a report.pdf < ./data/email.txt
        done
        rm -f ./data/email.txt
        rm -f ./data/script.gpl
        rm -f ./data/data.res
        rm -f report.pdf
    fi
done

