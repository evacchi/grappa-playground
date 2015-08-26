GRAPPA_HOME=$HOME/Devel/Cpp/grappa_master
GRAPPA_BUILD_DIR=$GRAPPA_HOME/build/Make+Release

g++ -c  -w -std=c++11 -w -fpermissive -O3 -fno-strict-aliasing -I$GRAPPA_HOME/system -I$GRAPPA_HOME/system/tasks -I$GRAPPA_BUILD_DIR/third-party/include -I/usr/include/mpich -o KMeans.o KMeans.cpp
g++   -o KMeans KMeans.o  -lGrappa -lglog -lgflags -ldl -lutil -lmpi -lz -lm -lc -lpthread -lboost_system -lboost_filesystem -L/usr/local/lib -L$GRAPPA_BUILD_DIR/system -L$GRAPPA_BUILD_DIR/third-party/lib -ljansson -ldl
                                                            
