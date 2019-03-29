#!/bin/sh

##########################################################
#       Neoway_N720V5_LTE 模块与WIFI模块切换 
##########################################################

voice_res_dir=/data/4g_test
port_4g=eth0
port_tty=/dev/ttyACM1

waitfor_device()
{
	count=10
	
	while true
	do
		if [ -f "$port_tty" ]; then
			return 0
		fi
		
		if [ $count -lt 0 ];then
			break
		fi
		
		count=$((count - 1))
		sleep 1
	done
	
	return 1
}


check_ip()
{
	net_port=$1
	count=10
	
	while true
	do
		ifconfig $net_port | grep -iE "inet addr"
		
		if [ $? -eq 0 ]; then
			return 0
		fi
		
		if [ $count -lt 0 ];then
			break
		fi
		
		count=$((count - 1))
		sleep 1
	done
	
	return 1
}


switch_to_4g()
{
	gst-play-1.0 $voice_res_dir/to_4g.mp3
	count=10
	flag=0
	echo "================================111"
	while true
	do
		ifconfig $port_4g up
		
		if [ $? -eq 0 ]; then
			flag=1
			break
		fi
		
		if [ $count -lt 0 ];then
			break
		fi
		
		count=$((count - 1))
		sleep 1
	done
	echo "================================222"
	if [ $flag -eq 1 ]; then
		echo "================================333"
		killall dhcpcd 
		dhcpcd $port_4g
		echo "================================444"
		
		waitfor_device
		echo "================================555"
		if [ $? -ne 0 ]; then
			echo "$port_tty not exist!!!"
			return 1
		fi
		echo "================================666"
		#激活4g网络
		echo -e "AT\$MYNETACT=0,1\r\n" >$port_tty
		echo "================================777"
		check_ip $port_4g
		echo "================================888"
		if [ $? -eq 0 ]; then
			ifconfig wlan0 down
			gst-play-1.0 $voice_res_dir/to_4g_ok.mp3
			return 0
		fi		
	fi
	
	gst-play-1.0 $voice_res_dir/to_4g_failed.mp3
	return 1
}

switch_to_wifi()
{
	#关闭4g网络
	echo -e "AT\$MYNETACT=0,0\r\n" >$port_tty
	ifconfig $port_4g down
	
	gst-play-1.0 $voice_res_dir/to_wifi.mp3
	
	count=20
	flag=0

	/etc/init.d/S42wifi start
	
	if [ $? -eq 0 ]; then
		gst-play-1.0 $voice_res_dir/to_wifi_ok.mp3
		return 0
	fi
	
	return 1
}

case $1 in
	4g)
	switch_to_4g   1>/dev/null 2>&1
	;;
	
	*)
	;;
esac



