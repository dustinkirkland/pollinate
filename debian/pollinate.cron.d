# Reseed the PRNG regularly, per FIPS 140-2 recommendation
# This was inspired by the NIST DRBG Special Publication 800-90A, which
# recommends periodically reseeding pseudo random number generators.
# http://csrc.nist.gov/groups/STM/cavp/documents/drbg/DRBGVS.pdf

# Each client should choose a random hour:minute to run each day.
# The goal here is to evenly distribute the load on your entropy server.
__RAND_MINUTE__ __RAND_HOUR__ * * *	daemon	run-one pollinate
