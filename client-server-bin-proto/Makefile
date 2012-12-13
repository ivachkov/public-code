# Data obtained from the shell/OS
OS	:=	$(shell uname)

# Generic Declarations
CC	=	cc
CFLAGS	= 	-Wall -I.
LDFLAGS	=

STRIP	=	strip
SRCCHK	=	cppcheck --enable=all

# Server CFLAGS and LDFLAGS
S_CFLAGS	=
S_LDFLAGS	=

# Client CFLAGS and LDFLAGS
C_CFLAGS	=
C_LDFLAGS	=

# Debug (make DEBUG=yes $target)
ifeq ($(DEBUG), yes)
	CFLAGS		+= -g
endif

# OS-specific 
ifeq ("${OS}", "Linux")
	CFLAGS 		+= -DLINUX
	S_CFLAGS	+=
	S_LDFLAGS	+=
	C_CFLAGS	+=
	C_LDFLAGS	+=
endif

ifeq ("${OS}", "FreeBSD")
	CFLAGS		+= -DFREEBSD
	S_CFLAGS	+=
	S_LDFLAGS	+=
	C_CFLAGS	+=
	C_LDFLAGS	+=
endif

# Server Source and Object Files
S_SRC	= server-bin-proto.c bprot.c
S_OBJ	= server-bin-proto.o bprot.o

# Client Source and Object Files
C_SRC	= client-bin-proto.c bprot.c
C_OBJ	= client-bin-proto.o bprot.o

# Binary file names
SERVER	= server-bin-proto
CLIENT	= client-bin-proto

all: $(S_SRC) $(C_SRC)
	$(CC) $(CFLAGS) $(C_CFLAGS) $(S_CFLAGS) -c *.c
	$(CC) $(CFLAGS) $(C_CFLAGS) -o $(CLIENT) $(C_OBJ) $(LDFLAGS) $(C_LDFLAGS)
	$(CC) $(CFLAGS) $(S_CFLAGS) -o $(SERVER) $(S_OBJ) $(LDFLAGS) $(S_LDFLAGS)	
	
client: $(C_SRC)
	$(CC) $(CFLAGS) $(C_CFLAGS) -c $(C_SRC)
	$(CC) $(CFLAGS) $(C_CFLAGS) -o $(CLIENT) $(C_OBJ) $(LDFLAGS) $(C_LDFLAGS)

server: $(S_SRC)
	$(CC) $(CFLAGS) $(S_CFLAGS) -c $(S_SRC)
	$(CC) $(CFLAGS) $(S_CFLAGS) -o $(SERVER) $(S_OBJ) $(LDFLAGS) $(S_LDFLAGS)

static-check: $(S_SRC) $(C_SRC)
	$(SRCCHK) $(S_SRC) 
	$(SRCCHK) $(C_SRC)

install: all
	@echo "NOT IMPLEMENTED!"

tests: all
	@echo "NOT IMPLEMENTED!"

clean:
	rm -rf *.o *core $(SERVER) $(CLIENT) 

debug:
	make DEBUG=yes
