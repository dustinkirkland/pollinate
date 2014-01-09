# Reseed the PRNG regularly, per FIPS 140-2 recommendation
# This was inspired by the NIST DRBG Special Publication 800-90A, which
# recommends periodically reseeding pseudo random number generators.
# http://csrc.nist.gov/groups/STM/cavp/documents/drbg/DRBGVS.pdf‎

# At package installation, the postinst will choose a random minute,
# 0-59, for this pollinate client to run.  The goal here is to spread
# the load on the entropy server.
__RAND__ * * * *	daemon	run-one pollinate
