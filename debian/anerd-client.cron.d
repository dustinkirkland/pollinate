# Run once at boot, and hourly thereafter
@reboot		daemon	run-one anerd
*/__RAND__ * * * *	daemon	run-one anerd
