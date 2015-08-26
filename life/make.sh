GRAPPA_HOME=/home/vagrant/grappa_dev
GRAPPA_BUILD_DIR=$GRAPPA_HOME/build/Make+Debug

g++ -c  -w -pedantic -Wall -std=c++11 -w -fpermissive -O3 -fno-strict-aliasing -I$GRAPPA_HOME/system -I$GRAPPA_HOME/system/tasks -I$GRAPPA_BUILD_DIR/third-party/include -I/usr/include/mpich -o life.o life.cpp
g++   -o life life.o  -lGrappa -lglog -lgflags -ldl -lutil -lmpi -lz -lm -lc -lrt -lpthread -lboost_system -lboost_filesystem -L/usr/local/lib -L$GRAPPA_BUILD_DIR/system -L$GRAPPA_BUILD_DIR/third-party/lib -ljansson -ldl
                                                            
