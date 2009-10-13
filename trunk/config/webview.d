#!/bin/sh

user=user
app=thttpd
app_dir=/opt/webview
pid_file=/var/run/$app.pid

test -f $app || exit 0

cd $app_dir

case "$1" in
start)
	/sbin/start-stop-daemon --start --oknodo -d $app_dir --pidfile $pid_file --exec $app_dir/$app -- -C $app_dir/config/thttpd.conf
	;;
stop)
	start-stop-daemon --stop --oknodo --name $app --retry 5
	;;
*)	
        exit 2
        ;;
esac
exit 0

