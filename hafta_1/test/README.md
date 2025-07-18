make clean
make
./daemon_logger
tail -f /tmp/ds.log 


ps aux | grep daemon_logger
kill 83412
