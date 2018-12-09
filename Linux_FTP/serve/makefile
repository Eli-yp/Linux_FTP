CC := g++
CFLAGS := -Wall -g -Os

SHDIR := ../common

OBJS = ftpserve.o $(SHDIR)/common.o

all: ftpserve

ftpserve: $(OBJS)
	@$(CC) $(CFLAGS) $(OBJS) -o ftpserve 

$(OBJS) : %.o: %.cpp 
	@$(CC) -c $(CFLAGS) $< -o $@

.PHONY:
clean:
	@rm -f *.o ftpserve
	@rm -f ../common/*.o
	@echo Done cleaning
