CC = gcc 

OBJ = obj/main.o obj/alpha_b.o obj/logs.o obj/general.o

all: global

global: $(OBJ) 
	$(CC) $(OBJ) -lsfml-graphics -lsfml-window -lsfml-system -lstdc++  -lm -o overlay -fsanitize=bounds -fsanitize=address

obj/main.o: src/main.cpp src/global_conf.h
	$(CC) src/main.cpp -c -o obj/main.o 

obj/alpha_b.o: src/global_conf.h src/alphablending/alpha_b.cpp src/alphablending/alpha_b.h src/alphablending/alpha_b_conf.h
	$(CC) src/alphablending/alpha_b.cpp -c -o obj/alpha_b.o -O2 -mavx -mavx2 -msse4 -lm

obj/logs.o: src/global_conf.h src/logs/errors_and_logs.cpp src/logs/errors_and_logs.h src/logs/errors.h src/logs/log_definitions.h src/include/errors.txt
	$(CC) src/logs/errors_and_logs.cpp -c -o obj/logs.o 

obj/general.o: src/global_conf.h src/general/general.cpp src/general/general.h 
	$(CC) src/general/general.cpp -c -o obj/general.o

.PNONY: cleanup

cleanup:
	rm obj/*.o 
