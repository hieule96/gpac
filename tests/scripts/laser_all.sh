is_linux32=0
#on linux 32 bit we for now disable the hashes, they all differ due to different float/double precision
config=`gpac -h bin 2>&1 | grep GPAC_HAS_64`

if [ -z $config ] ; then
config=`gpac -h bin 2>&1 | grep GPAC_CONFIG_LINUX`

if [ -n "$config" ] ; then
is_linux32=1
fi
fi

#@lsr_test execute tests on lsr file: laser<->MP4, laser<->saf,  conversions BT, XMT and MP4 Playback
lsr_test ()
{
 lsrfile=$1
 mp4file=${lsrfile%.*}'.mp4'
 saffile=${lsrfile%.*}'.saf'
 name=$(basename $1)
 name=${name%.*}
 force_coord_bits=0

 case $1 in
 *.png )
 return ;;

 *.jpg )
 return ;;

 *enst_canvas* )
 force_coord_bits=1 ;;
 esac

 #start our test, specifying all hash names we will check
 test_begin "laser-$name"
 if [ $test_skip  = 1 ] ; then
  return
 fi

 opts=""
 if [ $force_coord_bits = 1 ] ; then
  opts="-coord-bits 24"
 fi

 #LSR->MP4
 do_test "$MP4BOX $opts -mp4 $lsrfile" "LSR2MP4" && do_hash_test "$mp4file" "lsr-to-mp4"

 #LSR->SAF
 do_test "$MP4BOX $opts -saf $lsrfile" "LSR2SAF" && do_hash_test "$saffile" "lsr-to-saf"

 #MP4->LSR
 do_test "$MP4BOX -lsr $mp4file -out $TEMP_DIR/test1.lsr" "MP42LSR"
 do_hash_test "$TEMP_DIR/test1.lsr" "mp4-to-lsr"

 #SAF->LSR
 do_test "$MP4BOX -lsr $saffile -out $TEMP_DIR/test2.lsr" "SAF2LSR"
 do_hash_test "$TEMP_DIR/test2.lsr" "saf-to-lsr"

 #mp4 read and render test
 RGB_DUMP="$TEMP_DIR/$2-dump.rgb"

 #for the time being we don't check hashes nor use same size/dur for our tests. We will redo the UI tests once finaizing filters branch
 dump_dur=5
 dump_size=192x192
 do_test "$GPAC -blacklist=vtbdec,nvdec -i $mp4file compositor:osize=$dump_size:vfr:dur=$dump_dur @ -o $RGB_DUMP" "dump"

 if [ -f $RGB_DUMP ] ; then
#  do_hash_test_bin "$RGB_DUMP" "$2-rgb"
  do_play_test "play" "$RGB_DUMP:size=$dump_size"
 else
  result="no output"
 fi

 #test saf demux
 myinspect=$TEMP_DIR/inspect_saf.txt
 do_test "$GPAC -i $saffile inspect:all:deep:interleave=false:log=$myinspect"

 #don't hash content on 32 bits, fp precision leads to different results, hence different crc
 if [ $is_linux32 != 1 ] ; then
  do_hash_test $myinspect "inspect-saf"
 fi
 #this will sync everything, we can delete after
 test_end

# rm $saffile 2> /dev/null
# rm $mp4file 2> /dev/null
}


laser_tests ()
{
 for xsr in $MEDIA_DIR/laser/*.xml ; do
  lsr_test $xsr
 done
}

#just in case
rm -f MEDIA_DIR/laser/*.mp4 2> /dev/null
rm -f MEDIA_DIR/laser/*.saf 2> /dev/null

lsr_test $MEDIA_DIR/laser/laser_all.xml
#don't do these, all covered by laser_all.xml
#laser_tests

