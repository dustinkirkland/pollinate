all:
	gccgo -g -o pollen pollen.go

clean:
	rm -f pollen
