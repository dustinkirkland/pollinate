# Run once at boot, and hourly thereafter
@reboot		daemon	run-one pollinate
*/__RAND__ * * * *	daemon	run-one pollinate
