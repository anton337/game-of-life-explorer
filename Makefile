FLAGS=-O3 -g0
#FLAGS=-O0 -g3

all:
	g++ $(FLAGS) glad.c main.cpp -lglfw -I. -o shader_test

clean:
	rm -f shader_test
