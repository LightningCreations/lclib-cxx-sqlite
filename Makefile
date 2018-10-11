CXX := g++
CC := gcc

OBJECT_FILES := out/SqliteDriver.o

CC_FLAGS := -g -fvisibility=hidden -fvisibility-inlines-hidden -std=c11 -fpic -w
COMPILE_FLAGS := -g -fvisibility=hidden -fvisibility-inlines-hidden -std=c++2a -fpic -w -fpermissive 
LINKER_FLAGS := -fvisibility=hidden -fvisibility-inlines-hidden -shared -fpic -flinker-output=dyn 
LIBS := -llc-cxx -lsqlite
OUTPUT := liblc-sqlite.so
INCLUDE := -I./include
DEFINES := -D_LIBLC_CXX_SQLITE_BUILD
LIBNAME = -Wl,-soname,liblc-sqlite.so

DIRS := out/ 

all: $(OUTPUT)

out:
	mkdir -p $(DIRS)

$(OUTPUT): out $(OBJECT_FILES)
	$(CXX) $(LINKER_FLAGS) $(LIBNAME) -o $@ $(LIBS) $(OBJECT_FILES) 

install:$(OUTPUT)
	install $(OUTPUT) /usr/lib/
	install --mode=755 -d -v /usr/include/lclib-cxx-sqlite include/lclib-cxx-sqlite
	cp -R include/lclib-cxx-sqlite /usr/include
	chmod -R 755 /usr/include/lclib-cxx-sqlite

uninstall:
	rm -rf /usr/include/lclib-cxx-sqlite
	rm -rf /usr/lib/$(OUTPUT)

relink:
	rm -rf $(OUTPUT)
	make $(OUTPUT)


clean:
	rm -rf out
	rm -f $(OUTPUT)
	rm -f init

rebuild:
	make clean
	make $(OUTPUT)

out/%.o: src/%.cpp
	$(CXX) $(COMPILE_FLAGS) $(DEFINES) -c $(INCLUDE) -o $@ $<

out/%.o: src/%.c
	$(CC) $(CC_FLAGS) $(DEFINES) -c $(INCLUDE) -o $@ $<
	
	