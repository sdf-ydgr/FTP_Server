
#This is a hack to pass arguments to the run command and probably only 
#works with gnu make. 
ifeq (run,$(firstword $(MAKECMDGOALS)))
  # use the rest as arguments for "run"
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  # ...and turn them into do-nothing targets
  $(eval $(RUN_ARGS):;@:)
endif


all: ftp_Server

#The following lines contain the generic build options
CC=gcc
CPPFLAGS=
CFLAGS=-g -Werror-implicit-function-declaration

#List all the .o files here that need to be linked 
OBJS=ftp_Server.o usage.o dir.o ftp_Commands.o

usage.o: usage.c usage.h

dir.o: dir.c dir.h

ftp_Server.o: ftp_Server.c dir.h usage.h ftp_Commands.o ftp_Commands.c ftp_Commands.h

ftp_Commands.o: ftp_Commands.c ftp_Commands.h

ftp_Server: $(OBJS) 
	$(CC) -o ftp_Server $(OBJS) 

clean:
	rm -f *.o
	rm -f ftp_Server

.PHONY: run
run: ftp_Server  
	./ftp_Server $(RUN_ARGS)
