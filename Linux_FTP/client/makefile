CC := g++
CFLAGS := -Wall -g -Os

SHDIR := ../common

OBJS = ftpclient.o $(SHDIR)/common.o

all: ftpclient

ftpclient: $(OBJS)
	@$(CC) -o ftpclient $(CFLAGS) $(OBJS)

$(OBJS) : %.o: %.cpp 
	@$(CC) -c $(CFLAGS) $< -o $@

.PHONY:
clean:
	@rm -f *.o ftpclient
	@rm -f ../common/*.o
	@echo Done cleaning
