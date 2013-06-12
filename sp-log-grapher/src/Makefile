# Definitions 
CC	=	gcc
CFLAGS	=	-Wall -Werror -g
INC	=	-I. 
LDFLAGS	=	

PROG	=	sp_lg

SRC	=	main.c parser.c event_store.c stat_data_buf.c 
OBJ	=	main.o parser.o event_store.o stat_data_buf.o

# Targets
all:
	$(CC) $(CFLAGS) -c $(SRC) $(INC)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJ) $(LDFLAGS)

clean:
	rm -rf $(PROG) $(RELAY) *.o *.core

