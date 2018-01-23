CC = gcc
CFLAGS = -g -O0 -Wall -Wextra -std=gnu99

TARGET = test

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $@ $@.c -lm

bench: $(TARGET)
	./test

plot_shuffle: shuffle.gp
	gnuplot shuffle.gp

plot_strcpy: dot.gp average_line.gp
	gnuplot dot.gp
	gnuplot average_line.gp

clean:
	$(RM) $(TARGET) *.o *.png *.txt
