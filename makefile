all:
	g++ -g -O2 -o main main.cpp -lsimlib -lm
run: main
	./main
clear:
	rm main