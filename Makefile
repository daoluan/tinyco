all : main udp_server

SRCCC = $(shell find ./ -maxdepth 1 -name "*.cc")
OBJS = $(SRCCC:%.cc=%.o)

udp_server : udp_server.o
	g++ $^ -o $@

main : $(OBJS)
	g++ $^ -o $@
	$(info good job!)

%.o : %.cc
	g++ -c $^ -o $@ -std=c++11 -g -O0

%.o : ./tool/udp_server/%.cc
	g++ -c $^ -o $@ -std=c++11 -g -O0

.PHONY: clean
clean:
	rm main -f
	rm *.o -f
