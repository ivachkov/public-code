# Definitions 
CC	=	gcc
CFLAGS	=	-Wall -g
INC	=	-I. 
LDFLAGS	=	

PROG	=	sp_lg

SRC	=	main.c parser.c event_store.c
OBJ	=	main.o parser.o event_store.o

# Targets
all:
	$(CC) $(CFLAGS) -c $(SRC) $(INC)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJ) $(LDFLAGS)

clean:
	rm -rf $(PROG) $(RELAY) *.o *.core
