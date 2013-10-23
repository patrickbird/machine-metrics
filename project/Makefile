measure: main.c system.o metric.o
	gcc -o measure -pthread main.c system.o metric.o -lm -lpthread

system.o: system.c
	gcc -c system.c

metric.o: metric.c
	gcc -c -pthread metric.c -lm -lpthread

clean:
	rm -rf *o *s measure


