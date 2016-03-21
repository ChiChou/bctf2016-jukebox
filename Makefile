EXEC=jukebox

release:
	gcc -s -Wall -Wl,-z,relro,-z,now -g -o $(EXEC) $(EXEC).c -lsqlite3 -fPIC -pie -O3

run: release
	./$(EXEC)

clean:
	rm $(EXEC)