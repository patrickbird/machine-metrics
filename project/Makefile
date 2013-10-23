main.o: main.c system.o metric.o
	gcc -o main.o main.c system.o metric.o -lm

system.o: system.c
	gcc -c system.c

metric.o: metric.c
	gcc -c metric.c -lm

clean:
	rm -rf *o *s measure


