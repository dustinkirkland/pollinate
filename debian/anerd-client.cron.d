# Run once at boot, and hourly thereafter
@reboot		daemon	run-one anerd
*/42 * * * *	daemon	run-one anerd
