all:
	gccgo -o pollen pollen.go

clean:
	rm -f pollen
