#!/bin/sh

if [ $2 ];then
	mode=$2
else
	mode="a2dp"
fi

if [ $3 ];then
	device=$3
else
	device="rtk"
fi

echo "|--bluez: device = $device mode = $mode--|"
flag_file="/tmp/bt_running"

set_running_flag()
{
	echo "|--bluez: set running flag|"
	echo 1 > $flag_file
}

clear_running_flag()
{
	echo "|--bluez: clear running flag|"
	rm -f $flag_file
}

realtek_bt_init()
{
	modprobe rtk_btuart
	modprobe rtk_btusb
	usleep 500000
	rtk_hciattach -n -s 115200 /dev/ttyS1 rtk_h5 &
}

A2DP_service()
{
	echo "|--bluez a2dp-sink/hfp-hf service--|"
	hciconfig hci0 up
	usleep 200000
	/usr/libexec/bluetooth/bluetoothd -n &
	usleep 200000
	bluealsa -p a2dp-sink -p hfp-hf &
	bluealsa-aplay --profile-a2dp 00:00:00:00:00:00 -d dmixer_auto &
	default_agent &
	hciconfig hci0 piscan
	hciconfig hci0 inqparms 18:1024
	hciconfig hci0 pageparms 18:1024


}

BLE_service()
{
	echo "|--bluez ble service--|"
	hciconfig hci0 up
	hciconfig hci0 noscan
	usleep 200000
	btgatt-server &
}

kill_and_check()
{
	p=$1
	local cnt=20
	while [ $cnt -gt 0 ]; do
		n=`ps | grep $p | wc -l`
		if [ $n -gt 1 ];then
			killall $p
			usleep 20000
			cnt=$((cnt - 1))
		else
			break
		fi
	done

	if [ $cnt -eq 0 ];then
		echo "try kill $p fail!!!"
		exit 0
	fi
}

service_down()
{
	echo "|--stop bluez service--|"
	killall default_agent
	killall bluealsa-aplay
	killall bluealsa
	killall bluetoothd
	killall btgatt-server

	kill_and_check default_agent
	kill_and_check bluealsa-aplay
	kill_and_check bluealsa
	kill_and_check bluetoothd
	kill_and_check btgatt-server

	hciconfig hci0 down

}

Blue_start()
{
	if [ -f $flag_file ];then
		echo "|--bluez service running!!--|"
		exit 2
	fi

	set_running_flag
	
	echo 0 > /sys/class/rfkill/rfkill0/state
	usleep 300000
	echo 1 > /sys/class/rfkill/rfkill0/state

	echo
	echo "|-----start bluez----|"
	if [ $device = "rtk" ];then
		realtek_bt_init
	else
		modprobe hci_uart
		usleep 300000
		hciattach -s 115200 /dev/ttyS1 any
	fi
	local cnt=50
	while [ $cnt -gt 0 ]; do
		hciconfig hci0 2> /dev/null
		if [ $? -eq 1 ];then
			echo "checking hci0 ......."
			usleep 200000
			cnt=$((cnt - 1))
		else
			break
		fi
	done

	if [ $cnt -eq 0 ];then
		echo "hcio shows up fail!!!"
		clear_running_flag
		exit 1
	fi

	if [ $mode = "ble" ];then
		BLE_service
	else
		A2DP_service
	fi

	echo "|-----bluez is ready----|"
	echo "==================BT end=============================="
	date "+%Y-%m-%d %H:%M:%S"

}

Blue_stop()
{
	echo -n "Stopping bluez"
	service_down
	killall rtk_hciattach
	killall hciattach
	rmmod hci_uart
	rmmod rtk_btusb
	echo 0 > /sys/class/rfkill/rfkill0/state
	clear_running_flag
	echo
	echo "|-----bluez is shutdown-----|"
}

case "$1" in
	start)
		echo "==================A2DP start=============================="
		date "+%Y-%m-%d %H:%M:%S"
		Blue_start &
		;;
	restart)
		echo "==================restart for BLE========================="
		date "+%Y-%m-%d %H:%M:%S"
		Blue_stop
		Blue_start
		;;
	up)
		service_up
		;;
	down)
		service_down
		;;
	reset)
		service_down
		service_up
		;;
	stop)
		Blue_stop
		;;
	*)
		echo "Usage: $0 {start|stop}"
		exit 1
esac

exit $?

