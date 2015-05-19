gcc -I/usr/local/include/libmongoc-1.0 -I/usr/local/include/libbson-1.0 -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"src/structs/dir.d" -MT"src/structs/dir.d" -o "src/structs/dir.o" "../src/structs/dir.c"

gcc -I/usr/local/include/libmongoc-1.0 -I/usr/local/include/libbson-1.0 -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"src/structs/file.d" -MT"src/structs/file.d" -o "src/structs/file.o" "../src/structs/file.c"

gcc -I/usr/local/include/libmongoc-1.0 -I/usr/local/include/libbson-1.0 -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"src/mongo/mongo.d" -MT"src/mongo/mongo.d" -o "src/mongo/mongo.o" "../src/mongo/mongo.c"

gcc -I/usr/local/include/libmongoc-1.0 -I/usr/local/include/libbson-1.0 -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"src/mongo/mongo_dir.d" -MT"src/mongo/mongo_dir.d" -o "src/mongo/mongo_dir.o" "../src/mongo/mongo_dir.c"

gcc -I/usr/local/include/libmongoc-1.0 -I/usr/local/include/libbson-1.0 -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"src/mongo/mongo_file.d" -MT"src/mongo/mongo_file.d" -o "src/mongo/mongo_file.o" "../src/mongo/mongo_file.c"
 
gcc -I/usr/local/include/libmongoc-1.0 -I/usr/local/include/libbson-1.0 -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"src/console/console.d" -MT"src/console/console.d" -o "src/console/console.o" "../src/console/console.c"

gcc -I/usr/local/include/libmongoc-1.0 -I/usr/local/include/libbson-1.0 -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"src/filesystem.d" -MT"src/filesystem.d" -o "src/filesystem.o" "../src/filesystem.c"

gcc -L/usr/local/lib -o "FileSystem"  ./src/structs/dir.o ./src/structs/file.o  ./src/mongo/mongo.o ./src/mongo/mongo_dir.o ./src/mongo/mongo_file.o  ./src/console/console.o  ./src/filesystem.o   -lcommons -lbson-1.0 -lmongoc-1.0
