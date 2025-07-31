CC		= gcc
CFLAGS		= -Wall
LDFLAGS		= 
OBJFILES 	= function.o server.o
TARGET		= news_server

all: $(TARGET) $(TARGET1)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)

clean:
	rm -f $(OBJFILES) $(TARGET) *~
