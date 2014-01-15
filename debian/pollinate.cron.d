# Reseed the PRNG regularly, per FIPS 140-2 recommendation
# This was inspired by the NIST DRBG Special Publication 800-90A, which
# recommends periodically reseeding pseudo random number generators.
# http://csrc.nist.gov/groups/STM/cavp/documents/drbg/DRBGVS.pdf‎

# At package installation, the postinst will choose a random
# hour:minute, for this pollinate client to run each day.
# The goal here is to spread the load on the entropy server.
__RAND_MINUTE__ __RAND_HOUR__ * * *	daemon	run-one pollinate
