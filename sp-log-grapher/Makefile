# Definitions
CODE	=	src
PROG	=	sp_lg

# Targets
all:
	$(MAKE) -C $(CODE)
	mkdir -p bin
	cp $(CODE)/$(PROG) bin

clean:
	$(MAKE) -C $(CODE) clean
	rm -rf bin

