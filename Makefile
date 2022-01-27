CC=gcc
TARGET=wrdsh

$(TARGET): $(TARGET).c
	$(CC) -Wall -o $(TARGET) $(TARGET).c
clean:
	rm -f $(TARGET)
