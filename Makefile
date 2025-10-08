TARGET = bin/dbview
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

run: clean default
		# check if double create is prevented
		./$(TARGET) -nf mydb.db -a "testing,123,456"
		# ./$(TARGET) -f mydb.db -la "George Michael,1,2"
		# ./$(TARGET) -f mydb.db -ld "testing"
		# ./$(TARGET) -f mydb.db -lu "George Michael,20"
		#./$(TARGET) -nf mydb.db
		# check if regular read is possible
		#./$(TARGET) -f mydb.db

default: $(TARGET)

clean:
		rm -f obj/*.o
		rm -f bin/*
		rm -f *.db

$(TARGET): $(OBJ)
		gcc -o $@ $?

obj/%.o : src/%.c
		gcc -c $< -o $@ -I include
