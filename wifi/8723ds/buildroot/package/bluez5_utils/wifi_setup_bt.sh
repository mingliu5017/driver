#!/bin/sh

PATH=/bin:/sbin:/usr/bin:/usr/sbin
WIFI_FILE=/data/select.txt
ssid="CMCC-QLINK"
password="NONE"
RTK_WIFI_FLAG="NONE"
driver_list="8723ds"
wifi_chip_id=""
wifi_module_id=""
wifi_driver_name=""
MULTI_WIFI=/usr/bin/multi_wifi_load_driver
RTK_FLAG_FILE=/data/rtk_station_mode
WIFI_STATUS_FILE=/tmp/wifi_status
id=0

NAME1=wpa_supplicant
DAEMON1=/usr/sbin/$NAME1
PIDFILE1=/var/run/$NAME1.pid

NAME3=dnsmasq
DAEMON3=/usr/sbin/$NAME3
PIDFILE3=/var/run/$NAME3.pid

NAME4=dhcpcd
DAEMON4=/usr/sbin/$NAME4
PIDFILE4=/var/run/${NAME4}-wlan0.pid

init_wifi_env()
{
	killall dnsmasq
	killall dhcpcd
}

parse_paras()
{
	ssid=`sed -n "1p" $WIFI_FILE`
	password=`sed -n "2p" $WIFI_FILE`
	if [ "`echo $password |wc -L`" -lt "8" ];then
		echo "waring: password lentgh is less than 8, it is not fit for WPA-PSK"
	fi
}

reconnect_handler() {
	while [ $id -ne 0 ]; do
		wpa_cli remove_network $id
		id=$((id - 1))
		ensure_available $id
		if [ $? -eq 1 ]; then
			wpa_cli select_network $id
			wpa_cli enable_network $id
			check_state 15
			if [ $? -eq 1 ]; then
				dhcpcd wlan0
				#if [ -f $WIFI_STATUS_FILE ]; then
				echo 2 > $WIFI_STATUS_FILE
				#fi
				return 0
			fi
		fi
	done
	#if [ -f $WIFI_STATUS_FILE ]; then
	echo 3 > $WIFI_STATUS_FILE
	#fi
}

ping_test()
{
	killall udhcpc
	if [ $1 -eq 0 ];then
          echo "ping fail!! ip is NULL"
          #if [ -f $WIFI_STATUS_FILE ]; then
		  echo 3 > $WIFI_STATUS_FILE
          #fi
	  return 0
    fi
	echo "now going to ping router's ip: $1 for 5 seconds"
	ping $1 -W 3 -c 2
	if [ $? -eq 1 ];then
		echo "ping fail!! please check"
		reconnect_handler
	else
		echo "ping successfully"
		touch ${RTK_FLAG_FILE}
		if [ $ssid != "CMCC-QLINK" ]; then
			#ssid will not be repeat
			old_id=`wpa_cli list_network | grep -w $ssid | awk 'NR==1 {print $1}'`
			if [ $old_id -ne $id ]; then
				wpa_cli remove_network $old_id
			fi
			wpa_cli save_config
			sync
			#if [ -f  $WIFI_STATUS_FILE ] ;then
			echo 2 >  $WIFI_STATUS_FILE
			#fi
		fi
		date "+%Y-%m-%d %H:%M:%S"
		dhcpcd wlan0
	fi
}

ensure_available() {
        crt_ssid=`wpa_cli get_network $1 ssid | sed "1d" | sed 's/\"//g'`
        wpa_cli scan > /etc/null
        scan_list=`wpa_cli scan_result | cut -f 5- | sed "1,2d"`
        arr=$(echo $scan_list|tr "\n" "\n")

        for x in $arr; do
                if [ "$crt_ssid" = "$x" ]; then
                        return 1
                fi
        done
        return 0
}

check_state()
{
	local cnt=1
	while [ $cnt -lt $1 ]; do
		echo "check_in_loop processing..."
		ret=`wpa_cli status | grep "wpa_state"`
		ret=${ret##*=}
		if [ $ret == "COMPLETED" ]; then
			return 1
		else
			cnt=$((cnt + 1))
			sleep 1
			continue
		fi
	done
	return 0
}

wifi_setup()
{
	date "+%Y-%m-%d %H:%M:%S"
	init_wifi_env
	parse_paras
	id=`wpa_cli add_network | grep -v "interface"`
	echo "***************wifi setup paras***************"
	echo "**  id=$id                                  **"
	echo "**  ssid=$ssid                              **"
	echo "**  password=$password                      **"
	echo "**********************************************"
	wpa_cli set_network $id ssid \"$ssid\"
	if [ "$password" == "NONE" ]; then
		wpa_cli set_network $id key_mgmt NONE
	else
		wpa_cli set_network $id psk \"$password\"
	fi
	if [ $ssid == "CMCC-QLINK" ]; then
		wpa_cli set_network $id scan_ssid 1 
	fi

	wpa_cli select_network $id
	wpa_cli enable_network $id
	check_state 15
	if [ $? -eq 0 ] ;then
		echo "connect fail!!"
		reconnect_handler
	else
		echo "start wpa_supplicant successfully!!"
		ip_addr=`udhcpc -q -n -s /usr/share/udhcpc/default.script -i wlan0 2> /dev/null | grep "adding dns*" | awk '{print $3}'`
		ping_test $ip_addr $ssid
	fi
}

wifi_setup
